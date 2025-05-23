#!/bin/bash

echo "========================================"
echo "Clean Ninja Build"
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

# Get build configuration from command line or use default
BUILD_CONFIG="${1:-Game__Shipping__Win64}"
echo "Build configuration: $BUILD_CONFIG"
echo

# Navigate to repository root (from tools/buildscripts)
cd ../..

# Create build directory
BUILD_DIR="build_ninja_${BUILD_CONFIG}"

# Clean existing build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake using Ninja
echo "Configuring CMake with Ninja (clean build)..."
cmake .. -G "Ninja" \
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

# Build with parallel jobs
ninja -j$(nproc)

if [ $? -ne 0 ]; then
    echo
    echo "Build failed!"
    exit 1
fi

cd ..

echo
echo "========================================"
echo "Clean build completed successfully!"
echo "Configuration: $BUILD_CONFIG"
echo "Output is in: $BUILD_DIR"
echo "Compile commands: $BUILD_DIR/compile_commands.json"
echo "========================================"