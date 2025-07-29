# Toolchain file for cross-compiling from Linux to Windows using clang-cl under Wine
#
# Prerequisites:
#   - Wine installed and configured
#   - Windows LLVM/Clang installed in Wine environment
#   - Or clang-cl wrapper script that runs Windows clang-cl.exe under Wine
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=path/to/clang-wine-toolchain.cmake ..
#
# You may need to set:
#   WINE_PREFIX - Path to Wine prefix (default: ~/.wine)
#   CLANG_CL_WINE - Path to clang-cl wrapper or clang-cl.exe

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set Wine prefix
if(NOT DEFINED ENV{WINE_PREFIX})
    set(WINE_PREFIX "$ENV{HOME}/.wine")
else()
    set(WINE_PREFIX "$ENV{WINE_PREFIX}")
endif()

# Find clang-cl
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

# Set other tools
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

# Disable Wine debug output for cleaner builds
set(ENV{WINEDEBUG} "-all")

# Set library prefixes and suffixes
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# Tell CMake where to find libraries and includes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)