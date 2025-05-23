#!/bin/bash

echo "========================================"
echo "Cross-compiling for Windows with XMake and MinGW"
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

# Check if mingw is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: MinGW not found in PATH"
    echo "Please install MinGW:"
    echo "  Ubuntu/Debian: sudo apt install mingw-w64"
    echo "  Fedora: sudo dnf install mingw64-gcc mingw64-gcc-c++"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    exit 1
fi

# Get build mode from command line or use default
BUILD_MODE="${1:-Game__Shipping__Win64}"
echo "Build mode: $BUILD_MODE"
echo "Using MinGW: $(which x86_64-w64-mingw32-gcc)"
echo "MinGW version: $(x86_64-w64-mingw32-gcc --version | head -n1)"
echo

# Navigate to repository root (from tools/buildscripts)
cd ../..

# Configure xmake for Windows cross-compilation
echo "Configuring XMake for Windows cross-compilation..."
xmake config --plat=windows --arch=x64 --toolchain=mingw -m "$BUILD_MODE" -y

if [ $? -ne 0 ]; then
    echo
    echo "XMake configuration failed!"
    exit 1
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
echo "Compiler: MinGW"
echo "========================================"