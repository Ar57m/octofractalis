import numpy as np
#from PIL import Image
#%pip install cython
#import cython
import time
import cv2
from ctypes import cdll, c_double, POINTER, c_uint32, c_uint16, c_uint8, c_bool, c_float #, c_longdouble



# Carregue a biblioteca
lib = cdll.LoadLibrary('./libfract.so')


fractal = lib.fractal
juliaset = lib.juliaset


lib.process_array.argtypes = [POINTER(c_uint32), POINTER(c_uint8), c_uint16, c_uint16, c_double, c_uint16, c_double]
lib.process_array.restype = None
lib.scale.argtypes = [POINTER(c_float), POINTER(c_float), c_uint32, c_float, c_float] 
lib.scale.restype = None

fractal.argtypes = [POINTER(c_uint16), c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_bool]
juliaset.argtypes = [POINTER(c_uint16), c_uint16, c_uint16, c_uint16, c_double, c_double, c_double, c_double, c_double, c_double, c_bool]



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
    return np.round((input.copy()/np.max(input))*max).astype(np.uint32)



# This can only scale positive numbers not negatives
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

    output_image = cv2.cvtColor(output_array.astype(np.uint8), cv2.COLOR_BGR2RGB) 
    
    del output_array
    cv2.imwrite(f'{imgname}.png', output_image) 
    print(f'{imgname}.png' )





def image_to_array(image_path, min=0, max=2**24-1):
        img = cv2.imread(image_path)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB) 
        array_image = np.array(img).astype(np.uint32)  #.astype(np.int64)
    
        if array_image.ndim != 3:
            array_image = np.array(img.convert("RGBA"))
            
        if array_image.ndim == 3:
              if array_image.shape[2] == 4:
                      array_image = array_image[:, :, :-1] #.astype(np.int64)
                      
              if array_image.shape[2] == 3:
                      array_image = (array_image[:, :, 0]*(256**2)+array_image[:, :, 1]*(256)+array_image[:, :, 2])
    
        #shape = array_image.shape
        #array_image = array_image.reshape(shape[0],shape[1])

        #array_image = np.clip(array_image , min, max)
        
        return array_image


def palette_load(palette, top_colors=4, lake_palette=False, lake=False):
    if (lake == False) or (lake_palette == False):
        palette = image_to_array(palette).astype(np.uint32)
        unique_colors, counts = np.unique(palette, return_counts=True)
        del palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors = unique_colors[sorted_indices][:top_colors]
        return array_top_colors, False
    else:
        palette = image_to_array(palette).astype(np.uint32)
        unique_colors, counts = np.unique(palette, return_counts=True)
        del palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors = unique_colors[sorted_indices][:top_colors]
        
        # Lake palette
        lake_palette = image_to_array(lake_palette).astype(np.uint32)
        unique_colors, counts = np.unique(lake_palette, return_counts=True)
        del lake_palette
        sorted_indices = np.argsort(counts)[::-1]
        array_top_colors_lake = unique_colors[sorted_indices][:top_colors]
        return array_top_colors, array_top_colors_lake




# Image with palette
def create_image(palette, data, filename, iterations, array_top_colors, lake=False):
    
    data = data.copy()
    shape = data.shape
    shape = (shape[1], shape[0])
    data = data.reshape(shape)
    
    if (lake == True) and (isinstance(array_top_colors[1], np.ndarray)) :
        data = np.where(data>iterations, np.round(scale((np.sin(data.astype(np.float32)).reshape(-1)) , iterations+1, iterations+array_top_colors[1].shape[0])).astype(np.uint32).reshape(shape), data) #.reshape(-1)
        for i, n in enumerate(array_top_colors[1]):
            data[data == i+iterations+1] = n+iterations
            
        
    #data = np.where(data<=iterations,np.round(scale(np.sin((data.astype(np.float64))) , 0, array_top_colors[0].shape[0]-1)).astype(np.uint32),data).reshape(-1)
    data = np.where(data<=iterations,(scale_fast(np.sin(data.astype(np.float32))+1, array_top_colors[0].shape[0]-1)),data).reshape(-1)
    for i, n in enumerate(array_top_colors[0]):
        data[data == i] = n

    process_image(data.reshape(shape), np.max(data), filename )


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



# Not working properly yet
def depth_to_intensity(rgb_image, depth_map):
    depth_map = (depth_map).astype(np.float64)
    normalized_depth = (1-(depth_map / np.max(depth_map)))
    
    intensity_image = rgb_image.astype(np.float64) * normalized_depth.reshape(rgb_image.shape[0],-1,1) #[:,np.newaxis]
    
    return np.round(intensity_image).astype(np.uint8)





width = int(4096) # I'm using ratio 1/1
height = int(4096) #2304

# Number of iterations
max_iter = 1000

mandelbrot_on = True

# Here you can move around 
xmin_xmax = np.array([((-16/6)), ((16/6))], dtype=np.float64)        #-16/5, 16/5
ymin_ymax = np.array([(-16/6), (16/6)], dtype=np.float64)             #-9/5, 9/5



# n_squares is a grid 7x7 to help you aim
#                       ([(column, line, grid nxn)])
coordinates = np.array([(1,2,3),(3,2,3),(1,2,3),(1,2,3),(3,3,5),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3)])  #np.array([(1,2,3),(2,2,3),(2,2,3),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2,3),(1,2,3),(2,2, 3)])

xmin, xmax, ymin, ymax = xmin_xmax[0], xmin_xmax[1], ymin_ymax[0], ymin_ymax[1]
xmin, xmax, ymin, ymax = divide_in_squares(coordinates[:5, :], xmin, xmax, ymin, ymax)

palette = "palette.png"
use_palette = True
# How many top colors to use from the palette.png
top_colors = 24

# Julia set parameters
juliaset_on = False
juliaset_c_real = -0.8
juliaset_c_imag = 0.16

# Makes the part that converges visible
lake = True
# Palette path to another palette image
lake_palette = "lake_palette.png"
# Here it's loading the palette before the generation and conversion
array_top_colors = palette_load(palette, top_colors, lake_palette, lake)

        
def activate(max_iter, xmin, xmax, ymin, ymax):

        
        
        # Mandelbrot Set
        if mandelbrot_on:
            start_time = time.perf_counter()
            
            mandelbrot_set = np.empty((height, width), dtype=np.uint16)
            fractal(mandelbrot_set.ctypes.data_as(POINTER(c_uint16)), width, height, max_iter, xmin, xmax, ymin, ymax, lake)
            
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            start_time = time.perf_counter()
            if use_palette:
                create_image(palette, mandelbrot_set.reshape(width, height), "colorful", max_iter, array_top_colors, lake)
            else:
                process_image(mandelbrot_set, (2**24-1), "generated_fractal" )
            end_time = time.perf_counter()
            print("Took ", end_time - start_time, "seconds to convert")
            
        
        # Julia Set
        if juliaset_on:
            start_time = time.perf_counter()
            
            julia_set = np.empty((height, width), dtype=np.uint16)
            juliaset(julia_set.ctypes.data_as(POINTER(c_uint16)), width, height, max_iter, juliaset_c_real, juliaset_c_imag, xmin, xmax, ymin, ymax, lake)
            
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to generate")
            
            start_time = time.perf_counter()
            if use_palette:
                create_image(palette, julia_set.reshape(width, height), "colorful_julia_set" , max_iter, array_top_colors, lake)
            else:
                process_image(julia_set, (2**24-1), "generated_fractal_julia_set")
            end_time = time.perf_counter()
            
            print("Took ", end_time - start_time, "seconds to convert")
            

# The first image generated
activate(max_iter, xmin, xmax, ymin, ymax)


        
        
        
        
        
