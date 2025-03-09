#!/bin/bash

# Remove old shared object files
find . -maxdepth 1 -type f -name '*.so' -exec rm {} \;

# Check for --cuda flag
if [[ "$1" == "--cuda" ]]; then
    echo "Compiling with CUDA support..."
    nvcc fract.cpp fract_kernel.cu -O3 -arch=sm_75 -Xcompiler -fPIC -DUSE_CUDA -shared -o libfract.so
else
    echo "Compiling for CPU only..."
    g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp -shared -o libfract.so fract.cpp
fi
