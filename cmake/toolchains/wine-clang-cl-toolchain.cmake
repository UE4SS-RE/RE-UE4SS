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

# Set the compiler type and include the common Wine toolchain
set(WINE_COMPILER "CLANG_CL")
include("${CMAKE_CURRENT_LIST_DIR}/wine-toolchain.cmake")