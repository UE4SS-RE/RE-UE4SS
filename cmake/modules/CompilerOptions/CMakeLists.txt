# Compiler options management
#
# This module selects the appropriate compiler options file based on the compiler ID
# and sets the required variables for the build system.

# Required variables set by this module:
# DEFAULT_COMPILER_FLAGS - Common compiler flags for C/C++
# DEFAULT_SHARED_LINKER_FLAGS - Common linker flags for shared libraries
# DEFAULT_EXE_LINKER_FLAGS - Common linker flags for executables

# Check for MSVC-compatible compilers (MSVC or clang-cl)
get_filename_component(COMPILER_NAME "${CMAKE_CXX_COMPILER}" NAME)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   COMPILER_NAME MATCHES "clang-cl" OR
   (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    # MSVC or clang-cl - both use MSVC-style flags
    message(STATUS "CompilerOptions: Using msvc-compatible.cmake")
    include("msvc-compatible.cmake")
    
    # Propagate configuration-specific compiler flags to parent scope
    # These are used by generate_build_configurations()
    # Use CACHE INTERNAL to make them accessible across directory scopes
    set(Debug_FLAGS ${Debug_FLAGS} CACHE INTERNAL "Debug compiler flags")
    set(Dev_FLAGS ${Dev_FLAGS} CACHE INTERNAL "Dev compiler flags")
    set(Shipping_FLAGS ${Shipping_FLAGS} CACHE INTERNAL "Shipping compiler flags")
    set(Test_FLAGS ${Test_FLAGS} CACHE INTERNAL "Test compiler flags")

    # Propagate configuration-specific linker flags to parent scope
    # These are used by generate_build_configurations()
    # Use CACHE INTERNAL to make them accessible across directory scopes
    set(Debug_LINKER_FLAGS ${Debug_LINKER_FLAGS} CACHE INTERNAL "Debug linker flags")
    set(Dev_LINKER_FLAGS ${Dev_LINKER_FLAGS} CACHE INTERNAL "Dev linker flags")
    set(Shipping_LINKER_FLAGS ${Shipping_LINKER_FLAGS} CACHE INTERNAL "Shipping linker flags")
    set(Test_LINKER_FLAGS ${Test_LINKER_FLAGS} CACHE INTERNAL "Test linker flags")
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Regular Clang compiler
    message(STATUS "CompilerOptions: Using clang.cmake")
    include("clang.cmake")
    
    # TODO: Propagate clang-specific configuration flags if defined
    if(DEFINED Debug_FLAGS)
        set(Debug_FLAGS ${Debug_FLAGS} PARENT_SCOPE)
    endif()
    if(DEFINED Shipping_FLAGS)
        set(Shipping_FLAGS ${Shipping_FLAGS} PARENT_SCOPE)
    endif()
    # Add other configurations as needed
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "CompilerOptions: Using gnu.cmake")
    include("gnu.cmake")
    
    # TODO: Propagate GNU-specific configuration flags if defined
    if(DEFINED Debug_FLAGS)
        set(Debug_FLAGS ${Debug_FLAGS} PARENT_SCOPE)
    endif()
    if(DEFINED Shipping_FLAGS)
        set(Shipping_FLAGS ${Shipping_FLAGS} PARENT_SCOPE)
    endif()
    # Add other configurations as needed
    
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Note: The build configuration is handled by the triplet system in the main CMakeLists.txt
# We don't add any additional configuration-specific flags here to preserve compatibility