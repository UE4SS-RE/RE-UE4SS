# Toolchain file for cross-compiling to Windows using xwin and clang-cl
#
# This uses clang-cl (MSVC-compatible frontend) for better compatibility with MSVC code.
# For simpler GNU-style compilation, use xwin-clang-toolchain.cmake instead.
#
# Prerequisites:
#   - clang-cl and LLVM tools installed
#   - xwin installed and splat completed:
#     cargo install xwin
#     xwin --accept-license splat --output /path/to/xwin
#
# Usage:
#   XWIN_DIR=/path/to/xwin cmake -DCMAKE_TOOLCHAIN_FILE=xwin-clang-cl-toolchain.cmake ..

# Set the target system and processor
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 10.0)

# Set the target architecture for clang-cl
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

# CMake Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../modules")

# Include helper modules
include(FindLLVMTools)

# Find all required LLVM tools
# Uses find_llvm_windows_tools() from cmake/modules/FindLLVMTools.cmake
find_llvm_windows_tools()

# Verify we found everything we need
if(NOT CLANG_CL_EXECUTABLE)
    message(FATAL_ERROR "clang-cl not found. Please install LLVM with Windows target support.")
endif()

# Set the compilers using the found executable
set(CMAKE_C_COMPILER ${CLANG_CL_EXECUTABLE})
set(CMAKE_CXX_COMPILER ${CLANG_CL_EXECUTABLE})

# Set the linker, archiver, and resource compiler using absolute paths
set(CMAKE_LINKER ${LLD_LINK_EXECUTABLE})
set(CMAKE_AR ${LLVM_LIB_EXECUTABLE})
set(CMAKE_RANLIB ${LLVM_RANLIB_EXECUTABLE})
set(CMAKE_RC_COMPILER ${LLVM_RC_EXECUTABLE})
if(LLVM_MT_EXECUTABLE)
    set(CMAKE_MT ${LLVM_MT_EXECUTABLE})
endif()

# Get xwin directory from environment or use default relative path
if(DEFINED ENV{XWIN_DIR})
    set(XWIN_CACHE_DIR "$ENV{XWIN_DIR}")
else()
    get_filename_component(XWIN_CACHE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../xwin" ABSOLUTE)
endif()

if(NOT EXISTS "${XWIN_CACHE_DIR}")
    message(FATAL_ERROR "xwin directory not found at ${XWIN_CACHE_DIR}. Please set XWIN_DIR environment variable or run: xwin --accept-license splat --output ${XWIN_CACHE_DIR}")
endif()

set(CMAKE_SYSROOT "${XWIN_CACHE_DIR}")

# Use the modern CMake variable to select the MSVC runtime library
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime library selection" FORCE)

execute_process(
    COMMAND ${CLANG_CL_EXECUTABLE} /clang:-print-resource-dir
    OUTPUT_VARIABLE CLANG_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Define all necessary system include paths as external to suppress warnings
set(SYSTEM_INCLUDE_FLAGS "/external:I${CLANG_RESOURCE_DIR}/include /external:I${XWIN_CACHE_DIR}/crt/include /external:I${XWIN_CACHE_DIR}/sdk/include/ucrt /external:I${XWIN_CACHE_DIR}/sdk/include/shared /external:I${XWIN_CACHE_DIR}/sdk/include/um /external:I${XWIN_CACHE_DIR}/sdk/include/winrt /external:W0")

# C flags - just the essentials for cross-compilation
set(CMAKE_C_FLAGS_INIT "-vctoolsdir=${XWIN_CACHE_DIR}/crt -winsdkdir=${XWIN_CACHE_DIR}/sdk ${SYSTEM_INCLUDE_FLAGS}")

# C++ flags - just the essentials for cross-compilation
set(CMAKE_CXX_FLAGS_INIT "-vctoolsdir=${XWIN_CACHE_DIR}/crt -winsdkdir=${XWIN_CACHE_DIR}/sdk ${SYSTEM_INCLUDE_FLAGS} /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

# Explicitly add library paths for the linker
set(LINKER_FLAGS "/LIBPATH:${XWIN_CACHE_DIR}/crt/lib/x86_64 /LIBPATH:${XWIN_CACHE_DIR}/sdk/lib/um/x86_64 /LIBPATH:${XWIN_CACHE_DIR}/sdk/lib/ucrt/x86_64 /MANIFEST:NO")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "")

# Use Windows conventions
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")

# Set find root path modes
set(CMAKE_FIND_ROOT_PATH "${XWIN_CACHE_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Skip compiler checks to avoid issues during cross-compilation
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Tell CMake about the compiler architecture
set(CMAKE_C_COMPILER_ARCHITECTURE_ID "x64" CACHE STRING "C compiler architecture" FORCE)
set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID "x64" CACHE STRING "C++ compiler architecture" FORCE)

# Force C++23 standard
set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ standard" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "C++ standard required" FORCE)
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "C++ extensions" FORCE)

# Override CMake's default libraries for Windows
set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib oldnames.lib" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" CACHE STRING "" FORCE)

# Configure Rust for cross-compilation
set(Rust_CARGO_TARGET "x86_64-pc-windows-msvc" CACHE STRING "" FORCE)

# Disable platform-specific libraries that don't exist on Windows
set(CMAKE_DL_LIBS "" CACHE STRING "" FORCE)
set(CMAKE_THREAD_LIBS_INIT "" CACHE STRING "" FORCE)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)