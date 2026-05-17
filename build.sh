#!/bin/bash

# Default Settings
MODE="CPU"
PRECISION="DOUBLE"
SHARE="OFF"
TARGET="UI"   # UI or CLI

show_help() {
    echo "Fractal Viewer Build Script"
    echo "Usage: ./build.sh [options]"
    echo "--cuda      Compile for GPU (defaults to float)"
    echo "--cpu       Compile for CPU (defaults to double)"
    echo "--float     Use 32-bit floats"
    echo "--double    Use 64-bit doubles"
    echo "--share     Portable binary (disables march=native)"
    echo "--cli       Build CLI version (no SDL/ImGui)"
    exit 0
}

# ------------------------
# Parse args
# ------------------------
while [[ $# -gt 0 ]]; do
    case $1 in
        --cuda)   MODE="CUDA"; PRECISION="FLOAT"; shift ;;
        --cpu)    MODE="CPU"; PRECISION="DOUBLE"; shift ;;
        --float)  PRECISION="FLOAT"; shift ;;
        --double) PRECISION="DOUBLE"; shift ;;
        --share)  SHARE="ON"; shift ;;
        --cli)    TARGET="CLI"; shift ;;
        -h|--help) show_help ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ------------------------
# SDL (ONLY for UI)
# ------------------------
if [ "$TARGET" == "UI" ]; then
    if [ ! -f "libSDL3.so" ]; then
        if [ ! -f "sdl/build/libSDL3.so" ]; then
            echo "SDL3 Shared not found. Building..."
            mkdir -p sdl/build && cd sdl/build
            cmake .. -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_BUILD_TYPE=Release
            JOBS=$(nproc 2>/dev/null || echo 4)
            make -j$JOBS
            cd ../..
        fi
        cp -L sdl/build/libSDL3.so* .
    fi
fi

# ------------------------
# Flags
# ------------------------
CXX_FLAGS="-std=c++20 -O3 -fomit-frame-pointer -ffp-contract=fast -funroll-loops -fno-math-errno -fno-trapping-math"
INCLUDES="-I lodepng"
DEFS=""
OMP_FLAGS=""
LDFLAGS=""

# Only UI needs these
if [ "$TARGET" == "UI" ]; then
    INCLUDES+=" -I imgui -I imgui/backends -I sdl/include -I sdl/include/SDL3"
    LDFLAGS="-L. -Wl,-rpath,\$ORIGIN"
fi

[[ "$MODE" == "CUDA" ]] && DEFS+="-DUSE_CUDA "
[[ "$PRECISION" == "FLOAT" ]] && DEFS+="-DUSE_FLOAT "

if [ "$MODE" == "CPU" ]; then
    OMP_FLAGS="-fopenmp"
fi

if [ "$SHARE" == "OFF" ]; then
    CXX_FLAGS+=" -march=native"
else
    LDFLAGS+=" -static-libstdc++ -static-libgcc"
fi

rm -f *.o

# ------------------------
# Compile
# ------------------------
if [ "$TARGET" == "CLI" ]; then

    echo "Building CLI..."

    g++ $CXX_FLAGS $OMP_FLAGS $DEFS $INCLUDES -c \
        fractal_cli.cpp \
        lodepng.cpp

else

    echo "Building UI..."

    g++ $CXX_FLAGS $OMP_FLAGS $DEFS $INCLUDES -c \
        fractal_ui.cpp \
        lodepng.cpp \
        imgui/imgui.cpp \
        imgui/imgui_draw.cpp \
        imgui/imgui_tables.cpp \
        imgui/imgui_widgets.cpp \
        imgui/imgui_demo.cpp \
        imgui/backends/imgui_impl_sdl3.cpp \
        imgui/backends/imgui_impl_sdlrenderer3.cpp

fi

# CUDA
if [ "$MODE" == "CUDA" ]; then
    nvcc -c fract_kernel.cu -O3 $DEFS \
        -gencode arch=compute_75,code=sm_75 \
        -gencode arch=compute_86,code=sm_86 \
        -Xcompiler -fPIC
fi

# ------------------------
# Link
# ------------------------
if [ "$TARGET" == "CLI" ]; then

    if [ "$MODE" == "CUDA" ]; then
        g++ *.o -o fractal_cli \
            -L/usr/local/cuda/lib64 \
            $LDFLAGS -lcudart -lm -lpthread
    else
        g++ *.o -o fractal_cli \
            $OMP_FLAGS $LDFLAGS -lm -lpthread
    fi

    echo "Build complete: ./fractal_cli"

else

    LIBS="-lSDL3 -lm -lpthread"

    if [ "$MODE" == "CUDA" ]; then
        g++ *.o -o fractal_viewer \
            -L/usr/local/cuda/lib64 \
            $LDFLAGS -lcudart $LIBS
    else
        g++ *.o -o fractal_viewer \
            $OMP_FLAGS $LDFLAGS $LIBS
    fi

    echo "Build complete: ./fractal_viewer"
fi