@echo off
setlocal

echo ========================================
echo Building with System LLVM/Clang
echo ========================================
echo.

set CLANG_PATH=C:\Program Files\LLVM\bin\clang-cl.exe
set LLVM_RC_PATH=C:\Program Files\LLVM\bin\llvm-rc.exe

REM Check if clang-cl exists
if not exist "%CLANG_PATH%" (
    echo Error: clang-cl.exe not found at "%CLANG_PATH%"
    echo Please install LLVM or update the path in this script
    pause
    exit /b 1
)

echo Using Clang from: %CLANG_PATH%

REM Get build configuration from command line or use default
set BuildConfig=%1
if "%BuildConfig%"=="" (
    set BuildConfig=Game__Shipping__Win64
    echo No build configuration specified, using default: Game__Shipping__Win64
)

echo Build configuration: %BuildConfig%
echo.

REM Create build directory
set BuildDir=build_system_clang
if not exist "%BuildDir%" mkdir "%BuildDir%"

cd "%BuildDir%"

REM Configure with CMake using system Clang
echo Configuring CMake...
cmake .. -G "Ninja" ^
    -DCMAKE_C_COMPILER="%CLANG_PATH%" ^
    -DCMAKE_CXX_COMPILER="%CLANG_PATH%" ^
    -DCMAKE_RC_COMPILER="%LLVM_RC_PATH%" ^
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
cmake --build . --config %BuildConfig%

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
echo Output is in: %BuildDir%
echo ========================================
pause