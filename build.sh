#!/bin/bash

# Default Settings
MODE="CPU"
PRECISION="DOUBLE"
MATH="QUAT"
SHARE="OFF"

show_help() {
    echo "Fractal Viewer Build Script"
    echo "Usage: ./build.sh [options]"
    echo "--cuda      Compile for GPU (defaults to float)"
    echo "--cpu       Compile for CPU (defaults to double)"
    echo "--float     Use 32-bit floats"
    echo "--double    Use 64-bit doubles"
    echo "--octo      Use Octonion math"
    echo "--share     Portable binary (disables march=native)"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --cuda)   MODE="CUDA"; PRECISION="FLOAT"; shift ;;
        --cpu)    MODE="CPU"; PRECISION="DOUBLE"; shift ;;
        --float)  PRECISION="FLOAT"; shift ;;
        --double) PRECISION="DOUBLE"; shift ;;
        --octo)   MATH="OCTO"; shift ;;
        --quat)   MATH="QUAT"; shift ;;
        --share)  SHARE="ON"; shift ;;
        -h|--help) show_help ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [ ! -f "libSDL3.so" ]; then
    if [ ! -f "sdl/build/libSDL3.so" ]; then
        echo "SDL3 Shared not found. Building..."
        mkdir -p sdl/build && cd sdl/build
        cmake .. -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        cd ../..
    fi
    # -L copies the actual library data instead of a link, fixing the "No such file" error
    cp -L sdl/build/libSDL3.so* .
fi

# 2. Setup Flags
CXX_FLAGS="-std=c++20 -O3 -I imgui -I sdl/include -I sdl/include/SDL3"
DEFS=""
[[ "$MODE" == "CUDA" ]] && DEFS+="-DUSE_CUDA "
[[ "$PRECISION" == "FLOAT" ]] && DEFS+="-DUSE_FLOAT "
[[ "$MATH" == "OCTO" ]] && DEFS+="-DOCTO "
OMP_FLAGS=""
if [ "$MODE" == "CPU" ]; then OMP_FLAGS="-fopenmp"; fi

if [ "$SHARE" == "OFF" ]; then
    CXX_FLAGS+=" -march=native"
fi
if [ "$SHARE" == "ON" ]; then
    LDFLAGS+=" -static-libstdc++ -static-libgcc"
fi
rm -f *.o

# 3. Compile
g++ $CXX_FLAGS $OMP_FLAGS $DEFS -c \
    fractal_ui.cpp \
    lodepng.cpp \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    imgui/imgui_demo.cpp \
    imgui/backends/imgui_impl_sdl3.cpp \
    imgui/backends/imgui_impl_sdlrenderer3.cpp

if [ "$MODE" == "CUDA" ]; then
    nvcc -c fract_kernel.cu -O3 $DEFS \
    -gencode arch=compute_75,code=sm_75 \
    -gencode arch=compute_86,code=sm_86 \
    -Xcompiler -fPIC
fi

# 4. Link - RPATH set to '$ORIGIN' (current folder)
# 4. Link
LIBS="-lSDL3 -lm -lpthread"
LDFLAGS="-L. -Wl,-rpath,\$ORIGIN"

if [ "$MODE" == "CUDA" ]; then
    g++ *.o -o fractal_viewer -L/usr/local/cuda/lib64 $LDFLAGS -lcudart $LIBS
else
    g++ *.o -o fractal_viewer $OMP_FLAGS $LDFLAGS $LIBS
fi

echo "Build complete: ./fractal_viewer (libSDL3.so copied to the current folder)"