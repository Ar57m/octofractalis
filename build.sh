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

# Platform Detection
OS="$(uname -s)"
if [ "$OS" == "Darwin" ]; then
    PLATFORM="MACOS"
    LIB_EXT="dylib"
    RPATH_ORIGIN="@executable_path"
else
    PLATFORM="LINUX"
    LIB_EXT="so"
    RPATH_ORIGIN="\$ORIGIN"
fi

show_help() {
    echo "Fractal Viewer Build Script"
    echo "Usage: ./build.sh [options]"
    # echo "--cuda      Compile for GPU (defaults to float)"
    echo "--cpu       Compile for CPU (defaults to double)"
    echo "--float     Use 32-bit floats"
    echo "--double    Use 64-bit doubles"
    echo "--share     Portable build (no -march=native, better CPU compatibility)"
    echo "--cli       Build CLI version (no SDL/ImGui)"
    exit 0
}

# ------------------------
# Parse args
# ------------------------
while [[ $# -gt 0 ]]; do
    case $1 in
        # --cuda)   MODE="CUDA"; PRECISION="FLOAT"; shift ;;
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
SDL_FOUND=false
if [ "$TARGET" == "UI" ]; then
    # 1. Try pkg-config first (Smartest way for macOS/Linux)
    if pkg-config --exists sdl3; then
        echo "SDL3 detected via pkg-config."
        SDL_FOUND=true
        SDL_CFLAGS=$(pkg-config --cflags sdl3)
        SDL_LDFLAGS=$(pkg-config --libs sdl3)
    # 2. Check local directory or build manually
    elif [ ! -f "libSDL3.$LIB_EXT" ]; then
        if [ ! -f "$LIBS/sdl/build/libSDL3.$LIB_EXT" ]; then
            echo "SDL3 not found. Building locally..."
            mkdir -p "$LIBS/sdl/build" && cd "$LIBS/sdl/build"
            cmake .. -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_BUILD_TYPE=Release
            JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
            make -j$JOBS
            cd - >/dev/null
        fi
        cp -L "$LIBS/sdl/build/libSDL3.$LIB_EXT"* .
        SDL_LDFLAGS="-L. -lSDL3"
        SDL_CFLAGS="-I$LIBS/sdl/include -I$LIBS/sdl/include/SDL3"
    else
        # Already exists in root
        SDL_LDFLAGS="-L. -lSDL3"
        SDL_CFLAGS="-I$LIBS/sdl/include -I$LIBS/sdl/include/SDL3"
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
    INCLUDES+=" -I $SRC/ext/imgui -I $SRC/ext/imgui/backends $SDL_CFLAGS"
    LDFLAGS="-L. -Wl,-rpath,$RPATH_ORIGIN"
fi

# [[ "$MODE" == "CUDA" ]] && DEFS+="-DUSE_CUDA "
[[ "$PRECISION" == "FLOAT" ]] && DEFS+="-DUSE_FLOAT "

if [ "$MODE" == "CPU" ]; then
    # macOS Clang needs different OpenMP handling if using Homebrew libomp
    if [ "$PLATFORM" == "MACOS" ]; then
        if [ -d "/opt/homebrew/opt/libomp" ]; then
            OMP_FLAGS="-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include"
            LDFLAGS+=" -L/opt/homebrew/opt/libomp/lib -lomp"
        else
            OMP_FLAGS="-fopenmp"
        fi
    else
        OMP_FLAGS="-fopenmp"
    fi
fi

if [ "$SHARE" == "OFF" ]; then
    # Optimize for THIS machine only
    CXX_FLAGS+=" -march=native"
else
    # Portable binaries
    CXX_FLAGS+=" -mtune=generic"
fi

if [ "$PLATFORM" == "LINUX" ] && [ "$TERMUX_VERSION" == "" ]; then
    if [ "$SHARE" == "ON" ]; then
        # More portable Linux binaries
        LDFLAGS+=" -static-libstdc++ -static-libgcc"
    fi
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
# if [ "$MODE" == "CUDA" ]; then
#     nvcc -c $SRC/gpu/octofractalis-kernel.cu -O3 $DEFS \
#         -gencode arch=compute_75,code=sm_75 \
#         -gencode arch=compute_86,code=sm_86 \
#         -Xcompiler -fPIC
#     mv *.o $BUILD 2>/dev/null
# fi

# ------------------------
# Link
# ------------------------
OBJ_FILES=$(ls $BUILD/*.o)

if [ "$TARGET" == "CLI" ]; then
    # if [ "$MODE" == "CUDA" ]; then
    #     g++ $OBJ_FILES -o octofractalis-cli \
    #         -L/usr/local/cuda/lib64 \
    #         $LDFLAGS -lcudart -lm -lpthread
    # else
        g++ $OBJ_FILES -o octofractalis-cli \
            $OMP_FLAGS $LDFLAGS -lm -lpthread
    # fi
    echo "Build complete: ./octofractalis-cli"
else
    # Use the discovered SDL LDFLAGS
    LIBS_LINK="$SDL_LDFLAGS -lm -lpthread"

    # if [ "$MODE" == "CUDA" ]; then
    #     g++ $OBJ_FILES -o octofractalis \
    #         -L/usr/local/cuda/lib64 \
    #         $LDFLAGS -lcudart $LIBS_LINK
    # else
        g++ $OBJ_FILES -o octofractalis \
            $OMP_FLAGS $LDFLAGS $LIBS_LINK
    # fi
    echo "Build complete: ./octofractalis"

fi