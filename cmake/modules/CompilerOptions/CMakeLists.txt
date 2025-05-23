# Compiler options management
#
# This module selects the appropriate compiler options file based on the compiler ID
# and sets the required variables for the build system.

# Required variables set by this module:
# DEFAULT_COMPILER_FLAGS - Common compiler flags for C/C++
# DEFAULT_SHARED_LINKER_FLAGS - Common linker flags for shared libraries
# DEFAULT_EXE_LINKER_FLAGS - Common linker flags for executables

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Native MSVC compiler
    include("msvc.cmake")
    
    # Add LTCG flags for Shipping builds
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    
    # Handle the custom Game__Shipping__Win64 configuration
    set(CMAKE_STATIC_LINKER_FLAGS_GAME__SHIPPING__WIN64 "${CMAKE_STATIC_LINKER_FLAGS_GAME__SHIPPING__WIN64} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_GAME__SHIPPING__WIN64 "${CMAKE_EXE_LINKER_FLAGS_GAME__SHIPPING__WIN64} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_GAME__SHIPPING__WIN64 "${CMAKE_SHARED_LINKER_FLAGS_GAME__SHIPPING__WIN64} /LTCG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # clang-cl (Clang with MSVC frontend) - uses MSVC-style flags
    include("clang-cl.cmake")
    
    # Add LTCG flags for Shipping builds (clang-cl supports these)
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    
    # Handle the custom Game__Shipping__Win64 configuration
    set(CMAKE_STATIC_LINKER_FLAGS_GAME__SHIPPING__WIN64 "${CMAKE_STATIC_LINKER_FLAGS_GAME__SHIPPING__WIN64} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_GAME__SHIPPING__WIN64 "${CMAKE_EXE_LINKER_FLAGS_GAME__SHIPPING__WIN64} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_GAME__SHIPPING__WIN64 "${CMAKE_SHARED_LINKER_FLAGS_GAME__SHIPPING__WIN64} /LTCG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Regular Clang compiler
    include("clang.cmake")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    include("gnu.cmake")
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Note: The build configuration is handled by the triplet system in the main CMakeLists.txt
# We don't add any additional configuration-specific flags here to preserve compatibility
