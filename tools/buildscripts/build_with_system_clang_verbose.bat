@echo off
setlocal

echo ========================================
echo Building with System LLVM/Clang (Verbose)
echo ========================================
echo.

set CLANG_PATH=C:\Program Files\LLVM\bin\clang-cl.exe
set CLANGXX_PATH=C:\Program Files\LLVM\bin\clang-cl.exe
set LLVM_RC_PATH=C:\Program Files\LLVM\bin\llvm-rc.exe

REM Check if clang-cl exists
if not exist "%CLANG_PATH%" (
    echo Error: clang-cl.exe not found at "%CLANG_PATH%"
    echo Please install LLVM or update the path in this script
    pause
    exit /b 1
)

REM Display version info
echo Clang version:
"%CLANG_PATH%" --version
echo.

echo Using compilers:
echo   C:   %CLANG_PATH%
echo   C++: %CLANGXX_PATH%
echo   RC:  %LLVM_RC_PATH%
echo.

REM Get build configuration from command line or use default
set BuildConfig=%1
if "%BuildConfig%"=="" (
    set BuildConfig=Game__Shipping__Win64
    echo No build configuration specified, using default: Game__Shipping__Win64
)

echo Build configuration: %BuildConfig%
echo.

REM Create build directory
set BuildDir=build_system_clang_verbose
if not exist "%BuildDir%" mkdir "%BuildDir%"

cd "%BuildDir%"

REM Clear CMake cache to ensure fresh configuration
if exist "CMakeCache.txt" del CMakeCache.txt
if exist "CMakeFiles" rmdir /s /q CMakeFiles

REM Configure with CMake using system Clang with verbose output
echo Configuring CMake with verbose output...
cmake .. -G "Ninja" ^
    -DCMAKE_C_COMPILER="%CLANG_PATH%" ^
    -DCMAKE_CXX_COMPILER="%CLANGXX_PATH%" ^
    -DCMAKE_RC_COMPILER="%LLVM_RC_PATH%" ^
    -DCMAKE_BUILD_TYPE=%BuildConfig% ^
    -DCMAKE_VERBOSE_MAKEFILE=ON ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Configuration complete. Building with verbose output...
echo.

REM Build with verbose output
cmake --build . --config %BuildConfig% -- -v

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    echo Check the output above for the exact compiler commands and errors.
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Build completed successfully!
echo Output is in: %BuildDir%
echo Compile commands exported to: %BuildDir%\compile_commands.json
echo ========================================
pause