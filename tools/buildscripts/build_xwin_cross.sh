#!/bin/bash

# Build script for cross-compiling RE-UE4SS from Linux to Windows using xwin and clang-cl
# 
# Prerequisites:
#   pacman -S git cmake ninja clang llvm lld rustup openssh
#   rustup default stable
#   rustup target add x86_64-pc-windows-msvc
#   cargo install xwin
#   cargo install cargo-xwin
#
# First time setup:
#   xwin --accept-license splat --output xwin

# Exit on error
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
# Use new CMake if available
if [ -x "$HOME/.local/bin/cmake" ]; then
    CMAKE_BIN="$HOME/.local/bin/cmake"
else
    CMAKE_BIN="cmake"
fi
check_tool ninja
check_tool clang-19
check_tool lld-link-19
check_tool cargo-xwin

# Configure xwin paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
XWIN_DIR="${XWIN_DIR:-$PROJECT_ROOT/xwin}"

if [ ! -d "$XWIN_DIR" ]; then
    echo "Error: xwin directory not found at $XWIN_DIR"
    echo "Please run: xwin --accept-license splat --output $XWIN_DIR"
    echo "Or set XWIN_DIR environment variable to point to your xwin installation"
    exit 1
fi

# Export xwin directory for the toolchain file
export XWIN_DIR

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Game__Shipping__Win64}"
BUILD_DIR="build_xwin_${BUILD_TYPE}"

echo "Building RE-UE4SS with xwin cross-compilation"
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo "xwin directory: $XWIN_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake using the xwin toolchain
echo "Configuring with CMake..."
echo "Using CMake: $CMAKE_BIN"
"$CMAKE_BIN" "$PROJECT_ROOT" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchains/xwin-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DUE4SS_PROXY_PATH=default \
  -DRust_CARGO="$HOME/.cargo/bin/cargo-xwin"

# Build
echo "Building RE-UE4SS..."
"$CMAKE_BIN" --build . --target UE4SS

echo "Build completed successfully!"
echo "Output files are in: $BUILD_DIR"