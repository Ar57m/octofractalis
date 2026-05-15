@echo off
setlocal enabledelayedexpansion

:: Default Settings
set MODE=CPU
set PRECISION=DOUBLE
set SHARE=OFF

:parse_args
if "%~1"=="" goto check_sdl
if /i "%~1"=="--cuda"   set MODE=CUDA& set PRECISION=FLOAT& shift & goto parse_args
if /i "%~1"=="--cpu"    set MODE=CPU& set PRECISION=DOUBLE& shift & goto parse_args
if /i "%~1"=="--float"  set PRECISION=FLOAT& shift & goto parse_args
if /i "%~1"=="--double" set PRECISION=DOUBLE& shift & goto parse_args
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
echo --share     Portable binary (disables AVX2)
exit /b 0

:check_sdl
setlocal EnableDelayedExpansion

set "SDL_FOUND=0"

echo ---- SDL SEARCH START ----

for /d %%D in (SDL3-*) do (
    echo Checking folder: %%D

    if exist "%%D\include\SDL3\SDL.h" (
        echo Found SDL headers in %%D

        set "SDL_DIR=%%D"
        set "SDL_FOUND=1"
        goto sdl_setup
    ) else (
        echo Missing: %%D\include\SDL3\SDL.h
    )
)

:sdl_setup
if "!SDL_FOUND!"=="1" (
    echo.
    echo SDL_DIR = "!SDL_DIR!"

    set "ARCH=%VSCMD_ARG_TGT_ARCH%"
    echo Raw ARCH from VS = "%VSCMD_ARG_TGT_ARCH%"

    if "!ARCH!"=="" (
        echo ARCH not set, defaulting to x64
        set "ARCH=x64"
    )

    echo Using ARCH = "!ARCH!"

    set "SDL_LIB_DIR=!SDL_DIR!\lib\!ARCH!"
    set "SDL_INCLUDE_DIR=!SDL_DIR!\include"

    echo SDL_LIB_DIR = "!SDL_LIB_DIR!"
    echo SDL_INCLUDE_DIR = "!SDL_INCLUDE_DIR!"

    echo Checking for SDL3.lib...
    if exist "!SDL_LIB_DIR!\SDL3.lib" (
        echo SDL3.lib FOUND
    ) else (
        echo SDL3.lib NOT FOUND
        dir "!SDL_LIB_DIR!"
        set SDL_FOUND=0
    )
)

echo ---- SDL SEARCH END ----
endlocal & (
    set "SDL_FOUND=%SDL_FOUND%"
    set "SDL_DIR=%SDL_DIR%"
    set "SDL_LIB_DIR=%SDL_LIB_DIR%"
    set "SDL_INCLUDE_DIR=%SDL_INCLUDE_DIR%"
)

if exist "!SDL_LIB_DIR!\SDL3.dll" copy /y "!SDL_LIB_DIR!\SDL3.dll" . >nul

:: Construct Flags
set CXX_FLAGS=/nologo /std:c++20 /O2 /Ob3 /GL /openmp /fp:precise /fp:except- /GS- /Gw /Gy /Oi /Ot /EHsc /MT ^
/I imgui ^
/I imgui\backends ^
/I lodepng\ ^
/I "!SDL_INCLUDE_DIR!"
set DEFS=
if "%MODE%"=="CUDA" set DEFS=%DEFS% -DUSE_CUDA
if "%PRECISION%"=="FLOAT" set DEFS=%DEFS% -DUSE_FLOAT
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
set LIB_PATHS=/LIBPATH:"!SDL_LIB_DIR!"
if "%MODE%"=="CUDA" (
    set LIBS=!LIBS! cudart.lib
    set LIB_PATHS=!LIB_PATHS! /LIBPATH:"%CUDA_PATH%\lib\x64"
)

link /nologo /LTCG /OUT:fractal_viewer.exe *.obj %LIB_PATHS% %LIBS%
echo Build complete: fractal_viewer.exe