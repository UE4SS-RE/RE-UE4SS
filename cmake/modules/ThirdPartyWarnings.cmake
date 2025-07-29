# Third-party library warning suppression module
# This module provides functions to suppress warnings from third-party libraries

# Helper function to mark a target's include directories as SYSTEM
#
# Arguments:
#   target - Target name to process
#
function(make_system_includes target)
    get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(includes)
        set_target_properties(${target} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${includes}")
    endif()
endfunction()

# Suppresses warnings from specified third-party targets
# 
# Arguments:
#   ARGN - List of target names to suppress warnings for
#
# Example usage:
#   suppress_third_party_warnings(asmjit asmtk glaze fmt)
#
function(suppress_third_party_warnings)
    # Check if suppression is enabled
    if(NOT UE4SS_SUPPRESS_THIRD_PARTY_WARNINGS)
        return()
    endif()
    
    # Process each target
    foreach(target ${ARGN})
        if(TARGET ${target})
            # Get target type
            get_target_property(target_type ${target} TYPE)
            
            # Mark include directories as SYSTEM to suppress warnings in headers
            make_system_includes(${target})
            
            # For compilable targets, suppress warnings in the compilation
            if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
                # Check if we're using clang-cl (Clang with MSVC frontend)
                set(IS_CLANG_CL OFF)
                if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
                    set(IS_CLANG_CL ON)
                endif()
                
                # Add compiler-specific flags to disable warnings
                if(IS_CLANG_CL)
                    # clang-cl uses MSVC-style flags and some GNU-style flags
                    target_compile_options(${target} PRIVATE 
                        /W0      # Disable all warnings
                        -w       # Also use clang's native flag for extra suppression
                        -Wno-everything                  # Disable all Clang warnings
                        -Wno-error                       # Don't treat warnings as errors
                        -Wno-enum-enum-conversion        # Disable enum conversion warning
                        -Wno-error=enum-enum-conversion  # Don't treat enum conversion as error
                    )
                else()
                    # Regular compilers
                    target_compile_options(${target} PRIVATE
                        $<$<CXX_COMPILER_ID:Clang>:-w>
                        $<$<CXX_COMPILER_ID:AppleClang>:-w>
                        $<$<CXX_COMPILER_ID:GNU>:-w>
                        $<$<CXX_COMPILER_ID:MSVC>:/W0>)
                endif()
                
                # For MSVC and clang-cl, also disable specific warnings that might still appear
                if(MSVC OR IS_CLANG_CL)
                    target_compile_options(${target} PRIVATE
                        /wd4996  # Disable deprecated warnings
                        /wd4267  # Disable size_t conversion warnings
                        /wd4244  # Disable narrowing conversion warnings
                        /wd4018  # Disable signed/unsigned mismatch
                        /wd4146  # Disable unary minus operator applied to unsigned type
                        /wd4065  # Disable switch statement contains 'default' but no 'case' labels
                    )
                endif()
            endif()
            
            message(STATUS "Suppressed warnings for third-party target: ${target}")
        endif()
    endforeach()
endfunction()

# Suppresses warnings for all targets matching a pattern in a given directory
#
# Arguments:
#   pattern - Regex pattern to match target names
#   dir - Directory to search for targets
#
# Example usage:
#   suppress_third_party_warnings_regex("^asmjit|^asmtk" "${CMAKE_CURRENT_SOURCE_DIR}")
#
function(suppress_third_party_warnings_regex pattern dir)
    # Get all targets in the specified directory
    get_property(all_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    
    # Filter targets matching the pattern
    set(matching_targets)
    foreach(target ${all_targets})
        if(target MATCHES "${pattern}")
            list(APPEND matching_targets ${target})
        endif()
    endforeach()
    
    # Apply suppression to matching targets
    if(matching_targets)
        suppress_third_party_warnings(${matching_targets})
    endif()
endfunction()

# Suppresses warnings for all targets created in the current directory
# This is meant to be called from within subdirectory CMakeLists.txt files
#
# Example usage (from within deps/third/imgui/CMakeLists.txt):
#   suppress_current_third_party_warnings()
#
function(suppress_current_third_party_warnings)
    # Get all targets defined in the current source directory
    get_property(current_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    
    # Apply suppression to all targets
    if(current_targets)
        suppress_third_party_warnings(${current_targets})
    endif()
endfunction()