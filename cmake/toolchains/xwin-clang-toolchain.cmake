# Toolchain file for cross-compiling to Windows using xwin and clang
# 
# This is the simple, reliable approach using regular clang with GNU-style flags.
# For MSVC-style clang-cl, use xwin-clang-cl-toolchain.cmake instead.
#
# Prerequisites:
#   - clang and lld installed
#   - xwin installed and splat completed:
#     cargo install xwin
#     xwin --accept-license splat --output /path/to/xwin
#
# Usage:
#   XWIN_DIR=/path/to/xwin cmake -DCMAKE_TOOLCHAIN_FILE=xwin-clang-toolchain.cmake ..

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# CMake Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../modules")

# Include helper modules
include(FindLLVMTools)

# Find GNU-style LLVM tools
# Uses find_llvm_gnu_tools() from cmake/modules/FindLLVMTools.cmake
find_llvm_gnu_tools()

set(CMAKE_C_COMPILER ${CLANG_EXECUTABLE})
set(CMAKE_CXX_COMPILER ${CLANGXX_EXECUTABLE})

# Get xwin directory from environment or use default
if(DEFINED ENV{XWIN_DIR})
    set(XWIN_DIR "$ENV{XWIN_DIR}")
else()
    get_filename_component(XWIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../xwin" ABSOLUTE)
endif()

if(NOT EXISTS "${XWIN_DIR}")
    message(FATAL_ERROR "xwin directory not found at ${XWIN_DIR}. Please set XWIN_DIR environment variable or run: xwin --accept-license splat --output ${XWIN_DIR}")
endif()

# Set up include paths
set(XWIN_CRT_INCLUDE "${XWIN_DIR}/crt/include")
set(XWIN_SDK_INCLUDE "${XWIN_DIR}/sdk/include")

# Set up library paths
set(XWIN_CRT_LIB "${XWIN_DIR}/crt/lib/x86_64")
set(XWIN_SDK_LIB "${XWIN_DIR}/sdk/lib")

# Compiler flags - GNU style with Windows target
set(CMAKE_C_FLAGS_INIT "-target x86_64-pc-windows-msvc -fms-compatibility -fms-extensions")
set(CMAKE_CXX_FLAGS_INIT "-target x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++23 -D_CRT_SECURE_NO_WARNINGS -D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR -Wno-invalid-offsetof")

# Get clang resource directory for intrinsics
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-resource-dir
    OUTPUT_VARIABLE CLANG_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add include directories
# IMPORTANT: Clang's resource directory must come FIRST for intrinsics to work
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${CLANG_RESOURCE_DIR}/include")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_CRT_INCLUDE}")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/ucrt")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/um")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/shared")

set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${CLANG_RESOURCE_DIR}/include")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_CRT_INCLUDE}")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/ucrt")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/um")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/shared")

# Set linker (already found by find_llvm_gnu_tools)
set(CMAKE_LINKER ${LLD_LINK_EXECUTABLE})

# Set resource compiler if found
if(LLVM_RC_EXECUTABLE)
    set(CMAKE_RC_COMPILER ${LLVM_RC_EXECUTABLE})
endif()
# Set RC flags to use Windows SDK includes
set(CMAKE_RC_FLAGS "-I ${XWIN_SDK_INCLUDE}/um -I ${XWIN_SDK_INCLUDE}/shared")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld-link -target x86_64-pc-windows-msvc")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld-link -target x86_64-pc-windows-msvc")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld-link -target x86_64-pc-windows-msvc")

# Add library directories
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${XWIN_CRT_LIB}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${XWIN_SDK_LIB}/um/x86_64")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${XWIN_SDK_LIB}/ucrt/x86_64")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} -L${XWIN_CRT_LIB}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} -L${XWIN_SDK_LIB}/um/x86_64")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} -L${XWIN_SDK_LIB}/ucrt/x86_64")

set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_MODULE_LINKER_FLAGS_INIT} -L${XWIN_CRT_LIB}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_MODULE_LINKER_FLAGS_INIT} -L${XWIN_SDK_LIB}/um/x86_64")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_MODULE_LINKER_FLAGS_INIT} -L${XWIN_SDK_LIB}/ucrt/x86_64")

# Prevent CMake from adding -rdynamic
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Use Windows conventions
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")

# Tell CMake where to find libraries
set(CMAKE_FIND_ROOT_PATH "${XWIN_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Force release runtime even in debug builds (since we only have msvcrt.lib)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "" FORCE)

# Skip compiler checks if needed
#set(CMAKE_C_COMPILER_WORKS 1)
#set(CMAKE_CXX_COMPILER_WORKS 1)

# Override CMake's default libraries to avoid Unix libraries
set(CMAKE_C_STANDARD_LIBRARIES "-lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 -loldnames" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "-lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 -loldnames" CACHE STRING "" FORCE)

# Configure Rust for cross-compilation
set(Rust_CARGO_TARGET "x86_64-pc-windows-msvc" CACHE STRING "" FORCE)

# Disable platform-specific libraries that don't exist on Windows
set(CMAKE_DL_LIBS "" CACHE STRING "" FORCE)
set(CMAKE_THREAD_LIBS_INIT "" CACHE STRING "" FORCE)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)