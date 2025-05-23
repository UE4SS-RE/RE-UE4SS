#!/bin/bash

echo "========================================"
echo "Building with XMake and Clang"
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

# Check if clang is available
if ! command -v clang &> /dev/null; then
    echo "Error: Clang not found in PATH"
    echo "Please install Clang:"
    echo "  Ubuntu/Debian: sudo apt install clang"
    echo "  Fedora: sudo dnf install clang"
    echo "  Arch: sudo pacman -S clang"
    exit 1
fi

# Get build mode from command line or use default
BUILD_MODE="${1:-Game__Shipping__Win64}"
echo "Build mode: $BUILD_MODE"
echo "Using Clang: $(which clang)"
echo "Clang version: $(clang --version | head -n1)"
echo

# Navigate to repository root (from tools/buildscripts)
cd ../..

# Configure xmake with Clang toolchain
echo "Configuring XMake with Clang toolchain..."
xmake config --toolchain=clang -m "$BUILD_MODE" -y

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
echo "Build completed successfully!"
echo "Mode: $BUILD_MODE"
echo "Compiler: Clang"
echo "========================================"