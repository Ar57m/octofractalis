import numpy as np
import time
import cv2
from ctypes import cdll, c_double, POINTER, c_int, c_uint32, c_uint16, c_uint8, c_bool, c_float #, c_longdouble
import argparse


# Carregue a biblioteca
lib = cdll.LoadLibrary('./libfract.so')


fractal = lib.fractal
juliaset = lib.juliaset
lyapunov = lib.lyapunov
sandpile = lib.sandpile

lib.process_array.argtypes = [POINTER(c_uint32), POINTER(c_uint8), c_uint16, c_uint16, c_double, c_uint16, c_double]
lib.process_array.restype = None
lib.scale.argtypes = [POINTER(c_float), POINTER(c_float), c_int, c_float, c_float] 
lib.scale.restype = None

fractal.argtypes = [POINTER(c_uint16), c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_bool]
juliaset.argtypes = [POINTER(c_uint16), c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double, c_double, c_bool]
lyapunov.argtypes = [POINTER(c_uint16), c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double]
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


def palette_load(palette, top_colors=4, lake_palette=False, lake=False):
    if (lake == False) or (lake_palette == False):
        palette = image_to_array(palette)
        unique_colors, counts = np.unique(palette, return_counts=True)
        del palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors = unique_colors[sorted_indices][:top_colors]
        return array_top_colors, False
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
        return array_top_colors, array_top_colors_lake




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






width = int(1600) # I'm using ratio 1/1
height = int(1600) #2304

# Number of iterations
max_iter = 1000

# Sandpile max grains
max_grains = 3

# You can generate different types of fractals
fractals = {
    'mandelbrot': True,
    'juliaset': True,
    'lyapunov': False,    # Lyapunov seems to run very slowly at high resolution try it with 1600x1600.
    'sandpile': False,     # Try sandpile with less resolution and much more iterations(=grains of sand) to get better results, but don't let the colored area touch the border or you will get broken results.
}


zoom = False
max_zoom = 20 # How many images # it's gonna generate  +n_coordinates more images than expected
per_zoom = 0.9 # Zooming after aiming: Using a value greater than 1.0 will zoom out; using a value less than 1.0 will zoom in
video_out = False # If you want to generate a video with the images


palette = "palette.png"  # Palette location
use_palette = False

# How many top colors to use from the palette.png
top_colors = 24
shift_palette = (0, 0)   # This shift the palette, you can set negative and positive integers.

# Julia set parameters
juliaset_c_real = -0.8
juliaset_c_imag = 0.16

# Makes the part that converges visible
lake = True
# Palette path to another palette image
lake_palette = "lake_palette.png"
# Here it's loading the palette before the generation and conversion
array_top_colors = palette_load(palette, top_colors, lake_palette, lake)


# Here you can move around 
xmin_xmax = np.array([(-(16/6)), ((16/6))], dtype=np.float64)     #-16/5, 16/5
ymin_ymax = np.array([-(16/6), (16/6)], dtype=np.float64)           #-9/5, 9/5



# This part is to help you aim
n_coordinates = 5   #  Number of coordinates to use, set False to not use it
#                       ([(column, line, grid nxn)])
coordinates = np.array([(1,2,3),(3,2,3),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)])

#coordinates = np.array([(1,1,3),(2,3,4),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)]) 
#coordinates = np.array([(3,3,3),(3,4,5),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)])

xmin, xmax, ymin, ymax = xmin_xmax[0], xmin_xmax[1], ymin_ymax[0], ymin_ymax[1]


# Uncomment the code below if you want to start at certain location
#xmin, xmax, ymin, ymax = divide_in_squares(coordinates[:(n_coordinates+1), :], xmin, xmax, ymin, ymax)


print("Your coordinates: ", xmin, xmax, ymin, ymax, "\n") 




def generate(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, n_iter, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette):

    array_top_colors = palette_load(palette, top_colors, lake_palette, lake)
    prefix = ""
    img_names = []


    if zoom:
        max_zoom = str(max_zoom)
        target_length = len(max_zoom)+1
        n_iter = str(n_iter)
        n_iter = n_iter.zfill(target_length)
        prefix = n_iter+"-"
        
    for key, value in fractals.items():
        # Mandelbrot Set
        if (key == "mandelbrot") and (value):
            gen_array = np.empty((height, width), dtype=np.uint16)
            start_time = time.perf_counter()
            fractal(gen_array.ctypes.data_as(POINTER(c_uint16)), width, height, max_iter, xmin, xmax, ymin, ymax, lake)
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        # Julia Set
        if (key == "juliaset") and (value):
            gen_array = np.empty((height, width), dtype=np.uint16)
            start_time = time.perf_counter()
            juliaset(gen_array.ctypes.data_as(POINTER(c_uint16)), width, height, max_iter, juliaset_c_real, juliaset_c_imag, xmin, xmax, ymin, ymax, lake)
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        # Lyapunov Set
        if (key == "lyapunov") and (value):
            gen_array = np.empty((height, width), dtype=np.uint16)
            start_time = time.perf_counter()
            lyapunov(gen_array.ctypes.data_as(POINTER(c_uint16)), width, height, max_iter, xmin, xmax, ymin, ymax)
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
                create_image(gen_array.reshape(width, height), "./images/" + prefix + "0" + localtime + "_colorful_"+key, max_iter, array_top_colors, lake, shift_palette)
                img_names.append("./images/" + prefix + "0" + localtime + "_colorful_"+key+".png")
            else:
                process_image(gen_array, (2**24-1), "./images/" + prefix + "0" + localtime + "_generated_fractal_"+key )
                img_names.append("./images/" + prefix + "0" + localtime + "_generated_fractal_"+key+".png")
            end_time = time.perf_counter()
            del gen_array
            print("Took ", end_time - start_time, "seconds to convert\n")
    return img_names

            
            
def generate_wrapper(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette):
    
    if zoom:
        assert fractals["sandpile"]== False, "Error: Can't zoom on sandpile."
        # The first image generated
        generate(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, 0, n_coordinates+max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette)
        
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
        
            generate(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, i+1, n_coordinates+max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette)
    else:
            # Normal mode without zoom
            img_names = generate(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, 0, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette)
            return img_names








def imgs_to_video(n_coordinates):
    import os
    import subprocess
    import re
    
    image_folder = os.getcwd()
    fps = 10
    frac = ["colorful_mandelbrot", "colorful_juliaset", "colorful_lyapunov"]
    image_files = sorted([f for f in os.listdir(image_folder) if f.endswith('.png') and 'colorful' in f])
    
    for i, n in enumerate(frac):
        
        filtered_files = [f for f in image_files if n in f]

        pattern = re.compile(rf".*{re.escape(n)}\.png$")

        if any(pattern.match(f) for f in filtered_files): 
            
            with open('input.txt', 'w') as f:
                for index, image_file in enumerate(filtered_files):
                    duration = 0.9 if index < n_coordinates else 0.1
                    f.write(f"file '{image_file}'\n")
                    f.write(f"duration {duration}\n")
        
                f.write(f"file '{image_files[-1]}'\n")
        
            subprocess.run([
                'ffmpeg', '-f', 'concat', '-safe', '0', '-i', 'input.txt', 
                '-fps_mode', 'vfr', '-pix_fmt', 'yuv420p', '-vf', f'fps={fps}', f'video_{n}.mp4' 
            ])
        
            os.remove('input.txt')
        
            print(f'\nvideo_{n}.mp4 Video Done!')
            
            
if video_out:
    imgs_to_video(n_coordinates)


# received_params = {}
# stop_gen = False
import multiprocessing

received_params = {}
stop_gen = multiprocessing.Event()

def server(port):


    import http.server
    import socketserver
    import signal
    import sys
    import os



    PORT = port
    DIRECTORY = ""
    

    def process_form_data(params):

        fractals = {
            'mandelbrot': 'fractals[mandelbrot]' in params,
            'juliaset': 'fractals[juliaset]' in params,
            'lyapunov': 'fractals[lyapunov]' in params,
            'sandpile': 'fractals[sandpile]' in params
        }

        width = int(params.get('width', [0]))
        height = int(params.get('height', [0]))
        max_iter = int(params.get('max_iter', [0]))
        top_colors = int(params.get('top_colors', [0]))
        max_grains = int(params.get('max_grains', [0]))
        juliaset_c_real = float(params.get('juliaset_c_real', [0.0]))
        juliaset_c_imag = float(params.get('juliaset_c_imag', [0.0]))
        xmin = float(params.get('xmin', [0.0]))
        xmax = float(params.get('xmax', [0.0]))
        ymin = float(params.get('ymin', [0.0]))
        ymax = float(params.get('ymax', [0.0]))
        use_palette = 'use_palette' in params
        palette = params.get('palette', [''])
        lake = 'lake' in params
        lake_palette = params.get('lake_palette', [''])
        shift_palette = int(params.get('shift_palette'))
        shift_palette_lake = int(params.get('shift_palette_lake'))
        
        zoom = False
        max_zoom = 20


        if use_palette and not os.path.exists(palette):
            print(f"Palette file does not exist: {palette}")
            return
        if lake and not os.path.exists(lake_palette):
            print(f"Lake palette file does not exist: {lake_palette}")
            return

        print(f"Calling activate with parameters:\nfractals: {fractals}, width: {width}, height: {height}, max_iter: {max_iter}, top_colors: {top_colors}, max_grains: {max_grains}, juliaset_c_real: {juliaset_c_real}, juliaset_c_imag: {juliaset_c_imag}, xmin: {xmin}, xmax: {xmax}, ymin: {ymin}, ymax: {ymax}, use_palette: {use_palette}, palette: {palette}, lake: {lake}, lake_palette: {lake_palette}")
        paths = generate_wrapper(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax, [shift_palette, shift_palette_lake])
        
        print((paths))
        return paths



    class MyHttpRequestHandler(http.server.SimpleHTTPRequestHandler):

        def do_POST(self):
            global received_params, stop_signal

            # Get the content type and boundary from headers
            content_type = self.headers.get('Content-Type')
            if not content_type:
                self.send_response(400)
                self.end_headers()
                return

            # Extract boundary from content type
            boundary = None
            for part in content_type.split(';'):
                if 'boundary=' in part:
                    boundary = part.split('=')[1].strip()
                    break
            
            if not boundary:
                self.send_response(400)
                self.end_headers()
                return

            # Read the entire body of the request
            content_length = int(self.headers['Content-Length'])
            body = self.rfile.read(content_length)

            # Parse the multipart form data
            parts = body.split(f'--{boundary}'.encode())
            for part in parts:
                if part.strip() and not part.startswith(b'--'):
                    # Extract headers and body from part
                    headers, payload = part.split(b'\r\n\r\n', 1)
                    headers = headers.decode('utf-8')
                    payload = payload.rstrip(b'--').decode('utf-8')

                    # Extract form field name
                    disposition = [h for h in headers.split('\r\n') if h.startswith('Content-Disposition:')]
                    if disposition:
                        content_disposition = disposition[0]
                        field_name = None
                        for param in content_disposition.split(';'):
                            if 'name=' in param:
                                field_name = param.split('=')[1].strip('" ')
                                break
                        if field_name:
                            received_params[field_name] = payload.strip()

            required_keys = ['width', 'height', 'max_iter', 'top_colors', 'max_grains', 'juliaset_c_real', 'juliaset_c_imag', 'xmin', 'xmax', 'ymin', 'ymax', 'palette', 'lake_palette']
            if all(key in received_params for key in required_keys):
                print(received_params)
                fractal_result = ",".join(process_form_data(received_params))
                received_params.clear()

                self.send_response(200)
                self.send_header('Content-type', 'application/json')
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
        print(f"Fractal Generator running at: http://localhost:{PORT}")
        httpd.serve_forever()





def main():
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
        generate_wrapper(fractals, width, height, top_colors, max_grains, juliaset_c_real, juliaset_c_imag, use_palette, palette, lake, lake_palette, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax, shift_palette)
        # n_coordinates is how many times it will use the array coordinates.
        if video_out:
            imgs_to_video(n_coordinates)

    else:

        with open('port.txt', 'w') as file:
            file.write(str(args.port))

        server(args.port)


if __name__ == "__main__":
    main()
