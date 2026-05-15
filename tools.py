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
    """
    Vectorized gradient generator.
    - arr: array-like of 0xRRGGBB integers (list or ndarray)
    - n_grad: number of gradient steps BETWEEN bases (e.g. 5 -> base + 5 steps)
    Returns: 1D np.ndarray dtype=np.uint32 of length N*(n_grad+1)
    """
    arr = np.asarray(arr, dtype=np.uint32)
    if arr.size == 0:
        return arr.astype(np.uint32)
    if n_grad <= 0:
        # just return bases
        return arr.astype(np.uint32)

    N = arr.size
    S = int(n_grad)  # number of steps per segment

    # extract channels as floats for interpolation
    r0 = ((arr >> 16) & 0xFF).astype(np.float64)  # shape (N,)
    g0 = ((arr >> 8) & 0xFF).astype(np.float64)
    b0 = (arr & 0xFF).astype(np.float64)

    arr_next = np.roll(arr, -1)
    r1 = ((arr_next >> 16) & 0xFF).astype(np.float64)
    g1 = ((arr_next >> 8) & 0xFF).astype(np.float64)
    b1 = (arr_next & 0xFF).astype(np.float64)

    dr = (r1 - r0)[None, :]   # shape (1, N)
    dg = (g1 - g0)[None, :]
    db = (b1 - b0)[None, :]

    # fractional steps: step/(S+1) for step=1..S -> shape (S,1)
    frac = (np.arange(1, S + 1, dtype=np.float64) / (S + 1.0))[:, None]  # (S,1)

    # produce S x N arrays for each channel (float)
    r_steps = r0[None, :] + dr * frac    # shape (S, N)
    g_steps = g0[None, :] + dg * frac
    b_steps = b0[None, :] + db * frac

    # round-to-nearest and clip to [0,255], then cast to uint32
    # use np.rint (same behavior as earlier helper); cast to uint32 for safe bit ops
    r_steps_i = np.clip(np.rint(r_steps), 0, 255).astype(np.uint32)  # (S,N)
    g_steps_i = np.clip(np.rint(g_steps), 0, 255).astype(np.uint32)
    b_steps_i = np.clip(np.rint(b_steps), 0, 255).astype(np.uint32)

    # prepare matrices: (N, S+1) where col0 is base, cols 1..S are steps
    r_mat = np.empty((N, S + 1), dtype=np.uint32)
    g_mat = np.empty((N, S + 1), dtype=np.uint32)
    b_mat = np.empty((N, S + 1), dtype=np.uint32)

    r_mat[:, 0] = np.clip(np.rint(r0), 0, 255).astype(np.uint32)
    g_mat[:, 0] = np.clip(np.rint(g0), 0, 255).astype(np.uint32)
    b_mat[:, 0] = np.clip(np.rint(b0), 0, 255).astype(np.uint32)

    # fill cols 1..S with steps transposed
    r_mat[:, 1:] = r_steps_i.T
    g_mat[:, 1:] = g_steps_i.T
    b_mat[:, 1:] = b_steps_i.T

    # combine channels into a (N, S+1) uint32 matrix, then flatten row-major
    out_mat = (r_mat << np.uint32(16)) | (g_mat << np.uint32(8)) | b_mat
    return out_mat.reshape(-1)



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


