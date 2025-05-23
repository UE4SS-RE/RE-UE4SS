# Toolchain file for cross-compiling to Windows using MinGW-w64

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Which compilers to use
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_ASM_MASM_COMPILER x86_64-w64-mingw32-as)

# Where to look for libraries and headers
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Adjust the default behavior of the FIND_XXX() commands:
# Search for programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set the Windows subsystem
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")

# Wine can be used to run Windows executables during the build
set(CMAKE_CROSSCOMPILING_EMULATOR wine)