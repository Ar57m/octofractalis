#ifndef CUDA_MATH_WRAPPER_H
#define CUDA_MATH_WRAPPER_H






#ifdef USE_CUDA
    #define HOST_DEVICE __host__ __device__
    #include <cuda_runtime.h>

#else
    #define HOST_DEVICE
    #include <cmath>
    using namespace std;
#endif


#ifdef USE_FLOAT
    using DefaultType = float;
    constexpr DefaultType Max_flt = 1e38F;
#else
    using DefaultType = double;
    constexpr DefaultType Max_flt = 1e300;
#endif


// Wrapper for abs (for DefaultTypes, use fabs)
HOST_DEVICE inline DefaultType my_abs(DefaultType x) {
    return fabs(x);
}

// Wrapper for floor
HOST_DEVICE inline DefaultType my_floor(DefaultType x) {
    return floor(x);
}

// Wrapper for sin
HOST_DEVICE inline DefaultType my_sin(DefaultType x) {
    return sin(x);
}

// Wrapper for cos
HOST_DEVICE inline DefaultType my_cos(DefaultType x) {
    return cos(x);
}

// Wrapper for cosh
HOST_DEVICE inline DefaultType my_cosh(DefaultType x) {
    return cosh(x);
}

// Wrapper for sinh
HOST_DEVICE inline DefaultType my_sinh(DefaultType x) {
    return sinh(x);
}

// Wrapper for sqrt
HOST_DEVICE inline DefaultType my_sqrt(DefaultType x) {
    return sqrt(x);
}

// Wrapper for pow (note: two arguments)
HOST_DEVICE inline DefaultType my_pow(DefaultType x, DefaultType y) {
    return pow(x, y);
}

// Wrapper for atan2 (argument order: y, x)
HOST_DEVICE inline DefaultType my_atan2(DefaultType y, DefaultType x) {
    return atan2(y, x);
}

// Wrapper for log
HOST_DEVICE inline DefaultType my_log(DefaultType x) {
    return log(x);
}

// Wrapper for exp
HOST_DEVICE inline DefaultType my_exp(DefaultType x) {
    return exp(x);
}

// Wrapper for swap (swaps two DefaultType values)
HOST_DEVICE inline void my_swap(DefaultType &a, DefaultType &b) {
    DefaultType tmp = a;
    a = b;
    b = tmp;
}

// Wrapper for round
HOST_DEVICE inline DefaultType my_round(DefaultType x) {
    return round(x);
}

// Wrapper for tgamma (gamma function)
HOST_DEVICE inline DefaultType my_tgamma(DefaultType x) {
    return tgamma(x);
}

// Wrapper for fmod (floating-point modulo)
HOST_DEVICE inline DefaultType my_fmod(DefaultType x, DefaultType y) {
    return fmod(x, y);
}

HOST_DEVICE inline DefaultType my_max(DefaultType x, DefaultType y) {
    return x > y ? x : y;
}

#endif // CUDA_MATH_WRAPPER_H
