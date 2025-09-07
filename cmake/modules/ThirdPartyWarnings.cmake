# Third-party library warning suppression module
# This module suppresses warnings from third-party libraries by marking their
# include directories as SYSTEM, which tells the compiler to treat them as
# external system headers.

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
            # Mark include directories as SYSTEM to suppress warnings
            get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
            if(includes)
                set_target_properties(${target} PROPERTIES
                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${includes}")
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