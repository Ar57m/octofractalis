#!/bin/bash

# ============================================================================
# PATHS
# ============================================================================
SRC="src"
BUILD="build"
LIBS="libs"

mkdir -p "$BUILD"

# Default Settings
MODE="CPU"
PRECISION="DOUBLE"
SHARE="OFF"
TARGET="UI"

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
# SDL (UI only)
# ------------------------
if [ "$TARGET" == "UI" ]; then
    if [ ! -f "libSDL3.so" ]; then
        if [ ! -f "$LIBS/sdl/build/libSDL3.so" ]; then
            echo "SDL3 not found. Building..."
            mkdir -p "$LIBS/sdl/build" && cd "$LIBS/sdl/build"
            cmake .. -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_BUILD_TYPE=Release
            JOBS=$(nproc 2>/dev/null || echo 4)
            make -j$JOBS
            cd - >/dev/null
        fi
        cp -L "$LIBS/sdl/build/libSDL3.so"* .
    fi
fi

# ------------------------
# Flags
# ------------------------
CXX_FLAGS="-std=c++20 -O3 -fomit-frame-pointer -ffp-contract=fast -funroll-loops -fno-math-errno -fno-trapping-math"
INCLUDES="-I $SRC -I $SRC/core -I $SRC/ext"
DEFS=""
OMP_FLAGS=""
LDFLAGS=""

if [ "$TARGET" == "UI" ]; then
    INCLUDES+=" -I $SRC/ext/imgui -I $SRC/ext/imgui/backends -I $LIBS/sdl/include -I $LIBS/sdl/include/SDL3"
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

rm -f $BUILD/*.o

# ------------------------
# Collect sources
# ------------------------

CORE_SRC=$(find $SRC/core -name "*.cpp")
EXT_SRC=$(find $SRC/ext -maxdepth 1 -name "*.cpp")

IMGUI_CORE=$(find $SRC/ext/imgui -maxdepth 1 -name "*.cpp")

IMGUI_BACKENDS="$SRC/ext/imgui/backends/imgui_impl_sdl3.cpp \
$SRC/ext/imgui/backends/imgui_impl_sdlrenderer3.cpp"

# ------------------------
# Compile
# ------------------------
if [ "$TARGET" == "CLI" ]; then

    echo "Building CLI..."

    SRC_FILES=(
        $(find $SRC/core -name "*.cpp")
        $(find $SRC/ext -maxdepth 1 -name "*.cpp")
        $SRC/app/octofractalis-cli.cpp
    )

    for file in "${SRC_FILES[@]}"; do
        obj="$BUILD/$(basename "${file%.cpp}.o")"
        g++ $CXX_FLAGS $OMP_FLAGS $DEFS $INCLUDES -c "$file" -o "$obj" || exit 1
    done

else

    echo "Building UI..."

    g++ $CXX_FLAGS $OMP_FLAGS $DEFS $INCLUDES -c \
        $CORE_SRC \
        $EXT_SRC \
        $IMGUI_CORE \
        $IMGUI_BACKENDS \
        $SRC/app/octofractalis-ui.cpp

    mv *.o $BUILD 2>/dev/null

fi

# ------------------------
# CUDA
# ------------------------
if [ "$MODE" == "CUDA" ]; then
    nvcc -c $SRC/gpu/octofractalis-kernel.cu -O3 $DEFS \
        -gencode arch=compute_75,code=sm_75 \
        -gencode arch=compute_86,code=sm_86 \
        -Xcompiler -fPIC
    mv *.o $BUILD 2>/dev/null
fi

# ------------------------
# Link
# ------------------------
OBJ_FILES=$(ls $BUILD/*.o)

if [ "$TARGET" == "CLI" ]; then

    if [ "$MODE" == "CUDA" ]; then
        g++ $OBJ_FILES -o octofractalis-cli \
            -L/usr/local/cuda/lib64 \
            $LDFLAGS -lcudart -lm -lpthread
    else
        g++ $OBJ_FILES -o octofractalis-cli \
            $OMP_FLAGS $LDFLAGS -lm -lpthread
    fi

    echo "Build complete: ./octofractalis-cli"

else

    LIBS_LINK="-lSDL3 -lm -lpthread"

    if [ "$MODE" == "CUDA" ]; then
        g++ $OBJ_FILES -o octofractalis \
            -L/usr/local/cuda/lib64 \
            $LDFLAGS -lcudart $LIBS_LINK
    else
        g++ $OBJ_FILES -o octofractalis \
            $OMP_FLAGS $LDFLAGS $LIBS_LINK
    fi

    echo "Build complete: ./octofractalis"
fi