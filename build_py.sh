#!/bin/bash

find . -maxdepth 1 -type f -name '*.so' -exec rm {} \;

USE_CUDA=0
USE_F32=0

for arg in "$@"; do
    case "$arg" in
        --cuda) USE_CUDA=1 ;;
        --float)  USE_F32=1 ;;
    esac
done

if [ $USE_F32 -eq 1 ]; then
    F32_FLAG="-DUSE_FLOAT"
else
    F32_FLAG=""
fi


if [ $USE_CUDA -eq 1 ]; then
    echo "Compiling with CUDA support..."
    nvcc fract.cpp fract_kernel.cu --ptxas-options=-v -O3 -arch=sm_75 -Xcompiler -fPIC $F32_FLAG -DUSE_CUDA -shared -o libfract.so
else
    echo "Compiling for CPU only..."
    g++ -std=c++20 -O3 -Wall -Wextra -pedantic -march=native -fomit-frame-pointer -ffp-contract=fast -fPIC -funroll-loops -fopenmp $F32_FLAG -shared -o libfract.so fract.cpp
fi
