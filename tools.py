import cv2
import numpy as np


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
        return generate_gradient(array_top_colors, gradient).tolist(), np.array([0], dtype=np.int32).tolist()
    else:
        if not use_palette:
            array_top_colors = np.linspace(0, 255, num=top_colors, dtype=np.int32)
            array_top_colors = (array_top_colors+array_top_colors*256+array_top_colors*256**2)
            return generate_gradient(array_top_colors, gradient).tolist(), generate_gradient(array_top_colors, gradient).tolist()
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

        return generate_gradient(array_top_colors, gradient).tolist(), generate_gradient(array_top_colors_lake, gradient).tolist()




def create_image(data, filename):
    data = data.copy().astype(np.uint8)
    data = cv2.cvtColor(data, cv2.COLOR_BGR2RGB)
    
    if "sandpile" in filename:
        data = cv2.resize(data, (data.shape[0]*4, data.shape[1]*4), interpolation=cv2.INTER_NEAREST)
    
    cv2.imwrite(f'{filename}.png', data)
    #print(f'{filename}.png' )

    
    
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
   
  
 
 