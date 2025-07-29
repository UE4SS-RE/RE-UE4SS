#!/bin/bash

# Unified build script for RE-UE4SS
# Supports native and cross-compilation builds with various configurations

set -e

# Default values
GENERATOR="ninja"
COMPILER=""
TOOLCHAIN=""
BUILD_CONFIG="Game__Shipping__Win64"
TARGET="UE4SS"
VERBOSE=0
CLEAN=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_error() { echo -e "${RED}Error: $1${NC}" >&2; }
print_success() { echo -e "${GREEN}$1${NC}"; }
print_info() { echo -e "${YELLOW}$1${NC}"; }

# Function to display usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
  -g, --generator GENERATOR    Build system generator (ninja, make) [default: ninja]
  -c, --compiler COMPILER      Compiler to use (gcc, clang, msvc) [default: system default]
  -t, --toolchain TOOLCHAIN    CMake toolchain file name (without path/extension)
                               Examples: xwin-clang, xwin-clang-cl, msvc-wine, clang-wine
  -b, --build-config CONFIG    Build configuration [default: Game__Shipping__Win64]
  --target TARGET              Build target [default: UE4SS]
  -v, --verbose                Enable verbose output
  --clean                      Clean build directory before building
  -h, --help                   Display this help message

Environment Variables:
  XWIN_DIR                     Path to xwin directory (for xwin toolchains)
  WINE_PREFIX                  Wine prefix path (for wine toolchains)
  CMAKE_PREFIX_PATH            Additional CMake search paths

Examples:
  # Native build with system compiler
  $0

  # Native build with clang
  $0 --compiler clang

  # Cross-compile with xwin clang-cl
  $0 --toolchain xwin-clang-cl

  # Cross-compile with MSVC under Wine
  $0 --toolchain msvc-wine

  # Build specific configuration
  $0 --build-config Game__Debug__Win64

  # Clean build with make generator
  $0 --generator make --clean
EOF
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -g|--generator)
            GENERATOR="$2"
            shift 2
            ;;
        -c|--compiler)
            COMPILER="$2"
            shift 2
            ;;
        -t|--toolchain)
            TOOLCHAIN="$2"
            shift 2
            ;;
        -b|--build-config)
            BUILD_CONFIG="$2"
            shift 2
            ;;
        --target)
            TARGET="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Function to check if a tool is available
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        print_error "$1 is not installed or not in PATH"
        return 1
    fi
    return 0
}

# Validate generator
case $GENERATOR in
    ninja)
        check_tool ninja || exit 1
        CMAKE_GENERATOR="Ninja"
        ;;
    make)
        check_tool make || exit 1
        CMAKE_GENERATOR="Unix Makefiles"
        ;;
    *)
        print_error "Invalid generator: $GENERATOR"
        exit 1
        ;;
esac

# Get script and project directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Use custom CMake if available
if [ -x "$HOME/.local/bin/cmake" ]; then
    CMAKE_BIN="$HOME/.local/bin/cmake"
else
    CMAKE_BIN="cmake"
fi
check_tool "$CMAKE_BIN" || exit 1

# Set up build directory name
BUILD_DIR_PREFIX="build"
if [ -n "$TOOLCHAIN" ]; then
    BUILD_DIR_PREFIX="${BUILD_DIR_PREFIX}_${TOOLCHAIN}"
fi
if [ -n "$COMPILER" ]; then
    BUILD_DIR_PREFIX="${BUILD_DIR_PREFIX}_${COMPILER}"
fi
BUILD_DIR="${BUILD_DIR_PREFIX}_${BUILD_CONFIG}"

# Set up CMake arguments
CMAKE_ARGS=(
    "$PROJECT_ROOT"
    -G "$CMAKE_GENERATOR"
    -DCMAKE_BUILD_TYPE="$BUILD_CONFIG"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

# Set up compiler if specified
if [ -n "$COMPILER" ]; then
    case $COMPILER in
        gcc)
            check_tool gcc || exit 1
            check_tool g++ || exit 1
            export CC=gcc
            export CXX=g++
            ;;
        clang)
            check_tool clang || exit 1
            check_tool clang++ || exit 1
            export CC=clang
            export CXX=clang++
            ;;
        msvc)
            if [ -z "$TOOLCHAIN" ]; then
                print_error "MSVC requires a toolchain (e.g., --toolchain msvc-wine)"
                exit 1
            fi
            ;;
        *)
            print_error "Invalid compiler: $COMPILER"
            exit 1
            ;;
    esac
fi

# Set up toolchain if specified
if [ -n "$TOOLCHAIN" ]; then
    TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchains/${TOOLCHAIN}-toolchain.cmake"
    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        print_error "Toolchain file not found: $TOOLCHAIN_FILE"
        print_info "Available toolchains:"
        ls -1 "$PROJECT_ROOT/cmake/toolchains/" | grep -E '\.cmake$' | sed 's/-toolchain\.cmake$//' | sed 's/^/  /'
        exit 1
    fi
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE")
    
    # Handle toolchain-specific requirements
    case $TOOLCHAIN in
        xwin-*)
            # Check xwin directory
            XWIN_DIR="${XWIN_DIR:-$PROJECT_ROOT/xwin}"
            if [ ! -d "$XWIN_DIR" ]; then
                print_error "xwin directory not found at $XWIN_DIR"
                print_info "Please run: xwin --accept-license splat --output $XWIN_DIR"
                print_info "Or set XWIN_DIR environment variable"
                exit 1
            fi
            export XWIN_DIR

            # Don't set UE4SS_PROXY_PATH - let CMake use its default behavior
            ;;
        *wine*)
            # Check Wine
            check_tool wine || exit 1
            
            # Set Wine prefix if not already set
            export WINE_PREFIX="${WINE_PREFIX:-$HOME/.wine}"
            
            # Disable Wine debug output
            export WINEDEBUG="-all"
            ;;
    esac
fi

# Print build configuration
echo "========================================"
print_info "Building RE-UE4SS"
echo "========================================"
echo "Generator:     $GENERATOR"
echo "Compiler:      ${COMPILER:-system default}"
echo "Toolchain:     ${TOOLCHAIN:-none}"
echo "Configuration: $BUILD_CONFIG"
echo "Target:        $TARGET"
echo "Build dir:     $BUILD_DIR"
echo "========================================"

# Clean build directory if requested
if [ $CLEAN -eq 1 ]; then
    print_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create and enter build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Create .gitignore to prevent accidental commits of build artifacts
if [ ! -f ".gitignore" ]; then
    cat > .gitignore << 'EOF'
# Automatically generated .gitignore for build directory
# This prevents accidental commits of build artifacts

# Ignore everything in this directory
*
EOF
    print_info "Created .gitignore in build directory"
fi

# Configure with CMake
print_info "Configuring with CMake..."
if [ $VERBOSE -eq 1 ]; then
    echo "CMake command: $CMAKE_BIN ${CMAKE_ARGS[@]}"
fi

if ! "$CMAKE_BIN" "${CMAKE_ARGS[@]}"; then
    print_error "CMake configuration failed!"
    exit 1
fi

# Build
print_info "Building target: $TARGET..."
BUILD_ARGS=()
if [ $VERBOSE -eq 1 ]; then
    case $GENERATOR in
        ninja) BUILD_ARGS+=(-v) ;;
        make) BUILD_ARGS+=(VERBOSE=1) ;;
    esac
fi

if ! "$CMAKE_BIN" --build . --target "$TARGET" -- "${BUILD_ARGS[@]}"; then
    print_error "Build failed!"
    exit 1
fi

# Return to original directory
cd "$PROJECT_ROOT"

echo
print_success "========================================"
print_success "Build completed successfully!"
print_success "Configuration: $BUILD_CONFIG"
print_success "Output is in: $BUILD_DIR"
if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    print_success "Compile commands: $BUILD_DIR/compile_commands.json"
fi
print_success "========================================"