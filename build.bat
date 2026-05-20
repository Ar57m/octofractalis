@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: PATH SETUP
:: ============================================================================
set SRC=src
set BUILD=build
set LIBS=libs

if not exist %BUILD% mkdir %BUILD%

:: Default Settings
set MODE=CPU
set PRECISION=DOUBLE
set SHARE=OFF
set TARGET=UI

:parse_args
if "%~1"=="" goto check_target
if /i "%~1"=="--cuda"   set MODE=CUDA& set PRECISION=FLOAT& shift & goto parse_args
if /i "%~1"=="--cpu"    set MODE=CPU& set PRECISION=DOUBLE& shift & goto parse_args
if /i "%~1"=="--float"  set PRECISION=FLOAT& shift & goto parse_args
if /i "%~1"=="--double" set PRECISION=DOUBLE& shift & goto parse_args
if /i "%~1"=="--share"  set SHARE=ON& shift & goto parse_args
if /i "%~1"=="--cli"    set TARGET=CLI& shift & goto parse_args
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
echo --share      Portable binary (no AVX2, runs on older CPUs)
echo --cli       Build CLI version (no SDL/ImGui)
exit /b 0

:check_target
if "%TARGET%"=="CLI" goto build_cli
if "%TARGET%"=="UI" goto build_ui
echo Unknown target: %TARGET% & exit /b 1


:: ============================================================================
:: CLI MODE
:: ============================================================================
:build_cli
echo Building CLI...

set FLAGS=/std:c++20 /O2 /openmp /EHsc /MT ^
/fp:precise /GS- /Gw /Gy /Oi /Ot ^
/I %SRC% ^
/I %SRC%\core ^
/I %SRC%\ext

set DEFS=
if "%MODE%"=="CUDA" set DEFS=%DEFS% -DUSE_CUDA
if "%PRECISION%"=="FLOAT" set DEFS=%DEFS% -DUSE_FLOAT

if "%SHARE%"=="OFF" set FLAGS=%FLAGS% /arch:AVX2

del %BUILD%\*.obj 2>nul

:: Collect sources (CLI + core + ext, but NOT UI)
set FILES=
for %%F in (%SRC%\core\*.cpp) do set FILES=!FILES! %%F
for %%F in (%SRC%\ext\*.cpp) do set FILES=!FILES! %%F
set FILES=!FILES! %SRC%\app\octofractalis-cli.cpp


echo Compiling...
rc /DCLI_BUILD /fo %BUILD%\octofractalis.res %SRC%\app\octofractalis.rc
cl %FLAGS% %DEFS% /Fo%BUILD%\ %FILES% %BUILD%\octofractalis.res /Fe:octofractalis-cli.exe

echo Build complete: octofractalis-cli.exe
exit /b 0


:: ============================================================================
:: UI MODE
:: ============================================================================
:build_ui
echo Building UI...

set "SDL_FOUND=0"
echo ---- SDL SEARCH START ----

for /d %%D in (%LIBS%\SDL3-*) do (
    if exist "%%D\include\SDL3\SDL.h" (
        set "SDL_DIR=%%D"
        set "SDL_FOUND=1"
        goto sdl_setup
    )
)

:sdl_setup
if "%SDL_FOUND%"=="1" (
    set "ARCH=%VSCMD_ARG_TGT_ARCH%"
    if "%ARCH%"=="" set "ARCH=x64"

    set "SDL_LIB_DIR=!SDL_DIR!\lib\!ARCH!"
    set "SDL_INCLUDE_DIR=!SDL_DIR!\include"
)

if exist "%SDL_LIB_DIR%\SDL3.dll" copy /y "%SDL_LIB_DIR%\SDL3.dll" . >nul



:: Compiler flags
set CXX_FLAGS=/nologo /std:c++20 /O2 /Ob3 /GL /openmp ^
/fp:precise ^
/EHsc /MT /GS- /Gw /Gy /Oi /Ot ^
/I %SRC% ^
/I %SRC%\core ^
/I %SRC%\ext ^
/I %SRC%\ext\imgui ^
/I %SRC%\ext\imgui\backends ^
/I "%SDL_INCLUDE_DIR%"

set DEFS=
if "%MODE%"=="CUDA" set DEFS=%DEFS% -DUSE_CUDA
if "%PRECISION%"=="FLOAT" set DEFS=%DEFS% -DUSE_FLOAT

if "%SHARE%"=="OFF" set CXX_FLAGS=%CXX_FLAGS% /arch:AVX2

del %BUILD%\*.obj 2>nul

:: Collect sources (exclude CLI)
set FILES=
for %%F in (%SRC%\core\*.cpp) do set FILES=!FILES! %%F
for %%F in (%SRC%\ext\*.cpp) do set FILES=!FILES! %%F
for %%F in (%SRC%\ext\imgui\*.cpp) do set FILES=!FILES! %%F
set FILES=!FILES! %SRC%\ext\imgui\backends\imgui_impl_sdl3.cpp
set FILES=!FILES! %SRC%\ext\imgui\backends\imgui_impl_sdlrenderer3.cpp
set FILES=!FILES! %SRC%\app\octofractalis-ui.cpp

echo Compiling...
cl %CXX_FLAGS% %DEFS% /Fo%BUILD%\ /c !FILES!

:: CUDA
if "%MODE%"=="CUDA" (
    set NVCC_ARCH=-gencode arch=compute_75,code=sm_75 -gencode arch=compute_86,code=sm_86
    nvcc -c %SRC%\gpu\octofractalis-kernel.cu -o %BUILD%\kernel.obj -O3 %DEFS% !NVCC_ARCH! --ptxas-options=-v -Xcompiler "/MD /EHsc"
)

:: Link
set LIBS_LINK=SDL3.lib user32.lib gdi32.lib shell32.lib
set LIB_PATHS=/LIBPATH:"%SDL_LIB_DIR%"

if "%MODE%"=="CUDA" (
    set LIBS_LINK=%LIBS_LINK% cudart.lib
    set LIB_PATHS=%LIB_PATHS% /LIBPATH:"%CUDA_PATH%\lib\x64"
)

rc /fo %BUILD%\octofractalis.res %SRC%\app\octofractalis.rc
link /nologo /LTCG /OUT:octofractalis.exe %BUILD%\*.obj %BUILD%\octofractalis.res %LIB_PATHS% %LIBS_LINK%

echo Build complete: octofractalis.exe
exit /b 0