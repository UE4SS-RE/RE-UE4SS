#!/bin/bash

echo "========================================"
echo "Building with Ninja and Clang"
echo "========================================"
echo

# Check if ninja is available
if ! command -v ninja &> /dev/null; then
    echo "Error: Ninja not found in PATH"
    echo "Please install Ninja:"
    echo "  Ubuntu/Debian: sudo apt install ninja-build"
    echo "  Fedora: sudo dnf install ninja-build"
    echo "  Arch: sudo pacman -S ninja"
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

# Get build configuration from command line or use default
BUILD_CONFIG="${1:-Game__Shipping__Win64}"
echo "Build configuration: $BUILD_CONFIG"
echo "Using Clang: $(which clang)"
echo "Clang version: $(clang --version | head -n1)"
echo

# Navigate to repository root (from tools/buildscripts)
cd ../..

# Create build directory
BUILD_DIR="build_ninja_clang_${BUILD_CONFIG}"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# Configure with CMake using Ninja and Clang
echo "Configuring CMake with Ninja and Clang..."
CC=clang CXX=clang++ cmake .. -G "Ninja" \
    -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo
    echo "CMake configuration failed!"
    exit 1
fi

echo
echo "Configuration complete. Building..."
echo

# Build with verbose output to see actual compiler commands
ninja -v

if [ $? -ne 0 ]; then
    echo
    echo "Build failed!"
    exit 1
fi

cd ..

echo
echo "========================================"
echo "Build completed successfully!"
echo "Configuration: $BUILD_CONFIG"
echo "Compiler: Clang"
echo "Output is in: $BUILD_DIR"
echo "Compile commands: $BUILD_DIR/compile_commands.json"
echo "========================================"