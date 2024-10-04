import numpy as np
import time
import cv2
from ctypes import cdll, c_double, POINTER, c_int, c_uint32, c_uint16, c_uint8, c_bool, c_float, c_char_p #, c_longdouble
import argparse
import sys
import os
import re
from decimal import Decimal, getcontext
# Carregue a biblioteca
lib = cdll.LoadLibrary('./libfract.so')


fractal = lib.fractal
lyapunov = lib.lyapunov
sandpile = lib.sandpile

lib.process_array.argtypes = [POINTER(c_uint32), POINTER(c_uint8), c_uint16, c_uint16, c_double, c_uint16, c_double]
lib.process_array.restype = None
lib.scale.argtypes = [POINTER(c_float), POINTER(c_float), c_int, c_float, c_float] 
lib.scale.restype = None

fractal.argtypes = [POINTER(c_uint16), c_char_p, c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double, c_double, c_bool, c_bool, c_double, c_double]
lyapunov.argtypes = [POINTER(c_uint16), c_char_p, c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double, c_double, c_bool, c_double, c_double]
sandpile.argtypes = [POINTER(c_uint8), c_uint16, c_uint16, c_uint32, c_uint16]


def scale(input_array, min, max):
    shape = input_array.shape
    size = (input_array.size)
    
    input_array = input_array.copy().reshape(-1).astype(np.float32) 
    input_array = input_array.ctypes.data_as(POINTER(c_float))
    output_array = (c_float * (size))()
    # Call the function
    lib.scale(input_array, output_array, size, min, max)
    # Convert the output array to a numpy array
    output_array = np.ctypeslib.as_array(output_array).reshape(shape)
    
    return output_array



def scale_fast(input, max):
    return (input%(max+1))



# This can only scale positive numbers not negative numbers
def process_image(input_array, max_val, imgname):
    width, height = input_array.shape
    max = np.float64(np.max(input_array))
    max_val = np.float64(max_val)

    input_array = input_array.copy().reshape(-1).astype(np.uint32)
    input_array = input_array.ctypes.data_as(POINTER(c_uint32))
    output_array = (c_uint8 * (width * height* 3))()

    # Call the function
    lib.process_array(input_array, output_array, width, height, max_val, 5000, max)
    del input_array
    # Convert the output array to a numpy array
    output_array = np.ctypeslib.as_array(output_array).reshape(width, height, 3 )

    output_array = cv2.cvtColor(output_array, cv2.COLOR_BGR2RGB)
    
    if "sandpile" in imgname:
        output_array = cv2.resize(output_array, (height*4, width*4), interpolation=cv2.INTER_NEAREST)
    
    cv2.imwrite(f'{imgname}.png', output_array)
    print(f'{imgname}.png' )




def image_to_array(image_path, min=0, max=2**24-1):
        img = cv2.imread(image_path)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB) 
        array_image = np.array(img).astype(np.uint32)
    
        if array_image.ndim != 3:
            array_image = np.array(img.convert("RGBA"))
            
        if array_image.ndim == 3:
              if array_image.shape[2] == 4:
                      array_image = array_image[:, :, :-1]
                      
              if array_image.shape[2] == 3:
                      array_image = (array_image[:, :, 0]*(256**2)+array_image[:, :, 1]*(256)+array_image[:, :, 2])
            
        return array_image

def generate_gradient(arr, n_grad):
    n_grad = n_grad + 2
    if (n_grad < 3):
        return arr
        
    n = len(arr)
    
    arr = arr.copy().astype(np.int32)
    r0, g0, b0 = (arr >> 16) & 0xFF, (arr >> 8) & 0xFF, arr & 0xFF
    arr = np.concatenate((arr[1:], arr[:1])) 
    r1, g1, b1 = (arr >> 16) & 0xFF, (arr >> 8) & 0xFF, arr & 0xFF

    dr, dg, db = r1 - r0, g1 - g0, b1 - b0
    
    i = np.linspace(0, n_grad - 1, n_grad)[:-1]
    i = np.vstack([i]*n).T
    
    r = np.int32(r0 + (dr * i) / (n_grad - 1))
    g = np.int32(g0 + (dg * i) / (n_grad - 1))
    b = np.int32(b0 + (db * i) / (n_grad - 1))
    
    return (((r << 16) + (g << 8) + b).T).reshape(-1)
    
    
def palette_load(palette, gradient, top_colors=4, lake_palette=False, lake=False):
    if (lake == False) or (lake_palette == False):
        palette = image_to_array(palette)
        unique_colors, counts = np.unique(palette, return_counts=True)
        del palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors = unique_colors[sorted_indices][:top_colors]
        return generate_gradient(array_top_colors, gradient), False
    else:
        palette = image_to_array(palette)
        unique_colors, counts = np.unique(palette, return_counts=True)
        del palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors = unique_colors[sorted_indices][:top_colors]
        
        # Lake palette
        lake_palette = image_to_array(lake_palette)
        unique_colors, counts = np.unique(lake_palette, return_counts=True)
        del lake_palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors_lake = unique_colors[sorted_indices][:top_colors]

        return generate_gradient(array_top_colors, gradient), generate_gradient(array_top_colors_lake, gradient) 




# Image with palette
def create_image(data, filename, iterations, array_top_colors, lake=False, shift_palette=(0, 0) ):
    data = data.copy().astype(np.uint32)
    shape = data.shape
    shape = (shape[1], shape[0])
    data = data.reshape(shape)
    
    array_top_colors = (
    np.roll(array_top_colors[0], shift_palette[0]),
    np.roll(array_top_colors[1], shift_palette[1]) if array_top_colors[1] is not False else False
    )
    
    if (lake and isinstance(array_top_colors[1], np.ndarray) and not ('lyapunov' in filename or 'sandpile' in filename)):
        temp = data > iterations
        
        data[temp] = scale_fast(data[temp], array_top_colors[1].shape[0] - 1) + iterations + 1

        lake_indices = data - iterations - 1
        data[temp] = np.take(array_top_colors[1], lake_indices[temp]) + iterations
        
        data[~temp] = scale_fast(data[~temp], array_top_colors[0].shape[0] - 1)
        
        data[~temp] = np.take(array_top_colors[0], data[~temp])

        data[temp] = data[temp] - iterations
        del temp
    else:
        data = scale_fast(data, array_top_colors[0].shape[0] - 1)
        data = np.take(array_top_colors[0], data)

    process_image(data.reshape(shape), np.max(data), filename)
    
    
    
# This helps you to aim by dividing in squares(grid)
def divide_in_squares(list_c, xmin, xmax, ymin, ymax):
    list = list_c.copy()
    list[:,:2] = list[:,:2]-1
    for col, line, n_squares in list:
        size_x = (xmax - xmin) / n_squares
        size_y = (ymax - ymin) / n_squares
        
        new_xmin = xmin + col * size_x
        new_xmax = xmin + (col + 1) * size_x
        new_ymin = ymin + line * size_y
        new_ymax = ymin + (line + 1) * size_y
        xmin, xmax, ymin, ymax = new_xmin, new_xmax, new_ymin, new_ymax
    
    return xmin, xmax, ymin, ymax






width = int(1024) # I'm using ratio 1/1
height = int(1024) #2304

# Number of iterations
max_iter = 500

# Sandpile max grains
max_grains = 3


# The equation
expression = "z*z+c"         # z = "z^2 + c"

# You can generate different types of fractals
fractals = {
    'mandelbrot': True,
    'juliaset': True,
    'lyapunov': False,    # Lyapunov seems to run very slowly at high resolution try it with 1600x1600.
    'sandpile': False,     # Try sandpile with less resolution and much more iterations(=grains of sand) to get better results, but don't let the colored area touch the border or you will get broken results.
}


zoom = False
max_zoom = 10 # How many images # it's gonna generate  +n_coordinates more images than expected
per_zoom = Decimal("0.9") # Zooming after aiming: Using a value greater than 1.0 will zoom out; using a value less than 1.0 will zoom in
video_out = False # If you want to generate a video with the images using ffmpeg
imgfromvidfolder = ""       # Folder to save all the imgs, it will be on ./images/yourfoldername try "imgs/"
os.mkdir("./images/"+imgfromvidfolder) if len(imgfromvidfolder) != 0 and video_out else None


palette = "./palettes/palette.png"  # Palette location
use_palette = True
gradient = 16        # Amount of colors between the colors

# How many top colors to use from the palette.png
top_colors = 24
shift_palette = (0, 0)   # This shift the palette, you can set negative and positive integers.

# Julia set parameters / Lyapunov uses it as the imaginary part if juliaset is off
juliaset_c_real = -0.8
juliaset_c_imag = 0.16

# Quaternion parameters
quaternion_j = 0.0
quaternion_k = 0.0

# Makes the part that converges visible
lake = True
# Palette path to another palette image
lake_palette = "./palettes/lake_palette.png"
# Here it's loading the palette before the generation and conversion
array_top_colors = palette_load(palette, gradient, top_colors, lake_palette, lake)


getcontext().prec = 28
# Here you can move around
xmin, xmax, ymin, ymax = Decimal("-2.7"),Decimal("2.7"),Decimal("-2.7"),Decimal("2.7")



# This part is to help you aim
n_coordinates = 0   #  Number of coordinates to use, set False to not use it
#                       ([(column, row, grid n*n)])
coordinates = np.array([(1,2,3),(2,2,3),(2,1,2),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)])

#coordinates = np.array([(1,1,3),(2,3,4),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)]) 
#coordinates = np.array([(3,3,3),(3,4,5),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)])




# Uncomment the code below if you want to start at certain location
if n_coordinates>0:
    xmin, xmax, ymin, ymax = divide_in_squares(coordinates[:(n_coordinates), :], xmin, xmax, ymin, ymax)







def generate(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, n_iter, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette, quaternion_j, quaternion_k):
    print("\nYour coordinates: ", xmin, xmax, ymin, ymax, "\n")

    array_top_colors = palette_load(palette, gradient, top_colors, lake_palette, lake)
    prefix = ""
    img_names = []

    print(expression.replace(" ", ""))
    expression = re.sub(r'\bc\b', 'rw', re.sub(r'\bz\b', 'rt', expression)).replace(" ", "")

    
    if zoom:
        max_zoom = str(max_zoom)
        target_length = len(max_zoom)+1
        n_iter = str(n_iter)
        n_iter = n_iter.zfill(target_length)
        prefix = n_iter+"-"
        
    for key, value in fractals.items():

        
        # Mandelbrot Set/Julia Set
        if ((key == "juliaset") or (key == "mandelbrot")) and (value):
            gen_array = np.empty((height, width), dtype=np.uint16)
            start_time = time.perf_counter()
            fractal(gen_array.ctypes.data_as(POINTER(c_uint16)), c_char_p(expression.encode('utf-8')), width, height, max_iter, xmin, xmax, ymin, ymax, juliaset_c_real, juliaset_c_imag, "juliaset" == key, lake, quaternion_j, quaternion_k)
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        # Lyapunov Set
        if (key == "lyapunov") and (value):
            gen_array = np.empty((height, width), dtype=np.uint16)
            start_time = time.perf_counter()
            lyapunov(gen_array.ctypes.data_as(POINTER(c_uint16)), c_char_p(expression.encode('utf-8')), width, height, max_iter, xmin, xmax, ymin, ymax, juliaset_c_real, juliaset_c_imag, not fractals.get('juliaset'), quaternion_j, quaternion_k)
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        # Abelian Sandpile Fractal
        if (key == "sandpile") and (value):
            gen_array = np.empty((height, width), dtype=np.uint8)
            start_time = time.perf_counter()
            sandpile(gen_array.ctypes.data_as(POINTER(c_uint8)), width, height, max_iter, max_grains)
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        if "gen_array" in locals():
            start_time = time.perf_counter()
            localtime = time.strftime("%Y%m%d_%H%M%S", time.localtime())
            if use_palette:
                create_image(gen_array.reshape(width, height), "./images/"+ imgfromvidfolder + prefix + "0" + localtime + "_colorful_"+key, max_iter, array_top_colors, lake, shift_palette)
                img_names.append("./images/"+ imgfromvidfolder + prefix + "0" + localtime + "_colorful_"+key+".png")
            else:
                process_image(gen_array, (2**24-1), "./images/"+ imgfromvidfolder + prefix + "0" + localtime + "_generated_fractal_"+key )
                img_names.append("./images/"+ imgfromvidfolder + prefix + "0" + localtime + "_generated_fractal_"+key+".png")
            end_time = time.perf_counter()
            del gen_array
            print("Took ", end_time - start_time, "seconds to convert\n")
    return img_names

            
            
def generate_wrapper(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette, quaternion_j, quaternion_k):
    
    if zoom:
        assert fractals["sandpile"]== False, "Error: Can't zoom on sandpile."
        # The first image generated
        generate(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, 0, n_coordinates+max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette, quaternion_j, quaternion_k)
        
        xmin1, xmax1, ymin1, ymax1 =  xmin, xmax, ymin, ymax
        
        
        
        for i in range(n_coordinates+max_zoom): 
            if (i < n_coordinates) and (n_coordinates !=False):
                xmin, xmax, ymin, ymax = divide_in_squares(coordinates[:(i+1), :], xmin1, xmax1, ymin1, ymax1)
            else:
                
                x_center = (xmin + xmax) / 2
                y_center = (ymin + ymax) / 2
                
                widtho = (xmax - xmin) * per_zoom
                heighto = (ymax - ymin) * per_zoom
                
                xmin = x_center - widtho / 2
                xmax = x_center + widtho / 2
                ymin = y_center - heighto / 2
                ymax = y_center + heighto / 2
        
            generate(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, i+1, n_coordinates+max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette, quaternion_j, quaternion_k)
    else:
            # Normal mode without zoom
            img_names = generate(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, 0, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette, quaternion_j, quaternion_k)
            return img_names








def imgs_to_video(n_coordinates):
    import subprocess
    
    image_folder = "./images/" + imgfromvidfolder
    
    fps = 10
    frac = ["colorful_mandelbrot", "colorful_juliaset", "colorful_lyapunov"]
    image_files = sorted([f for f in os.listdir(image_folder) if f.endswith('.png') and re.search(r"\d+-", f) and 'colorful' in f])
    print(image_files)
    for i, n in enumerate(frac):
        
        filtered_files = [f for f in image_files if n in f]

        pattern = re.compile(rf".*{re.escape(n)}\.png$")

        if any(pattern.match(f) for f in filtered_files): 
            
            with open('input.txt', 'w') as f:
                for index, image_file in enumerate(filtered_files):
                    duration = 0.9 if index < n_coordinates else 0.1
                    f.write(f"file './images/{imgfromvidfolder}{image_file}'\n")
                    f.write(f"duration {duration}\n")
        
                f.write(f"file './images/{imgfromvidfolder}{image_files[-1]}'\n")
        
            subprocess.run([
                'ffmpeg', '-f', 'concat', '-safe', '0', '-i', 'input.txt', 
                '-fps_mode', 'vfr', '-pix_fmt', 'yuv420p', '-vf', f'fps={fps}', f'video_{n}.mp4' 
            ])
        
            os.remove('input.txt')
        
            print(f'\nvideo_{n}.mp4 Video Done!')
            
            



# stop_gen = False
from multiprocessing import Process, Value

fractal_process = None
stop_flag = Value('b', False)


received_params = {}
# stop_gen = multiprocessing.Event()




def process_form_data(params):
    global xmin, xmax, ymin, ymax





    def is_url_or_path(value):


        url_regex = re.compile(
            r'^(?:http|ftp)s?://' # http:// or https://
            r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|' # Domain
            r'localhost|' # localhost
            r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}|' # IP
            r'\[?[A-F0-9]*:[A-F0-9:]+\]?)' # IPv6
            r'(?::\d+)?' # port
            r'(?:/?|[/?]\S+)$', re.IGNORECASE)

        if re.match(url_regex, value):
            return "url"
        elif os.path.isfile(value):
            return "image path"
        else:
            return "invalid"
        

    def download_image(url):
        # Check if the value is a URL
        if is_url_or_path(url) != "url":
            return url  # If not a URL, return the original value
        
        # Download the image
    

        import requests
        import hashlib
        
        response = requests.get(url)
        if response.status_code == 200:
            # Calculate MD5 hash of the content
            md5_hash = hashlib.md5(response.content).hexdigest()
            # Determine the extension
            ext = url.split('.')[-1].lower()
            if ext not in ['png', 'jpg', 'jpeg']:
                ext = 'png'  # Fallback to PNG if unsupported format
            
            file_name = f"./palettes/{md5_hash}.{ext}"
            
            if os.path.isfile(file_name):
                return file_name 
            
            with open(file_name, 'wb') as file:
                file.write(response.content)
            
            return file_name
        else:
            raise Exception(f"Failed to download the image. Status Code: {response.status_code}")






    expression = params.get('expression', ['z*z+c'])

    fractals = params.get("fractals", {
    'mandelbrot': False,
    'juliaset': False,
    'lyapunov': False,
    'sandpile': False,
    })

    width = int(params.get('width', [1024]))
    height = int(params.get('height', [1024]))
    max_iter = int(params.get('max_iter', [400]))
    top_colors = int(params.get('top_colors', [24]))
    max_grains = int(params.get('max_grains', [3]))
    juliaset_c_real = float(params.get('juliaset_c_real', [-0.8]))
    juliaset_c_imag = float(params.get('juliaset_c_imag', [0.16]))

    use_palette = bool(params.get('use_palette', True))
    palette = params.get('palette', [''])
    palette = download_image(palette)
    gradient = int(params.get('gradient', [16]))

    lake = bool(params.get('lake', True))
    lake_palette = params.get('lake_palette', [''])
    lake_palette = download_image(lake_palette)

    shift_palette = int(params.get('shift_palette'))
    shift_palette_lake = int(params.get('shift_palette_lake'))
    column_aim = int(params.get('column_aim'))
    row_aim = int(params.get('row_aim'))
    grid_length = int(params.get('grid_length'))
    coordinates = np.array([(column_aim,row_aim,grid_length)])
    continue_aim = bool(params.get('continue_aim', False))
    quaternion_j = float(params.get('quaternion_j', [0.0]))
    quaternion_k = float(params.get('quaternion_k', [0.0]))

    if continue_aim and grid_length != 1:

        xmin, xmax, ymin, ymax = divide_in_squares(coordinates, xmin, xmax, ymin, ymax) 
    else:
        xmin = Decimal(params.get('xmin', [-2.0]))
        xmax = Decimal(params.get('xmax', [2.0]))
        ymin = Decimal(params.get('ymin', [-2.0]))
        ymax = Decimal(params.get('ymax', [2.0]))

    
    zoom = False
    max_zoom = 20


    if use_palette and not os.path.exists(palette):
        print(f"Palette file does not exist: {palette}")
        return
    if lake and not os.path.exists(lake_palette):
        print(f"Lake palette file does not exist: {lake_palette}")
        return

    
    paths = generate_wrapper(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax, [shift_palette, shift_palette_lake],quaternion_j, quaternion_k)
    
    #print((paths))
    return paths




def server(port):
    global xmin, xmax, ymin, ymax
    xmin_xmax = np.array([(-(2.0)), ((2.0))], dtype=np.float64)
    ymin_ymax = np.array([-(2.0), (2.0)], dtype=np.float64)
    xmin, xmax, ymin, ymax = xmin_xmax[0], xmin_xmax[1], ymin_ymax[0], ymin_ymax[1]



    import http.server
    import socketserver
    import signal
    import json



    PORT = port
    DIRECTORY = ""
    





    class MyHttpRequestHandler(http.server.SimpleHTTPRequestHandler):
        


        def do_POST(self):
            global received_params, stop_gen


            content_type = self.headers.get('Content-Type')
            if content_type != 'application/json; charset=utf-8':
                self.send_response(400)
                self.end_headers()
                return

            content_length = int(self.headers['Content-Length'])
            body = self.rfile.read(content_length)
            try:
                received_params = json.loads(body.decode('utf-8'))
            except json.JSONDecodeError:
                self.send_response(400)
                self.end_headers()
                return

            required_keys = ['width', 'height', 'max_iter', 'top_colors', 'max_grains', 'juliaset_c_real', 'juliaset_c_imag', 'xmin', 'xmax', 'ymin', 'ymax', 'palette', 'lake_palette']
            if all(key in received_params for key in required_keys):
                fractal_result = ",".join(process_form_data(received_params))
                received_params.clear()

                self.send_response(200)
                self.send_header('Content-Type', 'application/json; charset=utf-8')
                self.end_headers()
                self.wfile.write(fractal_result.encode('utf-8'))
            else:
                self.send_response(400)
                self.end_headers()

        def translate_path(self, path):
            return http.server.SimpleHTTPRequestHandler.translate_path(self, f'/{DIRECTORY}{path}')

    def signal_handler(signum, frame):
        print(f"Signal {signum} received. Shutting down.")
        sys.exit(0)

    signal.signal(signal.SIGTERM, signal_handler)

    Handler = MyHttpRequestHandler

    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"\nAnother Fractal Generator running at: http://localhost:{PORT}\n")
        httpd.serve_forever()





def main():
    global video_out, n_coordinates
    parser = argparse.ArgumentParser(description='Process some arguments.')

    # Add the --noserver flag
    parser.add_argument(
        '--noserver', 
        action='store_true', 
        help='If specified, the server will not be started.'
    )
    parser.add_argument(
        '--port', 
        type=int,
        default=8888,
        help='Port to serve. Defaults to 8888 if not specified.'
    )
    
    args = parser.parse_args()
    
    if args.noserver:
        # Let's Run
        generate_wrapper(fractals, expression, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, gradient, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette, quaternion_j, quaternion_k)
        # n_coordinates is how many times it will use the array coordinates.
        if video_out:
            imgs_to_video(n_coordinates)

    else:

        server(args.port)


if __name__ == "__main__":
    main()
