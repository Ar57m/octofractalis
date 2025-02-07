import cv2
import numpy as np




def primes(n):
    """Generate first n primes using Sieve of Eratosthenes."""
    if n < 1:
        return np.array([], dtype=np.float64)
    # Estimate sieve size more accurately
    if n <= 10:
        limit = 30
    else:
        limit = int(n * (np.log(n) + np.log(np.log(n))))  # Better approximation
    
    sieve = np.ones(limit, dtype=bool)
    sieve[0:2] = False
    sieve[4::2] = False  # Mark even numbers >2
    
    for start in range(3, int(np.sqrt(limit)) + 1, 2):
        if sieve[start]:
            sieve[start*start::start] = False
    primes = np.nonzero(sieve)[0]
    return primes[:n].astype(np.float64)


def bw_image(path, width, height):
    """Load image as grayscale and resize if needed."""
    array = cv2.imread(path)
    if array is None:
        raise ValueError(f"Image not found or invalid: {path}")
    
    if (array.shape[1], array.shape[0]) != (width, height):
        array = cv2.resize(array, (width, height), interpolation=cv2.INTER_AREA)
    
    
    return np.mean(array[..., :3], axis=2).reshape(-1).astype(np.float64)/255



def image_to_array(image_path, channel_return =False):
    """Convert image to RGB integer array."""
    img = cv2.imread(image_path)
    if img is None:
        raise ValueError(f"Image Not Found: {image_path}")
    
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    array_image = img[..., :3].astype(np.int32)  # Remove alpha channel if present
    if channel_return :
        return np.array(array_image)
    # Combine RGB channels into single integer
    return (array_image[..., 0] << 16) | (array_image[..., 1] << 8) | array_image[..., 2]


    
def get_top_colors(img_path, top_n, use_palette = True, levels=16):
    
    def quantize_color(color, levels):
        out = color.astype(np.float32)
        factor = (levels - 1) / 255.0
        inv_factor = 255.0 / (levels - 1)
        
        out = np.rint(np.rint(out * factor) * inv_factor).astype(np.int32)
        out = (out[..., 0] << 16) | (out[..., 1] << 8) | out[..., 2]
        return out
        
    if not use_palette:
            grays = np.linspace(0, 255, num=top_n, dtype=np.int32)
            return grays * 0x010101
    if not (levels == 256) :
        arr = image_to_array(img_path,True)
        # Quantize all colors
        quantized = quantize_color(arr, levels)
    else:
        quantized = image_to_array(img_path)
   
    unique, counts = np.unique(quantized, return_counts=True)
    return unique[np.argsort(-counts)[:top_n]]



def generate_gradient(arr, n_grad):
    """Generate color gradient between consecutive colors."""
    n_grad += 2
    if n_grad < 3:
        return arr
    
    r0, g0, b0 = (arr >> 16) & 0xFF, (arr >> 8) & 0xFF, arr & 0xFF
    arr = np.roll(arr, -1)
    
    r1, g1, b1 = (arr >> 16) & 0xFF, (arr >> 8) & 0xFF, arr & 0xFF
    
    dr, dg, db = r1 - r0, g1 - g0, b1 - b0
    steps = np.linspace(0, 1, n_grad - 1, endpoint=False)[:, np.newaxis]
    
    r = (r0 + dr * steps).astype(np.int32)
    g = (g0 + dg * steps).astype(np.int32)
    b = (b0 + db * steps).astype(np.int32)
    
    return ((r << 16) + (g << 8) + b).T.reshape(-1)



def palette_load(palette, gradient, top_colors=4, lake_palette=False, lake=False, use_palette=True, levels = 16):
    """Load color palettes with optional gradient generation."""
    
    main_colors = get_top_colors(palette, top_colors, use_palette, levels)
    
    if lake and lake_palette:
        lake_colors = get_top_colors(lake_palette, top_colors, use_palette, levels)
        return (
            generate_gradient(main_colors, gradient).tolist(),
            generate_gradient(lake_colors, gradient).tolist()
        )
    
    return generate_gradient(main_colors, gradient).tolist(), [0]



def create_image(data, filename):
    """Save image with optional upscaling for sandpile patterns."""
    data = data.astype(np.uint8)
    data = cv2.cvtColor(data, cv2.COLOR_BGR2RGB)
    
    if "sandpile" in filename:
        data = cv2.resize(data, (data.shape[1]*4, data.shape[0]*4), 
                         interpolation=cv2.INTER_NEAREST)
    
    cv2.imwrite(f'{filename}.png', data)



def divide_in_squares(list_c, xmin, xmax, ymin, ymax):
    """Recalculate boundaries based on grid divisions."""
    list_c = np.array(list_c) - [1, 1, 0]
    for col, line, n_squares in list_c:
        size_x = (xmax - xmin) / n_squares
        size_y = (ymax - ymin) / n_squares
        xmin, xmax = xmin + col * size_x, xmin + (col + 1) * size_x
        ymin, ymax = ymin + line * size_y, ymin + (line + 1) * size_y
    return xmin, xmax, ymin, ymax


