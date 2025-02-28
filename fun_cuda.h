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



// Wrapper for abs (for doubles, use fabs)
HOST_DEVICE inline double my_abs(double x) {
    return fabs(x);
}

// Wrapper for floor
HOST_DEVICE inline double my_floor(double x) {
    return floor(x);
}

// Wrapper for sin
HOST_DEVICE inline double my_sin(double x) {
    return sin(x);
}

// Wrapper for cos
HOST_DEVICE inline double my_cos(double x) {
    return cos(x);
}

// Wrapper for cosh
HOST_DEVICE inline double my_cosh(double x) {
    return cosh(x);
}

// Wrapper for sinh
HOST_DEVICE inline double my_sinh(double x) {
    return sinh(x);
}

// Wrapper for sqrt
HOST_DEVICE inline double my_sqrt(double x) {
    return sqrt(x);
}

// Wrapper for pow (note: two arguments)
HOST_DEVICE inline double my_pow(double x, double y) {
    return pow(x, y);
}

// Wrapper for atan2 (argument order: y, x)
HOST_DEVICE inline double my_atan2(double y, double x) {
    return atan2(y, x);
}

// Wrapper for log
HOST_DEVICE inline double my_log(double x) {
    return log(x);
}

// Wrapper for exp
HOST_DEVICE inline double my_exp(double x) {
    return exp(x);
}

// Wrapper for swap (swaps two double values)
HOST_DEVICE inline void my_swap(double &a, double &b) {
    double tmp = a;
    a = b;
    b = tmp;
}

// Wrapper for round
HOST_DEVICE inline double my_round(double x) {
    return round(x);
}

// Wrapper for tgamma (gamma function)
HOST_DEVICE inline double my_tgamma(double x) {
    return tgamma(x);
}

// Wrapper for fmod (floating-point modulo)
HOST_DEVICE inline double my_fmod(double x, double y) {
    return fmod(x, y);
}

HOST_DEVICE inline double my_max(double x, double y) {
    return x > y ? x : y;
}

#endif // CUDA_MATH_WRAPPER_H
