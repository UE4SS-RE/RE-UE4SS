#!/bin/bash

echo "========================================"
echo "Cross-compiling for Windows with CMake and MinGW"
echo "========================================"
echo

# Check if MinGW is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: MinGW-w64 not found!"
    echo "Please install MinGW-w64:"
    echo "  Ubuntu/Debian: sudo apt install mingw-w64"
    echo "  Fedora: sudo dnf install mingw64-gcc mingw64-gcc-c++"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    exit 1
fi

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
echo "Using MinGW: $(which x86_64-w64-mingw32-gcc)"
echo "MinGW version: $(x86_64-w64-mingw32-gcc --version | head -n1)"
echo

# Navigate to repository root (from tools/buildscripts)
cd ../..

# Create build directory
BUILD_DIR="build_mingw_${BUILD_CONFIG}"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# Configure with CMake using MinGW toolchain
echo "Configuring CMake with MinGW cross-compilation toolchain..."
cmake .. -G "Ninja" \
    -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
    -DCMAKE_TOOLCHAIN_FILE="../cmake/toolchains/mingw-w64-toolchain.cmake" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo
    echo "CMake configuration failed!"
    echo "This might be due to missing Windows dependencies or incompatible code."
    exit 1
fi

echo
echo "Configuration complete. Building..."
echo

# Build with ninja
ninja -j$(nproc)

if [ $? -ne 0 ]; then
    echo
    echo "Build failed!"
    exit 1
fi

cd ..

echo
echo "========================================"
echo "Cross-compilation completed successfully!"
echo "Target: Windows x64"
echo "Configuration: $BUILD_CONFIG"
echo "Compiler: MinGW-w64"
echo "Output is in: $BUILD_DIR"
echo "========================================"