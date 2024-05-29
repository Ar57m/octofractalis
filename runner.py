import numpy as np
import time
import cv2
from ctypes import cdll, c_double, POINTER, c_uint32, c_uint16, c_uint8, c_bool, c_float #, c_longdouble



# Carregue a biblioteca
lib = cdll.LoadLibrary('./libfract.so')


fractal = lib.fractal
juliaset = lib.juliaset
lyapunov = lib.lyapunov
sandpile = lib.sandpile

lib.process_array.argtypes = [POINTER(c_uint32), POINTER(c_uint8), c_uint16, c_uint16, c_double, c_uint16, c_double]
lib.process_array.restype = None
lib.scale.argtypes = [POINTER(c_float), POINTER(c_float), c_uint32, c_float, c_float] 
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
        output_array = cv2.resize(output_array, (width*4, height*4), interpolation=cv2.INTER_NEAREST)
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
def create_image(palette, data, filename, iterations, array_top_colors, lake=False):
    data = data.copy().astype(np.uint32)
    shape = data.shape
    shape = (shape[1], shape[0])
    data = data.reshape(shape)
    
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






width = int(4096) # I'm using ratio 1/1
height = int(4096) #2304

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
max_zoom = 5 # How many images # it's gonna generate  +n_coordinates more images than expected
per_zoom = 0.9  # How much zoom after aiming


palette = "palette.png"
use_palette = True

# How many top colors to use from the palette.png
top_colors = 24

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
xmin_xmax = np.array([(-(16/6)), ((16/6))], dtype=np.float64)        #-16/5, 16/5
ymin_ymax = np.array([-(16/6), (16/6)], dtype=np.float64)             #-9/5, 9/5



# n_squares is a grid 7x7 to help you aim
#                       ([(column, line, grid nxn)])
coordinates = np.array([(1,2,3),(3,2,3),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)])  #np.array([(1,2,3),(2,2,3),(2,2,3),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2, 3)])
#coordinates = np.array([(1,1,3),(2,3,4),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)]) 

#coordinates = np.array([(3,3,3),(3,4,5),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)]) 
xmin, xmax, ymin, ymax = xmin_xmax[0], xmin_xmax[1], ymin_ymax[0], ymin_ymax[1]
#xmin, xmax, ymin, ymax = divide_in_squares(coordinates[:2, :], xmin, xmax, ymin, ymax)
print("Your coordinates: ", xmin, xmax, ymin, ymax, "\n") 




def generate(zoom, n_iter, max_zoom, max_iter, xmin, xmax, ymin, ymax):

    prefix = ""
    
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
            if use_palette:
                create_image(palette, gen_array.reshape(width, height), prefix + "colorful_"+key, max_iter, array_top_colors, lake)
            else:
                process_image(gen_array, (2**24-1), prefix + "generated_fractal_"+key )
            end_time = time.perf_counter()
            del gen_array
            print("Took ", end_time - start_time, "seconds to convert\n")

            
            
def generate_wrapper(n_coordinates, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax):
    
    if zoom:
        assert fractals["sandpile"]== False, "Error: Can't zoom on sandpile."
        # The first image generated
        generate(zoom, 0, n_coordinates+max_zoom, max_iter, xmin, xmax, ymin, ymax)
        
        xmin1, xmax1, ymin1, ymax1 =  xmin, xmax, ymin, ymax
        
        
        
        for i in range(n_coordinates+max_zoom): 
            if i < n_coordinates:
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
        
            generate(zoom, i+1, n_coordinates+max_zoom, max_iter, xmin, xmax, ymin, ymax)
    else:
            # Normal mode without zoom
            generate(zoom, 0, max_zoom, max_iter, xmin, xmax, ymin, ymax)




# Let's Run
generate_wrapper(5, zoom, max_zoom, max_iter, xmin, xmax, ymin, ymax)
# n_coordinates is how many times it will use the array coordinates.



