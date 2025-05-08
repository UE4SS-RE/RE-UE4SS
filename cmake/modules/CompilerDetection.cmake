# Compiler detection and reporting module
# This module provides functions for detecting compiler information and printing diagnostic output

# Detects and prints comprehensive compiler information
#
# Arguments:
#   None
#
# Example usage:
#   detect_and_print_compiler()
#
# Sets in parent scope:
#   UE4SS_COMPILER_TYPE - Simplified compiler type (msvc, clang-cl, clang, gcc, other)
#
function(detect_and_print_compiler)
    message(STATUS "========================================")
    message(STATUS "Compiler Detection Results:")
    message(STATUS "========================================")
    message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
    message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
    
    # Check for compiler frontend variant
    if(DEFINED CMAKE_CXX_COMPILER_FRONTEND_VARIANT)
        message(STATUS "C++ Compiler Frontend Variant: ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}")
    else()
        message(STATUS "C++ Compiler Frontend Variant: Not set")
    endif()
    
    # Detect if using clang-cl specifically
    get_filename_component(COMPILER_NAME "${CMAKE_CXX_COMPILER}" NAME)
    if(COMPILER_NAME MATCHES "clang-cl")
        message(STATUS "Using clang-cl (Clang with MSVC-compatible frontend)")
        set(UE4SS_COMPILER_TYPE "clang-cl" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        message(STATUS "Using clang-cl (detected via frontend variant)")
        set(UE4SS_COMPILER_TYPE "clang-cl" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(STATUS "Using native MSVC compiler")
        set(UE4SS_COMPILER_TYPE "msvc" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "Using regular Clang compiler")
        set(UE4SS_COMPILER_TYPE "clang" PARENT_SCOPE)
    else()
        message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler")
        set(UE4SS_COMPILER_TYPE "other" PARENT_SCOPE)
    endif()
    
    # Print compiler flags being used
    message(STATUS "Initial C++ Flags: ${CMAKE_CXX_FLAGS_INIT}")
    message(STATUS "========================================")
endfunction()

# Returns a simplified compiler type identifier
#
# Arguments:
#   OUT_VAR - Variable to store the compiler type
#
# Possible values:
#   - "msvc"     - Native MSVC compiler
#   - "clang-cl" - Clang with MSVC-compatible frontend
#   - "clang"    - Regular Clang compiler
#   - "gcc"      - GNU Compiler Collection
#   - "other"    - Other/unknown compiler
#
# Example usage:
#   get_compiler_type(COMPILER_TYPE)
#   if(COMPILER_TYPE STREQUAL "clang-cl")
#       # Handle clang-cl specific logic
#   endif()
#
function(get_compiler_type OUT_VAR)
    get_filename_component(COMPILER_NAME "${CMAKE_CXX_COMPILER}" NAME)
    
    if(COMPILER_NAME MATCHES "clang-cl" OR 
       (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
        set(${OUT_VAR} "clang-cl" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(${OUT_VAR} "msvc" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(${OUT_VAR} "clang" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(${OUT_VAR} "gcc" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "other" PARENT_SCOPE)
    endif()
endfunction()