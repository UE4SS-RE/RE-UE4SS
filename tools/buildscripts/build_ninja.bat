@echo off
setlocal

echo ========================================
echo Building with Ninja
echo ========================================
echo.

REM Check if ninja is available
where ninja >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: Ninja not found in PATH
    echo Please install Ninja and add it to your PATH
    echo Download from: https://github.com/ninja-build/ninja/releases
    pause
    exit /b 1
)

REM Get build configuration from command line or use default
set BuildConfig=%1
if "%BuildConfig%"=="" (
    set BuildConfig=Game__Shipping__Win64
    echo No build configuration specified, using default: Game__Shipping__Win64
)

echo Build configuration: %BuildConfig%
echo.

REM Create build directory
set BuildDir=..\..\build_ninja_%BuildConfig%
if not exist "%BuildDir%" mkdir "%BuildDir%"

cd "%BuildDir%"

REM Configure with CMake using Ninja
echo Configuring CMake with Ninja...
cmake .. -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=%BuildConfig%

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Configuration complete. Building...
echo.

REM Build
ninja

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Build completed successfully!
echo Configuration: %BuildConfig%
echo Output is in: %BuildDir%
echo ========================================
pause