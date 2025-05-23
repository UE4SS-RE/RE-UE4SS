# Toolchain file for cross-compiling to Windows using xwin and clang

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use clang-19 as the compiler
set(CMAKE_C_COMPILER clang-19)
set(CMAKE_CXX_COMPILER clang++-19)

# Get xwin directory from environment or use default
if(DEFINED ENV{XWIN_DIR})
    set(XWIN_DIR "$ENV{XWIN_DIR}")
else()
    get_filename_component(XWIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../xwin" ABSOLUTE)
endif()

if(NOT EXISTS "${XWIN_DIR}")
    message(FATAL_ERROR "xwin directory not found at ${XWIN_DIR}")
endif()

# Set up include paths
set(XWIN_CRT_INCLUDE "${XWIN_DIR}/crt/include")
set(XWIN_SDK_INCLUDE "${XWIN_DIR}/sdk/include")

# Set up library paths
set(XWIN_CRT_LIB "${XWIN_DIR}/crt/lib/x86_64")
set(XWIN_SDK_LIB "${XWIN_DIR}/sdk/lib")

# Compiler flags
set(CMAKE_C_FLAGS_INIT "-target x86_64-pc-windows-msvc -fms-compatibility -fms-extensions")
set(CMAKE_CXX_FLAGS_INIT "-target x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++20 -D_CRT_SECURE_NO_WARNINGS")

# Add include directories
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_CRT_INCLUDE}")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/ucrt")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/um")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/shared")

set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_CRT_INCLUDE}")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/ucrt")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/um")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -isystem ${XWIN_SDK_INCLUDE}/shared")

# Set linker
set(CMAKE_LINKER lld-link-19)

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld-link-19 -target x86_64-pc-windows-msvc")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld-link-19 -target x86_64-pc-windows-msvc")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld-link-19 -target x86_64-pc-windows-msvc")

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
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)