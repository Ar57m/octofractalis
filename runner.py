import numpy as np
import time
import cv2
from ctypes import cdll, c_double, POINTER, c_int, c_uint32, c_uint16, c_uint8, c_bool, c_char_p
import argparse
import sys
import os
import re

# Carregue a biblioteca
lib = cdll.LoadLibrary('./libfract.so')


fractal = lib.fractal
lyapunov = lib.lyapunov
newton = lib.newton
lorenz = lib.lorenz
sandpile = lib.sandpile



fractal.argtypes = [POINTER(c_uint8), POINTER(c_int), POINTER(c_int), POINTER(c_double),
    c_char_p, c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double,
    c_double, c_bool, c_bool, c_int, c_int, c_double, c_double, c_double, c_double]

lyapunov.argtypes = [POINTER(c_uint8), POINTER(c_int), POINTER(c_int), POINTER(c_double),
    c_char_p, c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double,
    c_double, c_double, c_double, c_int, c_int]

newton.argtypes = [POINTER(c_uint8), POINTER(c_int), POINTER(c_int), POINTER(c_double),
    c_char_p, c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double,
    c_double, c_bool, c_bool, c_int, c_int, c_double, c_double, c_double, c_double, c_double]

lorenz.argtypes = [POINTER(c_uint8), POINTER(c_int), c_double, POINTER(c_double),
    c_char_p, c_uint16, c_uint16, c_int, c_double, c_double, c_double, c_double, c_double, c_double,
    c_double, c_double, c_double, c_double, c_int, c_int, c_int, c_double, c_double, c_double, c_double]

sandpile.argtypes = [POINTER(c_uint8), POINTER(c_int), c_uint16, c_uint16, c_uint32, c_int, c_uint16]




def write_to_file(file_name, text):
    try:
        # Open the file in append mode
        with open(file_name, 'a+', encoding='utf-8') as file:
            file.seek(0)  # Move to the start of the file
            content = file.read()
            if content:  # Check if the file has content
                file.write('\n')  # Add a newline if it's not empty
            file.write(text)
    except IOError as e:
        print(f"An error occurred: {e}")


def image_to_array(image_path, min=0, max=2**24-1):
        img = cv2.imread(image_path)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB) 
        array_image = np.array(img).astype(np.int32)
    
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
    
    
def palette_load(palette, gradient, top_colors=4, lake_palette=False, lake=False, use_palette=True):
    if (not lake) or (not lake_palette):
        array_top_colors = 0
        if use_palette:
            palette = image_to_array(palette)
            unique_colors, counts = np.unique(palette, return_counts=True)
            del palette
            sorted_indices = np.argsort(counts)[::-1]
            array_top_colors = unique_colors[sorted_indices][:top_colors]
        else:
            array_top_colors = np.linspace(0, 255, num=top_colors, dtype=np.int32)
            array_top_colors = (array_top_colors+array_top_colors*256+array_top_colors*256**2)
        return generate_gradient(array_top_colors, gradient), np.array([0], dtype=np.int32)
    else:
        if not use_palette:
            array_top_colors = np.linspace(0, 255, num=top_colors, dtype=np.int32)
            array_top_colors = (array_top_colors+array_top_colors*256+array_top_colors*256**2)
            return generate_gradient(array_top_colors, gradient), generate_gradient(array_top_colors, gradient)
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




def create_image(data, filename):
    data = data.copy().astype(np.uint8)
    data = cv2.cvtColor(data, cv2.COLOR_BGR2RGB)
    
    if "sandpile" in filename:
        data = cv2.resize(data, (data.shape[0]*4, data.shape[1]*4), interpolation=cv2.INTER_NEAREST)
    
    cv2.imwrite(f'{filename}.png', data)
    print(f'{filename}.png' )

    
    
# This helps you to aim by dividing in squares(grid)
def divide_in_squares(list_c, xmin, xmax, ymin, ymax):
    # Convert the first two columns to zero-based indexing
    for i in range(len(list_c)):
        list_c[i][0] -= 1  # Subtract 1 from the `col` value
        list_c[i][1] -= 1  # Subtract 1 from the `line` value

    for col, line, n_squares in list_c:
        size_x = (xmax - xmin) / n_squares
        size_y = (ymax - ymin) / n_squares
        
        new_xmin = xmin + col * size_x
        new_xmax = xmin + (col + 1) * size_x
        new_ymin = ymin + line * size_y
        new_ymax = ymin + (line + 1) * size_y
        xmin, xmax, ymin, ymax = new_xmin, new_xmax, new_ymin, new_ymax
    
    return xmin, xmax, ymin, ymax



all_parameters = {



    'width' : int(1024), # I'm using ratio 1/1
    'height' : int(1024), #2304

    # Number of iterations
    'max_iter' : 400,

    # Sandpile max grains
    'max_grains' : 4,


    # The equation
    'expression' : "z^2+c",         # z = "z^2 + c"

    # You can generate different types of fractals
    'fractals' : {
        'mandelbrot': True,
        'juliaset': True,
        'newton' : False,
        'newton_juliaset': False,
        'lorenz' : False,      # Requires a bit of zoom out to it better and much more iterations than mandelbrot are recommended
        'lyapunov': False,    # Lyapunov seems to run very slowly at high resolution try it with 1600x1600.
        'sandpile': False,     # Try sandpile with less resolution and much more iterations(=grains of sand) to get better results, but don't let the colored area touch the border or you will get broken results.
    },


    'zoom' : False,
    'max_zoom' : 10, # How many images # it's gonna generate  +n_coordinates more images than expected
    'per_zoom' : 0.9, # Zooming after aiming: Using a value greater than 1.0 will zoom out; using a value less than 1.0 will zoom in
    'video_out' : False, # If you want to generate a video with the images using ffmpeg
    'imgfromvidfolder' : "",       # Folder to save all the imgs, it will be on ./images/yourfoldername try "imgs/"
    


    'palette' : "./palettes/palette.png",  # Palette location
    'use_palette' : True,
    'gradient' : 24,        # Amount of colors between the colors

    # How many top colors to use from the palette.png
    'top_colors' : 20,
    'shift_palette' : (0, 0),   # This shift the palette, you can set negative and positive integers.

    # Initial z for newton-based fractals and mandelbrot-based
    'z_initial_r' : 0.0,  # for newton use -1.0 and 0.0, for lorenz 0.0, 1.0 and the quaternion_j 1.05
    'z_initial_i' : 0.0,

    # Julia set parameters
    'juliaset_c_real' : -0.8,
    'juliaset_c_imag' : 0.16,

    # Newton epsilon for derivative
    'newton_epsilon' : 1e-6,

    # Lorenz Params
    'sigma' : 10.0,
    'rho' : 28.0,
    'beta' : 8/3,
    'dt' : 0.01,
    'rotation_angle': 0.0, #in degress
    'axis': -1, # 0 = X, 1 = Y, 2 = Z, anything else desactivated
    'max_point_size': 1, # to get the 3d effect, bigger points should be closer to the view

    #Lyapunov uses it as the imaginary part if juliaset is off
    'lyapunov_c_a' : 0.0,
    'lyapunov_c_b' : 0.0,


    # Quaternion parameters
    'quaternion_j' : 0.0,
    'quaternion_k' : 0.0,

    # Makes the part that converges visible
    'lake' : True,
    # Palette path to another palette image
    'lake_palette' : "./palettes/lake_palette.png",
    # # Here it's loading the palette before the generation and conversion
    # 'array_top_colors' : palette_load(palette, gradient, top_colors, lake_palette, lake),



    # Here you can move around
    'xmin': -2.5 * 1,
    'xmax':  2.5 * 1,
    'ymin': -2.5 * 1,
    'ymax':  2.5 * 1,

    # For Lorenz
    'zmin': -1e30 * 1,
    'zmax':  1e30 * 1,

    # This part is to help you aim
    'n_coordinates' : 1,   #  Number of coordinates to use, set 0 to not use it
    #                       ([(column, row, grid n*n)])
    'coordinates' : [
        [1, 2, 3],
        [2, 2, 3],
        [2, 1, 2],
        [1, 2, 3],
        [3, 3, 5],
        [2, 2, 3],
        [1, 2, 3],
        [2, 2, 3],
        [1, 2, 3],
        [2, 2, 3],
    ],

    'save_expressions': True,

}



imgfromvidfolder = all_parameters['imgfromvidfolder']
os.mkdir("./images/"+imgfromvidfolder) if len(imgfromvidfolder) != 0 and all_parameters['video_out'] else None


n_coordinates = all_parameters['n_coordinates']
if n_coordinates>0:
    coordinates = all_parameters['coordinates']
    all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'] = divide_in_squares(coordinates[:(n_coordinates)][ :],
                                                                                                                       all_parameters["xmin"], all_parameters["xmax"],
                                                                                                                       all_parameters["ymin"], all_parameters["ymax"])

use_palette = all_parameters["use_palette"]
all_parameters['array_top_colors'] = palette_load(all_parameters['palette'], all_parameters['gradient'], all_parameters['top_colors'],
                                                  all_parameters['lake_palette'], all_parameters['lake'], use_palette)





def generate(all_parameters):
    
    xmin, xmax, ymin, ymax = all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax']
    print("\nYour coordinates: ", xmin, xmax, ymin, ymax, "\n")

    lake = all_parameters['lake']
    use_palette = all_parameters["use_palette"]
    prefix = ""
    img_names = []

    expression = all_parameters['expression']
    input_expression = expression
    print(expression.replace(" ", ""))
    expression = re.sub(r'\bc\b', 'rw', re.sub(r'\bz\b', 'rt', expression)).replace(" ", "")
    array_top_colors = all_parameters['array_top_colors']
    

    if all_parameters['zoom']:
        max_zoom = str(all_parameters['max_zoom'])
        target_length = len(max_zoom)+1
        n_iter = str(all_parameters["n_iter"])
        n_iter = n_iter.zfill(target_length)
        prefix = n_iter+"-"
    else:
        array_top_colors = palette_load(all_parameters['palette'], all_parameters['gradient'], all_parameters['top_colors'],
                                        all_parameters['lake_palette'], lake, use_palette)

    
 
    fractals = all_parameters["fractals"]
    width = all_parameters["width"]
    height = all_parameters["height"]
    max_iter = all_parameters["max_iter"]
    max_grains = all_parameters["max_grains"]
    xmin = all_parameters["xmin"]
    xmax = all_parameters["xmax"]
    ymin = all_parameters["ymin"]
    ymax = all_parameters["ymax"]

    zmin = all_parameters["zmin"]
    zmax = all_parameters["zmax"]

    juliaset_c_real = all_parameters["juliaset_c_real"]
    juliaset_c_imag = all_parameters["juliaset_c_imag"]
    lyapunov_c_a = all_parameters["lyapunov_c_a"]
    lyapunov_c_b = all_parameters["lyapunov_c_b"]
    lake = all_parameters["lake"]

    shift_palette = all_parameters["shift_palette"]
    quaternion_j = all_parameters["quaternion_j"]
    quaternion_k = all_parameters["quaternion_k"]
    z_initial_r = all_parameters["z_initial_r"]
    z_initial_i = all_parameters["z_initial_i"]
    newton_epsilon = all_parameters["newton_epsilon"]

    sigma = all_parameters["sigma"]
    rho = all_parameters["rho"]
    beta = all_parameters["beta"]
    dt = all_parameters["dt"]
    rotation_angle = all_parameters["rotation_angle"]
    axis = all_parameters["axis"]
    max_point_size = all_parameters["max_point_size"]

    array_top_colors = (
    np.roll(array_top_colors[0], shift_palette[0]),
    np.roll(array_top_colors[1], shift_palette[1])
    )
    array_top_colors_outside = array_top_colors[0]
    array_top_colors_lake = array_top_colors[1]   


    for key, value in fractals.items():
        gen_array = np.zeros((height, width, 3), dtype=np.uint8)


        failed_gen = np.zeros((1), dtype=np.float64)
     
        # Mandelbrot Set/Julia Set
        if ((key == "juliaset") or (key == "mandelbrot")) and (value):
            
            start_time = time.perf_counter()
            fractal(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                array_top_colors_lake.ctypes.data_as(POINTER(c_int)), failed_gen.ctypes.data_as(POINTER(c_double)),
                c_char_p(expression.encode('utf-8')), width, height, max_iter, xmin, xmax, ymin, ymax,
                juliaset_c_real, juliaset_c_imag, "juliaset" == key, lake, (array_top_colors_outside.shape[0])-1, 
                (array_top_colors_lake.shape[0])-1, quaternion_j, quaternion_k, z_initial_r, z_initial_i
            )
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        # Lyapunov Set
        if (key == "lyapunov") and (value):
            
            start_time = time.perf_counter()
            lyapunov(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                array_top_colors_lake.ctypes.data_as(POINTER(c_int)), failed_gen.ctypes.data_as(POINTER(c_double)),
                c_char_p(expression.encode('utf-8')), width, height, max_iter, xmin, xmax, ymin, ymax,
                lyapunov_c_a, lyapunov_c_b, quaternion_j, quaternion_k, (array_top_colors_outside.shape[0])-1, 
                (array_top_colors_lake.shape[0])-1
            )
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")

        # Newton Fractal   
        if ((key == "newton") or (key == "newton_juliaset")) and (value):
            
            start_time = time.perf_counter()
            newton(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                array_top_colors_lake.ctypes.data_as(POINTER(c_int)), failed_gen.ctypes.data_as(POINTER(c_double)),
                c_char_p(expression.encode('utf-8')), width, height, max_iter, xmin, xmax, ymin, ymax,
                juliaset_c_real, juliaset_c_imag, "newton_juliaset" == key, lake, (array_top_colors_outside.shape[0])-1, 
                (array_top_colors_lake.shape[0])-1, quaternion_j, quaternion_k, z_initial_r, z_initial_i, newton_epsilon
            )
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")

        # Lorenz Attractor / Lorenz system
        if ((key == "lorenz")) and (value):
            
            start_time = time.perf_counter()
            lorenz(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                rotation_angle, failed_gen.ctypes.data_as(POINTER(c_double)),
                c_char_p(expression.encode('utf-8')), width, height, max_iter, xmin, xmax, ymin, ymax,
                zmin, zmax, sigma, rho, beta, dt, (array_top_colors_outside.shape[0])-1, 
                axis, max_point_size, quaternion_j, quaternion_k, z_initial_r, z_initial_i
            )
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")

        # Abelian Sandpile Fractal
        if (key == "sandpile") and (value):
            
            start_time = time.perf_counter()
            failed_gen[0] = 1.0
            sandpile(gen_array.ctypes.data_as(POINTER(c_uint8)),array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                width, height, max_iter, (array_top_colors_outside.shape[0]), max_grains)
            end_time = time.perf_counter()
            print("Took ", end_time - start_time, "seconds to generate")
            
            
        if value:
            imgfromvidfolder = all_parameters['imgfromvidfolder']
            start_time = time.perf_counter()
            print(failed_gen[0])
            localtime = time.strftime("%Y%m%d_%H%M%S", time.localtime())
            if failed_gen[0] > 0:
                create_image((gen_array), "./images/"+ imgfromvidfolder + prefix + "0" + localtime + "_colorful_"+key)
                img_names.append("./images/"+ imgfromvidfolder + prefix + "0" + localtime + "_colorful_"+key+".png")
            else:
                print("ERROR: Generation Failed.")
                img_names.append("./failed_gen.png")
            end_time = time.perf_counter()
            del gen_array
            print("Took ", end_time - start_time, "seconds to save\n")
    write_to_file("last_expressions.txt" , input_expression + ", " + ", ".join(img_names)) if all_parameters["save_expressions"] else None
    return img_names


            
def generate_wrapper(all_parameters):
    
    if all_parameters['zoom']:
        fractals = all_parameters['fractals']
        assert fractals["sandpile"] is False, "Error: Can't zoom on sandpile."
        # The first image generated
        n_coordinates = all_parameters['n_coordinates']
        max_zoom = all_parameters['max_zoom']
        all_parameters["n_iter"] = 0
        coordinates = all_parameters['coordinates']
        per_zoom = all_parameters['per_zoom']

        generate(all_parameters)

        xmin1, xmax1, ymin1, ymax1 =  all_parameters["xmin"], all_parameters["xmax"], all_parameters["ymin"], all_parameters["ymax"]
        xmin, xmax, ymin, ymax = xmin1, xmax1, ymin1, ymax1
        
    
        for i in range(n_coordinates+max_zoom): 
            if (i < n_coordinates) and (n_coordinates is not False):
                xmin, xmax, ymin, ymax = divide_in_squares(coordinates[:(i+1)][ :], xmin1, xmax1, ymin1, ymax1)
            else:
                
                x_center = (xmin + xmax) / 2
                y_center = (ymin + ymax) / 2
                
                width = (xmax - xmin) * per_zoom
                height = (ymax - ymin) * per_zoom
                
                xmin = x_center - width / 2
                xmax = x_center + width / 2
                ymin = y_center - height / 2
                ymax = y_center + height / 2

            all_parameters["n_iter"] = i+1
            all_parameters["xmin"] = xmin
            all_parameters["xmax"] = xmax
            all_parameters["ymin"] = ymin
            all_parameters["ymax"] = ymax

            generate(all_parameters)
    else:
            # Normal mode without zoom
            img_names = generate(all_parameters)
            return img_names








def imgs_to_video(all_parameters):
    import subprocess

    imgfromvidfolder = all_parameters['imgfromvidfolder']
    n_coordinates = all_parameters['n_coordinates']

    image_folder = "./images/" + imgfromvidfolder
    
    fps = 10
    frac = ["colorful_mandelbrot", "colorful_juliaset", "colorful_lyapunov"]
    image_files = sorted([f for f in os.listdir(image_folder) if f.endswith('.png') and re.search(r"\d+-", f) and 'colorful' in f])

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
#import multiprocessing


received_params = {}
# stop_gen = multiprocessing.Event()




def process_form_data(params):
    global all_parameters





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






    all_parameters['expression'] = params.get('expression', 'z*z+c')

    all_parameters['fractals'] = params.get("fractals", {
    'mandelbrot': False,
    'juliaset': False,
    'newton' : False,
    'newton_juliaset': False,
    'lyapunov': False,
    'sandpile': False,
    })

    all_parameters['width'] = int(params.get('width', 1024))
    all_parameters['height'] = int(params.get('height', 1024))
    all_parameters['max_iter'] = int(params.get('max_iter', 400))
    all_parameters['top_colors'] = int(params.get('top_colors', 24))
    all_parameters['max_grains'] = int(params.get('max_grains', 3))
    all_parameters['juliaset_c_real'] = float(params.get('juliaset_c_real', -0.8))
    all_parameters['juliaset_c_imag'] = float(params.get('juliaset_c_imag', 0.16))

    all_parameters['use_palette'] = bool(params.get('use_palette', True))
    palette = params.get('palette', './palettes/palette.png')
    palette = download_image(palette)
    all_parameters['palette'] = palette
    all_parameters['gradient'] = int(params.get('gradient', 16))
    all_parameters['lake'] = bool(params.get('lake', True))
    lake_palette = params.get('lake_palette', './palettes/lake_palette.png')
    lake_palette = download_image(lake_palette)
    all_parameters['lake_palette'] = lake_palette
    all_parameters['shift_palette'] = (int(params.get('shift_palette', 0)), int(params.get('shift_palette_lake',0)))

    grid_length = int(params.get('grid_length', 3))
    column_aim = min(int(params.get('column_aim', 2)), grid_length)
    row_aim = min(int(params.get('row_aim', 2)), grid_length)

    coordinates = [[column_aim,row_aim,grid_length]]
    all_parameters['column_aim'] = column_aim
    all_parameters['row_aim'] = row_aim
    all_parameters['grid_length'] = grid_length
    all_parameters['coordinates'] = coordinates
    all_parameters['continue_aim'] = bool(params.get('continue_aim', False))

    all_parameters['quaternion_j'] = float(params.get('quaternion_j', 0.0))
    all_parameters['quaternion_k'] = float(params.get('quaternion_k', 0.0))

    if all_parameters['continue_aim'] and all_parameters['grid_length'] != 1:
        xmin, xmax, ymin, ymax = divide_in_squares(coordinates, all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'])
        all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'] = xmin, xmax, ymin, ymax
    else:
        all_parameters['xmin'] = float(params.get('xmin', -2.7))
        all_parameters['xmax'] = float(params.get('xmax', 2.7))
        all_parameters['ymin'] = float(params.get('ymin', -2.7))
        all_parameters['ymax'] = float(params.get('ymax', 2.7))


    all_parameters["z_initial_r"]= float(params.get('z_initial_r', 0.0))
    all_parameters["z_initial_i"]= float(params.get('z_initial_i', 0.0))
    all_parameters["newton_epsilon"]= float(params.get('newton_epsilon', 0.000001))
    all_parameters["sigma"]= float(params.get('sigma', 10.0))
    all_parameters["rho"]= float(params.get('rho', 28.0))
    all_parameters["beta"]= float(params.get('beta', 2.66666666))
    all_parameters["dt"]= float(params.get('dt', 0.01))
    all_parameters["rotation_angle"] = float(params.get('rotation_angle', 0.0))
    all_parameters["axis"] = int(params.get('axis', -1))
    all_parameters["max_point_size"] = int(params.get('max_point_size', 1))

    #zoom = False
    #max_zoom = 20


    if all_parameters['use_palette'] and not os.path.exists(palette):
        print(f"Palette file does not exist: {palette}")
        return
    if all_parameters['lake'] and not os.path.exists(lake_palette):
        print(f"Lake palette file does not exist: {lake_palette}")
        return

    
    paths = generate_wrapper(all_parameters)
    
    #print((paths))
    return paths




def server(port):



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
    global all_parameters
    parser = argparse.ArgumentParser(description='Process some arguments.')

    # Add the --noserver flag
    parser.add_argument(
        '-noserver',
        action='store_true',
        help='If specified, the server will not be started.'
    )
    parser.add_argument(
        '-port',
        type=int,
        default=8888,
        help='Port to serve. Defaults to 8888 if not specified.'
    )
    parser.add_argument(
        '-e',
        type=str,
        help='Expression to use "z^2+c".'
    )
    parser.add_argument(
        '-d',
        action='store_true',
        help='Don\'t save expressions on last_expressions.txt'
    )

    args = parser.parse_args()
    all_parameters["save_expressions"] = not args.d

    
    if args.noserver:
        if args.e:
            all_parameters['expression'] = str(args.e)


        # Let's Run
        generate_wrapper(all_parameters)

        # for i in range(36):
        #     all_parameters['expression'] = f"rotation(z*z+c,pi/{36-(i+1)}, 1k)" #(1/35)*i
        #     time.sleep(1)


        # n_coordinates is how many times it will use the array coordinates.
        if all_parameters['video_out']:
            imgs_to_video(all_parameters)

    else:

        server(args.port)


if __name__ == "__main__":
    main()
