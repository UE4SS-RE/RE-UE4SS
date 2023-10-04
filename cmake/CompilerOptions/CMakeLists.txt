# This folder is designed to allow for specifying compiler options
# depending on the compiler we are currently compiling with

# Required variables to be set by subdirectory scripts:
# 
# `DEFAULT_COMPILER_FLAGS` - cxx/c compiler flags that are common between all triplets
#
# `DEFAULT_SHARED_LINKER_FLAGS` - shared linker flags that are common between all triplets
# `DEFAULT_EXE_LINKER_FLAGS` - exe linker flags that are common between all triplets
#
# Optional variables to be set by subdirectory scripts:
#
# NOTE: if one of these variables is set for a given compiler, it must be set for all compilers
# NOTE: currently there is no validation for this, so care must be taken when setting new variables
#
# `<TARGET>_FLAGS` - cxx/c compiler flags for this target
# `<TARGET>_LINKER_FLAGS` - shared/exe linker flags for this target
#
# `<CONFIGURATION>_FLAGS` - cxx/c compiler flags for this configuration
# `<CONFIGURATION>_LINKER_FLAGS` - shared/exe linker flags for this configuration
#
# `<PLATFORM>_FLAGS` cxx/c compiler flags for this platform
# `<PLATFORM>_LINKER_FLAGS` shared/exe linker flags for this platform

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    include("msvc.cmake")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    include("clang.cmake")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    include("gnu.cmake")
else ()
    message(FATAL_ERROR "Unsupported compiler id (${CMAKE_CXX_COMPILER_ID})")
endif ()