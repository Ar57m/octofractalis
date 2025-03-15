#!/bin/bash

find . -maxdepth 1 -type f -name '*.so' -exec rm {} \;

USE_CUDA=0
USE_F32=0
OCTO=0

for arg in "$@"; do
    case "$arg" in
        --cuda) USE_CUDA=1 ;;
        --f32)  USE_F32=1 ;;
        --octo)  OCTO=1 ;;
    esac
done

if [ $USE_F32 -eq 1 ]; then
    F32_FLAG="-DUSE_FLOAT"
else
    F32_FLAG=""
fi

if [ $OCTO -eq 1 ]; then
    OCTO_FLAG="-DOCTO"
else
    OCTO_FLAG=""
fi

if [ $USE_CUDA -eq 1 ]; then
    echo "Compiling with CUDA support..."
    nvcc fract.cpp fract_kernel.cu -O3 -arch=sm_75 -Xcompiler -fPIC $F32_FLAG $OCTO_FLAG -DUSE_CUDA -shared -o libfract.so
else
    echo "Compiling for CPU only..."
    g++ -O3 -Wall -Wextra -pedantic -march=native -fPIC -funroll-loops -fopenmp $F32_FLAG $OCTO_FLAG -shared -o libfract.so fract.cpp
fi