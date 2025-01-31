import numpy as np
import time

from ctypes import cdll, c_double, POINTER, c_int, c_uint32, c_uint16, c_uint8, c_bool, c_char_p
import argparse
import os
import re
import json

import tools


# Carregue a biblioteca
lib = cdll.LoadLibrary('./libfract.so')


fractal = lib.fractal
lyapunov = lib.lyapunov
newton = lib.newton
lorenz = lib.lorenz
sandpile = lib.sandpile
magnet = lib.magnet


fractal.argtypes = [POINTER(c_uint8), POINTER(c_int), POINTER(c_int), c_char_p,
    c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double,
    c_double, c_double, c_bool, c_bool, c_int, c_int, c_double, c_double, c_double,
    c_double, POINTER(c_double), c_uint32]

lyapunov.argtypes = [POINTER(c_uint8), POINTER(c_int), POINTER(c_int), c_char_p,
    c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double,
    c_double, c_double, c_double, c_double, c_int, c_int, POINTER(c_double), c_uint32]

newton.argtypes = [POINTER(c_uint8), POINTER(c_int), POINTER(c_int), c_char_p,
    c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double,
    c_double, c_bool, c_bool, c_int, c_int, c_double, c_double, c_double, c_double,
    c_double, POINTER(c_double), c_uint32]

lorenz.argtypes = [POINTER(c_uint8), POINTER(c_int), c_double, c_char_p,
    c_uint16, c_uint16, c_int, c_double, c_double, c_double, c_double, c_double, c_double,
    c_double, c_double, c_double, c_double, c_int, c_int, c_int, c_double, c_double,
    c_double, c_double, POINTER(c_double), c_uint32]

magnet.argtypes = [POINTER(c_uint8), POINTER(c_int), c_char_p,
    c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double, c_double,
    c_double, c_double, c_double, c_int, POINTER(c_double), c_uint32]

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






all_parameters = {



    'width' : int(1024), # I'm using ratio 1/1
    'height' : int(1024), #2304

    # Number of iterations
    'max_iter' : 400,
    'escape_radius': 0.0, 

    # Sandpile max grains
    'max_grains' : 4,


    # The equation
    'expression' : "z*z+c",         # z = "z^2 + c"

    # You can generate different types of fractals
    'fractals' : {
        'mandelbrot': True,
        'juliaset': False,
        'newton' : False,
        'newton_juliaset': False,
        'magnet': False,
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
    'gradient' : 16,        # Amount of colors between the colors

    # How many top colors to use from the palette.png
    'top_colors' : 24,
    'shift_palette' : 0,   # This shift the palette, you can set negative and positive integers.
    'shift_palette_lake' : 0,
    
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
    'axis': -1, #
    'max_point_size': 1, # to get the 3d effect, bigger points should be closer to the view

    #Lyapunov uses it as the imaginary part if juliaset is off
    'lyapunov_c_a' : 0.0,
    'lyapunov_c_b' : 0.0,


    # Quaternion parameters
    'quaternion_j' : 0.0,
    'quaternion_k' : 0.0,


    # Magnet Parameters
    'n_points'  : 3,
    'velocity_r' : 0.0,
    'velocity_i' : 0.0,

    # Makes the part that converges visible
    'lake' : True,
    # Palette path to another palette image
    'lake_palette' : "./palettes/lake_palette.png",
    # # Here it's loading the palette before the generation and conversion
    # 'array_top_colors' : palette_load(palette, gradient, top_colors, lake_palette, lake),



    # Here you can move around
    'xmin': -2.5,
    'xmax':  2.5,
    'ymin': -2.5,
    'ymax':  2.5,

    # For Lorenz
    'zmin': -1e30 * 1,
    'zmax':  1e30 * 1,

    # This part is to help you aim
    'n_coordinates' : 0,   #  Number of coordinates to use, set 0 to not use it
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

    'array_size': 1,
    
    'fractalize_image_path': False,
    

    'save_expressions': True,

}



imgfromvidfolder = all_parameters['imgfromvidfolder']
os.mkdir("./images/"+imgfromvidfolder) if len(imgfromvidfolder) != 0 and all_parameters['video_out'] else None


n_coordinates = all_parameters['n_coordinates']
if n_coordinates>0:
    coordinates = all_parameters['coordinates']
    all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'] = tools.divide_in_squares(coordinates[:(n_coordinates)][ :],
                                                                                                                       all_parameters["xmin"], all_parameters["xmax"],
                                                                                                                       all_parameters["ymin"], all_parameters["ymax"])

use_palette = all_parameters["use_palette"]
all_parameters['array_top_colors'] = tools.palette_load(all_parameters['palette'], all_parameters['gradient'], all_parameters['top_colors'],
                                                  all_parameters['lake_palette'], all_parameters['lake'], use_palette)


def generate(all_parameters):
    
    xmin, xmax, ymin, ymax = all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax']
    

    lake = all_parameters['lake']
    use_palette = all_parameters["use_palette"]
    prefix = ""
    img_names = []

    expression = all_parameters['expression']
    input_expression = expression

    expression = expression.replace(" ", "")
    
    

    if all_parameters['zoom']:
        max_zoom = str(all_parameters['max_zoom'])
        target_length = len(max_zoom)+1
        n_iter = str(all_parameters["n_iter"])
        n_iter = n_iter.zfill(target_length)
        prefix = n_iter+"-"
    else:
        array_top_colors = tools.palette_load(all_parameters['palette'], all_parameters['gradient'], all_parameters['top_colors'],
                                        all_parameters['lake_palette'], lake, use_palette)

    
 
    fractals = all_parameters["fractals"]
    width = all_parameters["width"]
    height = all_parameters["height"]
    max_iter = all_parameters["max_iter"]
    escape_radius = all_parameters["escape_radius"]
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
    shift_palette_lake = all_parameters["shift_palette_lake"]
    
    quaternion_j = all_parameters["quaternion_j"]
    quaternion_k = all_parameters["quaternion_k"]
    z_initial_r = all_parameters["z_initial_r"]
    z_initial_i = all_parameters["z_initial_i"]
    newton_epsilon = all_parameters["newton_epsilon"]

    velocity_r = all_parameters["velocity_r"]
    velocity_i = all_parameters["velocity_i"]
    n_points = all_parameters["n_points"]

    sigma = all_parameters["sigma"]
    rho = all_parameters["rho"]
    beta = all_parameters["beta"]
    dt = all_parameters["dt"]
    rotation_angle = all_parameters["rotation_angle"]
    axis = all_parameters["axis"]
    max_point_size = all_parameters["max_point_size"]
    
    array = tools.primes(all_parameters['array_size']) if not all_parameters['fractalize_image'] else tools.bw_image(all_parameters['fractalize_image'], width, height)

    array_top_colors = (
    np.roll(np.array(array_top_colors[0], dtype =np.int32), shift_palette),
    np.roll(np.array(array_top_colors[1], dtype =np.int32 ), shift_palette_lake)
    )
    array_top_colors_outside = array_top_colors[0]
    array_top_colors_lake = array_top_colors[1]
    
    expression = c_char_p(expression.encode('utf-8'))
    gen_array = False

    def save_img():
        localtime = time.strftime("%Y%m%d_%H%M%S", time.localtime())
        img_name = "./images/"+ all_parameters['imgfromvidfolder'] + prefix + "0" + localtime + "_colorful_"+key
        tools.create_image(gen_array, img_name)
        img_names.append(img_name+".png")


    for key, value in fractals.items():
        gen_array = np.zeros((height, width, 3), dtype=np.uint8)

     
        # Mandelbrot Set/Julia Set
        if ((key == "juliaset") or (key == "mandelbrot")) and (value):
            
            fractal(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                array_top_colors_lake.ctypes.data_as(POINTER(c_int)),
                expression, width, height, max_iter, xmin, xmax, ymin, ymax,
                juliaset_c_real, juliaset_c_imag, escape_radius, "juliaset" == key, lake, (array_top_colors_outside.shape[0])-1, 
                (array_top_colors_lake.shape[0])-1, quaternion_j, quaternion_k, z_initial_r, z_initial_i, array.ctypes.data_as(POINTER(c_double)), array.size
            )
            save_img()
            
            
        # Lyapunov Set
        if (key == "lyapunov") and (value):

            lyapunov(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                array_top_colors_lake.ctypes.data_as(POINTER(c_int)),
                expression, width, height, max_iter, xmin, xmax, ymin, ymax,
                lyapunov_c_a, lyapunov_c_b, quaternion_j, quaternion_k, escape_radius, (array_top_colors_outside.shape[0])-1, 
                (array_top_colors_lake.shape[0])-1, array.ctypes.data_as(POINTER(c_double)), array.size
            )
            save_img()

        # Newton Fractal   
        if ((key == "newton") or (key == "newton_juliaset")) and (value):

            newton(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                array_top_colors_lake.ctypes.data_as(POINTER(c_int)),
                expression, width, height, max_iter, xmin, xmax, ymin, ymax,
                juliaset_c_real, juliaset_c_imag, "newton_juliaset" == key, lake, (array_top_colors_outside.shape[0])-1, 
                (array_top_colors_lake.shape[0])-1, quaternion_j, quaternion_k, z_initial_r, z_initial_i, newton_epsilon,
                array.ctypes.data_as(POINTER(c_double)), array.size
            )
            save_img()


        # Lorenz Attractor / Lorenz system
        if ((key == "lorenz")) and (value):

            lorenz(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                rotation_angle,
                expression, width, height, max_iter, xmin, xmax, ymin, ymax,
                zmin, zmax, sigma, rho, beta, dt, (array_top_colors_outside.shape[0])-1, 
                axis, max_point_size, quaternion_j, quaternion_k, z_initial_r, z_initial_i,
                array.ctypes.data_as(POINTER(c_double)), array.size
            )
            save_img()


        # Magnet Pendulum Attractor
        if (key == "magnet") and (value):
            
            magnet(
                gen_array.ctypes.data_as(POINTER(c_uint8)), array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                expression, width, height, max_iter, xmin, xmax, ymin, ymax,
                velocity_r, velocity_i, escape_radius, quaternion_j, quaternion_k, n_points, array.ctypes.data_as(POINTER(c_double)), array.size
            )
            save_img()


        # Abelian Sandpile Fractal
        if (key == "sandpile") and (value):
            
            sandpile(gen_array.ctypes.data_as(POINTER(c_uint8)),array_top_colors_outside.ctypes.data_as(POINTER(c_int)),
                width, height, max_iter, (array_top_colors_outside.shape[0]), max_grains)
            save_img()

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
                xmin, xmax, ymin, ymax = tools.divide_in_squares(coordinates[:(i+1)][ :], xmin1, xmax1, ymin1, ymax1)
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
            
            




def main():
    global all_parameters
    
    parser = argparse.ArgumentParser(description="Process a JSON input and return the result.")
    parser.add_argument(
        "-json_data", 
        type=str, 
        help="JSON data to be processed."
    )
    parser.add_argument(
        "-returndict",
        action='store_true',
        help="Returns the base dict."
    )
    args = parser.parse_args()
    
    if args.returndict and not args.json_data:
        print(json.dumps(all_parameters))
    
    if not args.returndict: 
        try:
            data = json.loads(args.json_data)
            result = generate_wrapper(data)
            print(json.dumps(result))
        except (json.JSONDecodeError, Exception):
            print("failed_gen.png")

if __name__ == "__main__":
    main()
   
