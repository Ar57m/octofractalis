import numpy as np
from PIL import Image
#%pip install cython
import cython
from fractal_generator import fractal, julia_set


# Here I'm using 2 functions of my other project random_tools on github
def scale( input_tensor, new_min, new_max):
        current_min = np.min(input_tensor)
        current_max = np.max(input_tensor)
        if current_min == current_max: # to avoid infinity
           current_min -= 1
        scaled_tensor = (input_tensor - current_min) * (new_max - new_min) / (current_max - current_min) + new_min
        return scaled_tensor


def tensor_to_image(tensor, imgname, newmin= None, newmax= None):
        shape = tensor.shape
        array_image = np.nan_to_num(tensor, nan=0, posinf=1, neginf=-1)
        del tensor
        if (newmin == None) or (newmax == None):
            array_image = scale(array_image, 0, 16777215)
        else:
            array_image = scale(array_image, newmin, newmax)
        array_image = np.round(array_image).astype(np.int64)
        hexa =None

        hexa = array_image
        del array_image
    
        hexa_flat = hexa.reshape(-1)
        del hexa
        cor_rgb_flat = np.zeros((hexa_flat.size, 3), dtype=np.int64)
        cor_rgb_flat[:, 0] = (hexa_flat >> 16) & 0xFF
        cor_rgb_flat[:, 1] = (hexa_flat >> 8) & 0xFF
        cor_rgb_flat[:, 2] = hexa_flat & 0xFF
      
        del hexa_flat
    
        cor_rgb = cor_rgb_flat.reshape(shape[0], shape[1], 3).astype(np.uint8)
        del cor_rgb_flat 
        #oo = np.full((sh[0], sh[1], 1), 255, dtype=np.uint8)
        #cor_rgb = np.concatenate((cor_rgb, oo), axis=2)
    
        imag = Image.fromarray(cor_rgb)
        del cor_rgb
        imag.save(f'{imgname}.png')
        print(f'and {imgname}.png' )



def create_image(data, filename):
    data = np.nan_to_num(data, nan=0, posinf=1, neginf=-1)
    data = np.round(scale(data, 0, 255))
    normalized_data = data
    image = Image.fromarray(normalized_data.astype(np.uint8))

    image.save(filename)
    print(f"Images Saved to: {filename}")

# To do: I need a better of selecting the colors


width = 4096 # I'm using ratio 16/9
height = 2304

# Here you can move around 
xmin, xmax = -16/5, 16/6
ymin, ymax = -9/5, 9/5

max_iterations = 1000
mandelbrot_set = fractal(width, height, xmin, xmax, ymin, ymax, max_iterations)


create_image(mandelbrot_set.T, "blackandwhite.png")
tensor_to_image(mandelbrot_set.T,"generated_fractal", 0,2**24-1)

julia_set = julia_set(width, height, -0.8, 0.16, xmin, xmax, ymin, ymax, max_iterations)

create_image(julia_set.T, "blackandwhite_julia_set.png")
tensor_to_image(julia_set.T,"generated_fractal_julia_set", 0,2**24-1)






