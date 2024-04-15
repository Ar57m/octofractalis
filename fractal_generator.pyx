#cython: language_level=3str


import numpy as np

cimport numpy as np
cimport cython






cdef extern from "math.h":
    double sin(double x)
    double cos(double x)
    double fmod(double x, double y)
    double sinh(double x)
    double cosh(double x)
    double tanh(double x)
    double tan(double x)
    double log(double x)
    double log10(double x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double sin_cython(double x):
    return sin(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double cos_cython(double x):
    return cos(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double mod_cython(double x, double y):
    return fmod(x, y)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double sinh_cython(double x):
    return sinh(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double cosh_cython(double x):
    return cosh(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double tanh_cython(double x):
    return tanh(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double tan_cython(double x):
    return tan(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double logn_cython(double x):
    return log(x)

@cython.boundscheck(False)
@cython.wraparound(False)
cpdef double log_cython(double x):
    return log10(x)
 


@cython.boundscheck(False)
@cython.wraparound(False)
cpdef fractal(int width, int height, double xmin=-2, double xmax=2, double ymin=-2, double ymax=2, int max_iter=100):
    
    cdef np.ndarray[np.uint64_t, ndim=2] mandelbrot_set = np.zeros((width, height), dtype=np.uint64)
    cdef double dx = (xmax - xmin) / width
    cdef double dy = (ymax - ymin) / height
    cdef double r_part, i_part, z_real, z_imag, c_real, c_imag, temp
    cdef int x, y, i
    
    for x in range(width):
        r_part = xmin + x * dx
        for y in range(height):
            i_part = ymin + y * dy
            c_real = r_part
            c_imag = i_part
            z_real = 0
            z_imag = 0
            for i in range(max_iter):
                if (z_real*z_real + z_imag*z_imag) > 4:
                    break
                #temp = (z_real**3) - (3*z_real*z_imag**2) + c_real  # z_real**3 - 3*z_real*z_imag**2 is the real part of (z_real + z_imag*i)**3
                #z_imag = (3*z_real**2*z_imag - z_imag**3) + c_imag  # 3*z_real**2*z_imag - z_imag**3 is the imaginary part of (z_real + z_imag*i)**3
                temp = (z_real**2) - (z_imag**2) +c_real 
                z_imag = 2*z_real*z_imag + c_imag
                z_real = temp
            mandelbrot_set[x, y] = i

    return mandelbrot_set


@cython.boundscheck(False)
@cython.wraparound(False)
cpdef julia_set(int width, int height, double c_real, double c_imag, double xmin=-2, double xmax=2, double ymin=-2, double ymax=2, int max_iter=100):
    
    cdef np.ndarray[np.uint64_t, ndim=2] julia_set = np.zeros((width, height), dtype=np.uint64)
    cdef double dx = (xmax - xmin) / width
    cdef double dy = (ymax - ymin) / height
    cdef double r_part, i_part, z_real, z_imag, temp
    cdef int x, y, i
    
    for x in range(width):
        r_part = xmin + x * dx
        for y in range(height):
            i_part = ymin + y * dy
            z_real = r_part
            z_imag = i_part
            for i in range(max_iter):
                if (z_real*z_real + z_imag*z_imag) > 4:
                    break
              #  temp = (z_real**3) - (3*z_real*z_imag**2) + c_real
               # z_imag = (3*z_real**2*z_imag - z_imag**3) + c_imag
                temp = (z_real**2) - (z_imag**2) +c_real 
                z_imag = 2*z_real*z_imag + c_imag
                z_real = temp
            julia_set[x, y] = i

    return julia_set
