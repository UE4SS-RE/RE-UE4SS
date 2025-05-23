@echo off
setlocal

echo ========================================
echo Generating VS Solution with System LLVM/Clang
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
set BuildDir=VS_SystemClang
if not exist "%BuildDir%" mkdir "%BuildDir%"

cd "%BuildDir%"

REM Configure with CMake for Visual Studio using system Clang
echo Configuring CMake for Visual Studio with ClangCL toolset...
cmake .. -G "Visual Studio 17 2022" -T ClangCL ^
    -DCMAKE_C_COMPILER="%CLANG_PATH%" ^
    -DCMAKE_CXX_COMPILER="%CLANG_PATH%" ^
    -DCMAKE_RC_COMPILER="%LLVM_RC_PATH%"

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    echo.
    echo Note: Visual Studio's ClangCL toolset might override the compiler path.
    echo Consider using build_with_system_clang.bat for more direct control.
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Visual Studio solution generated successfully!
echo.
echo Open: %BuildDir%\UE4SSMonorepo.sln
echo.
echo Note: Visual Studio might still use its bundled Clang.
echo Check the build output to verify which compiler is used.
echo ========================================
pause