@echo off
setlocal enabledelayedexpansion

:: Default Settings
set MODE=CPU
set PRECISION=DOUBLE
set MATH=QUAT
set SHARE=OFF

:parse_args
if "%~1"=="" goto check_sdl
if /i "%~1"=="--cuda"   set MODE=CUDA& set PRECISION=FLOAT& shift & goto parse_args
if /i "%~1"=="--cpu"    set MODE=CPU& set PRECISION=DOUBLE& shift & goto parse_args
if /i "%~1"=="--float"  set PRECISION=FLOAT& shift & goto parse_args
if /i "%~1"=="--double" set PRECISION=DOUBLE& shift & goto parse_args
if /i "%~1"=="--octo"   set MATH=OCTO& shift & goto parse_args
if /i "%~1"=="--quat"   set MATH=QUAT& shift & goto parse_args
if /i "%~1"=="--share"  set SHARE=ON& shift & goto parse_args
if /i "%~1"=="-h"       goto help
if /i "%~1"=="--help"   goto help
echo Unknown option: %1 & exit /b 1

:help
echo Fractal Viewer Build Script
echo Usage: build.bat [options]
echo --cuda      Compile for GPU (defaults to float)
echo --cpu       Compile for CPU (defaults to double)
echo --float     Use 32-bit floats
echo --double    Use 64-bit doubles
echo --octo      Use Octonion math
echo --share     Portable binary (disables AVX2)
exit /b 0

:check_sdl
set "BUILD_SDL=0"
if not exist "SDL3.dll" set BUILD_SDL=1
if "!BUILD_SDL!"=="1" (
    if exist "sdl\build\Release\SDL3.dll" (
        copy /y "sdl\build\Release\SDL3.dll" . >nul
        set BUILD_SDL=0
    ) else if exist "sdl\build\SDL3.dll" (
        copy /y "sdl\build\SDL3.dll" . >nul
        set BUILD_SDL=0
    )
)
if "!BUILD_SDL!"=="1" (
    echo SDL3 DLL not found. Building...
    if not exist "sdl\build" mkdir "sdl\build"
    cd sdl\build
    cmake .. -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    cd ..\..
    if exist "sdl\build\Release\SDL3.dll" copy /y "sdl\build\Release\SDL3.dll" . >nul
    if exist "sdl\build\SDL3.dll" copy /y "sdl\build\SDL3.dll" . >nul
)

:: Construct Flags
set CXX_FLAGS=/nologo /std:c++20 /O2 /Ob3 /GL /openmp /fp:precise /fp:except- /GS- /Gw /Gy /Oi /Ot /EHsc /MT /I imgui /I sdl\include
set DEFS=
if "%MODE%"=="CUDA" set DEFS=%DEFS% -DUSE_CUDA
if "%PRECISION%"=="FLOAT" set DEFS=%DEFS% -DUSE_FLOAT
if "%MATH%"=="OCTO" set DEFS=%DEFS% -DOCTO
if "%SHARE%"=="OFF" ( set CXX_FLAGS=%CXX_FLAGS% /arch:AVX2 )

del *.obj 2>nul

:: Compile UI and Core
cl %CXX_FLAGS% %DEFS% fractal_ui.cpp /c

:: Compile Dependencies
cl %CXX_FLAGS% %DEFS% ^
lodepng.cpp ^
imgui\imgui.cpp ^
imgui\imgui_draw.cpp ^
imgui\imgui_tables.cpp ^
imgui\imgui_widgets.cpp ^
imgui\imgui_demo.cpp ^
imgui\backends\imgui_impl_sdl3.cpp ^
imgui\backends\imgui_impl_sdlrenderer3.cpp ^
/c

:: Compile CUDA
if "%MODE%"=="CUDA" (
    set NVCC_ARCH=-gencode arch=compute_75,code=sm_75 -gencode arch=compute_86,code=sm_86
    nvcc -c fract_kernel.cu -O3 %DEFS% !NVCC_ARCH! --ptxas-options=-v -Xcompiler "/MD /EHsc"
)

:: Link
set LIBS=SDL3.lib user32.lib gdi32.lib shell32.lib
set LIB_PATHS=/LIBPATH:"sdl\build"
if "%MODE%"=="CUDA" (
    set LIBS=!LIBS! cudart.lib
    set LIB_PATHS=!LIB_PATHS! /LIBPATH:"%CUDA_PATH%\lib\x64"
)

link /nologo /LTCG /OUT:fractal_viewer.exe *.obj %LIB_PATHS% %LIBS%
echo Build complete: fractal_viewer.exe (SDL3.dll copied to the current folder)