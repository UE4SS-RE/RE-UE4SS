#!/bin/bash

echo "========================================"
echo "Cross-compiling for Windows with XMake"
echo "========================================"
echo

# Check if xmake is available
if ! command -v xmake &> /dev/null; then
    echo "Error: XMake not found in PATH"
    echo "Please install XMake:"
    echo "  bash <(curl -fsSL https://xmake.io/shget.text)"
    echo "  or visit: https://xmake.io/#/getting_started"
    exit 1
fi

# Get build mode from command line or use default
BUILD_MODE="${1:-Game__Shipping__Win64}"
echo "Build mode: $BUILD_MODE"
echo

# Navigate to repository root (from tools/buildscripts)
cd ../..

# First, let's see what toolchains are available
echo "Checking available toolchains..."
xmake show -l toolchains

echo
echo "Configuring XMake for Windows cross-compilation..."

# Try different cross-compilation approaches
# Option 1: Try with platform=windows and let xmake figure out the toolchain
echo "Attempting cross-compilation with platform=windows..."
xmake config --plat=windows --arch=x64 -m "$BUILD_MODE" -y

if [ $? -ne 0 ]; then
    echo
    echo "Standard cross-compilation failed. Trying with Clang..."
    
    # Option 2: Try with Clang for cross-compilation
    if command -v clang &> /dev/null; then
        echo "Attempting with Clang cross-compilation..."
        xmake config --plat=windows --arch=x64 --toolchain=clang -m "$BUILD_MODE" --cross="x86_64-w64-mingw32-" -y
        
        if [ $? -ne 0 ]; then
            echo "Clang cross-compilation also failed."
            echo
            echo "To cross-compile for Windows, you need one of:"
            echo "1. MinGW-w64: sudo apt install mingw-w64"
            echo "2. Clang with Windows target support"
            echo "3. Wine with MSVC toolchain"
            exit 1
        fi
    else
        echo
        echo "Cross-compilation requires a Windows-capable toolchain."
        echo "Please install one of:"
        echo "1. MinGW-w64: sudo apt install mingw-w64"
        echo "2. Clang: sudo apt install clang"
        exit 1
    fi
fi

echo
echo "Configuration complete. Building..."
echo

# Build with xmake
xmake build -v

if [ $? -ne 0 ]; then
    echo
    echo "Build failed!"
    exit 1
fi

echo
echo "========================================"
echo "Cross-compilation completed successfully!"
echo "Target: Windows x64"
echo "Mode: $BUILD_MODE"
echo "========================================"