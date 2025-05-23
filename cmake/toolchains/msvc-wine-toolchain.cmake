# Toolchain file for cross-compiling from Linux to Windows using MSVC under Wine
#
# Prerequisites:
#   - Wine installed and configured
#   - msvc-wine installed (https://github.com/mstorsjo/msvc-wine)
#   - MSVC installed in Wine environment via msvc-wine
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=path/to/msvc-wine-toolchain.cmake ..
#
# You may need to set:
#   WINE_PREFIX - Path to Wine prefix (default: ~/.wine)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set Wine prefix
if(NOT DEFINED ENV{WINE_PREFIX})
    set(WINE_PREFIX "$ENV{HOME}/.wine")
else()
    set(WINE_PREFIX "$ENV{WINE_PREFIX}")
endif()

# Find cl.exe in msvc-wine installation
find_program(MSVC_WINE_CL cl)
if(NOT MSVC_WINE_CL)
    message(FATAL_ERROR "cl not found in PATH. Please ensure msvc-wine is installed and in PATH")
endif()

# Set compilers
set(CMAKE_C_COMPILER "${MSVC_WINE_CL}")
set(CMAKE_CXX_COMPILER "${MSVC_WINE_CL}")
set(CMAKE_RC_COMPILER rc)
set(CMAKE_AR lib)
set(CMAKE_LINKER link)

# Identify as MSVC
set(MSVC 1)
set(CMAKE_C_COMPILER_ID "MSVC")
set(CMAKE_CXX_COMPILER_ID "MSVC")

# MSVC flags - let CMake handle most of them
set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS")
set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /EHsc")

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