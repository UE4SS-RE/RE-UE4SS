# Toolchain file for native Windows builds with MSVC
#
# This toolchain file is for building on Windows using the native MSVC compiler.
# It's primarily useful when you want to explicitly specify MSVC settings or
# when using CMake from the command line without Visual Studio.
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=path/to/windows-msvc-toolchain.cmake ..
#
# Note: When using Visual Studio generators, this toolchain file is typically
# not needed as CMake will automatically detect and use MSVC.

# Set the target system and processor
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 10.0)

# Find MSVC compiler
# Let CMake find cl.exe in the PATH or through vcvarsall.bat environment
find_program(CMAKE_C_COMPILER cl)
find_program(CMAKE_CXX_COMPILER cl)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "MSVC compiler (cl.exe) not found. Please run from a Visual Studio Developer Command Prompt or set up the environment with vcvarsall.bat")
endif()

# Explicitly set compiler identification for MSVC
set(CMAKE_C_COMPILER_ID "MSVC" CACHE STRING "C compiler identification")
set(CMAKE_CXX_COMPILER_ID "MSVC" CACHE STRING "C++ compiler identification")

# Windows SDK and MSVC runtime
# These will be auto-detected by CMake when using cl.exe

# Resource compiler
find_program(CMAKE_RC_COMPILER rc)
if(NOT CMAKE_RC_COMPILER)
    message(WARNING "Windows Resource Compiler (rc.exe) not found")
endif()

# Set library tools
set(CMAKE_AR lib)
set(CMAKE_LINKER link)

# Windows-specific settings
set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS")
set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /EHsc")

# Library prefixes and suffixes
set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure we're building for 64-bit
set(CMAKE_VS_PLATFORM_NAME "x64")
set(CMAKE_GENERATOR_PLATFORM "x64")

# Set runtime library (optional - can be overridden by project)
# /MD  = MultiThreaded DLL
# /MDd = MultiThreaded Debug DLL
# /MT  = MultiThreaded static
# /MTd = MultiThreaded Debug static
# (Let the project decide which runtime to use)