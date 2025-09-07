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
#   MSVC_WINE_CL - Path to cl.exe

# Set the compiler type and include the common Wine toolchain
set(WINE_COMPILER "MSVC")
include("${CMAKE_CURRENT_LIST_DIR}/wine-toolchain.cmake")