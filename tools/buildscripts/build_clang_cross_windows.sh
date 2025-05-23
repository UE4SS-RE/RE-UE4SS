#!/bin/bash

# Alternative cross-compilation script using standard clang with Windows target
# This script uses clang's built-in Windows target support without clang-cl

set -e

# Check if required tools are installed
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "Error: $1 is not installed or not in PATH"
        echo "Please install it first"
        exit 1
    fi
}

echo "Checking for required tools..."
check_tool cmake
check_tool ninja
check_tool clang
check_tool lld

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Game__Shipping__Win64}"
BUILD_DIR="build_clang_cross_${BUILD_TYPE}"

echo "Building RE-UE4SS with clang cross-compilation to Windows"
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake using clang with Windows target
echo "Configuring with CMake..."
cmake "$PROJECT_ROOT" -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_C_FLAGS="-target x86_64-pc-windows-msvc -fms-compatibility -fms-extensions" \
  -DCMAKE_CXX_FLAGS="-target x86_64-pc-windows-msvc -fms-compatibility -fms-extensions" \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -target x86_64-pc-windows-msvc" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld -target x86_64-pc-windows-msvc" \
  -DUE4SS_PROXY_PATH=default

# Build
echo "Building RE-UE4SS..."
cmake --build . --target UE4SS

echo "Build completed successfully!"
echo "Output files are in: $BUILD_DIR"