# Consolidated Wine toolchain file for cross-compiling from Linux to Windows
#
# This file provides common Wine setup for both MSVC and clang-cl compilers.
# It should be included by specific toolchain files that set the WINE_COMPILER variable.
#
# Required variables (set before including this file):
#   WINE_COMPILER - Either "MSVC" or "CLANG_CL"
#
# Prerequisites:
#   - Wine installed and configured
#   - For MSVC: msvc-wine installed (https://github.com/mstorsjo/msvc-wine)
#   - For CLANG_CL: Windows LLVM/Clang installed in Wine or clang-cl wrapper
#
# Optional environment variables:
#   WINE_PREFIX - Path to Wine prefix (default: ~/.wine)
#   MSVC_WINE_CL - Path to cl.exe for MSVC mode
#   CLANG_CL_WINE - Path to clang-cl for CLANG_CL mode

# Validate that WINE_COMPILER is set
if(NOT DEFINED WINE_COMPILER)
    message(FATAL_ERROR "WINE_COMPILER must be set to either MSVC or CLANG_CL before including wine-toolchain.cmake")
endif()

if(NOT WINE_COMPILER STREQUAL "MSVC" AND NOT WINE_COMPILER STREQUAL "CLANG_CL")
    message(FATAL_ERROR "WINE_COMPILER must be either MSVC or CLANG_CL, got: ${WINE_COMPILER}")
endif()

# Common system settings
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set architecture info for GNUInstallDirs
set(CMAKE_SIZEOF_VOID_P 8)

# Skip compiler checks - Wine makes these unreliable
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_ABI_COMPILED 1)
set(CMAKE_CXX_ABI_COMPILED 1)

# Set Wine prefix
if(NOT DEFINED ENV{WINE_PREFIX})
    set(WINE_PREFIX "$ENV{HOME}/.wine")
else()
    set(WINE_PREFIX "$ENV{WINE_PREFIX}")
endif()

# Compiler-specific setup
if(WINE_COMPILER STREQUAL "MSVC")
    # MSVC setup
    if(DEFINED ENV{MSVC_WINE_CL})
        set(CMAKE_C_COMPILER "$ENV{MSVC_WINE_CL}")
        set(CMAKE_CXX_COMPILER "$ENV{MSVC_WINE_CL}")
    else()
        find_program(MSVC_WINE_CL cl)
        if(NOT MSVC_WINE_CL)
            message(FATAL_ERROR "cl not found in PATH. Please ensure msvc-wine is installed and in PATH")
        endif()
        set(CMAKE_C_COMPILER "${MSVC_WINE_CL}")
        set(CMAKE_CXX_COMPILER "${MSVC_WINE_CL}")
    endif()
    
    # MSVC tools
    set(CMAKE_RC_COMPILER rc)
    set(CMAKE_AR lib)
    set(CMAKE_LINKER link)
    
    # Identify as MSVC
    set(MSVC 1)
    set(CMAKE_C_COMPILER_ID "MSVC" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_ID "MSVC" CACHE STRING "" FORCE)
    
    # Force MSVC version to avoid detection issues with Wine
    # This corresponds to VS2022 17.12 (latest as of writing)
    set(CMAKE_C_COMPILER_VERSION "19.44.35213.0" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_VERSION "19.44.35213.0" CACHE STRING "" FORCE)
    set(MSVC_VERSION 1944 CACHE STRING "" FORCE)
    
    # Set C++ standard defaults for MSVC
    # These are internal CMake variables that must be set to avoid detection issues
    set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT "14" CACHE INTERNAL "")
    set(CMAKE_CXX_EXTENSIONS_COMPUTED_DEFAULT "OFF" CACHE INTERNAL "")
    set(CMAKE_C_STANDARD_COMPUTED_DEFAULT "11" CACHE INTERNAL "")
    set(CMAKE_C_EXTENSIONS_COMPUTED_DEFAULT "OFF" CACHE INTERNAL "")
    
    # MSVC flags
    set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS")
    set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /EHsc")
    
elseif(WINE_COMPILER STREQUAL "CLANG_CL")
    # clang-cl setup
    if(DEFINED ENV{CLANG_CL_WINE})
        set(CMAKE_C_COMPILER "$ENV{CLANG_CL_WINE}")
        set(CMAKE_CXX_COMPILER "$ENV{CLANG_CL_WINE}")
    else()
        find_program(CLANG_CL_WINE clang-cl)
        if(NOT CLANG_CL_WINE)
            message(FATAL_ERROR "clang-cl not found. Please install Windows LLVM in Wine or provide CLANG_CL_WINE")
        endif()
        set(CMAKE_C_COMPILER "${CLANG_CL_WINE}")
        set(CMAKE_CXX_COMPILER "${CLANG_CL_WINE}")
    endif()
    
    # clang-cl tools
    set(CMAKE_RC_COMPILER rc)
    set(CMAKE_AR llvm-lib)
    set(CMAKE_LINKER lld-link)
    
    # Identify as Clang with MSVC compatibility
    set(CMAKE_C_COMPILER_ID "Clang")
    set(CMAKE_CXX_COMPILER_ID "Clang")
    set(CMAKE_C_COMPILER_FRONTEND_VARIANT "MSVC")
    set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "MSVC")
    
    # clang-cl flags
    set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS")
    set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /EHsc /std:c++23")
endif()

# Common Wine settings
# Disable Wine debug output for cleaner builds
set(ENV{WINEDEBUG} "-all")

# Common library prefixes and suffixes
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# Common find root path settings
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)