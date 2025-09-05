# IDE Organization helper module
# This module provides functions for organizing targets in Visual Studio and other IDEs

# Enable IDE organization with folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Recursively gets all targets from the given directory and its subdirectories
function(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()

    get_property(dir_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${dir_targets})
    set(${targets} ${${targets}} PARENT_SCOPE)
endfunction()

# Helper function to set folder for a specific target if it exists
function(set_folder_for_target target folder)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER ${folder})
        message(STATUS "Set folder for target ${target} to ${folder}")
    endif()
endfunction()

# Sets folder for targets that match a specific pattern
function(set_folder_for_target_pattern target_pattern folder)
    # Get all targets in the project
    set(ALL_TARGETS "")
    get_all_targets_recursive(ALL_TARGETS ${CMAKE_SOURCE_DIR})
    
    foreach(target ${ALL_TARGETS})
        if(NOT TARGET ${target})
            continue()
        endif()
        
        if("${target}" MATCHES "${target_pattern}")
            # Always set the folder property
            set_target_properties(${target} PROPERTIES FOLDER ${folder})
            message(STATUS "Set folder for target ${target} to ${folder} (matched pattern: ${target_pattern})")
        endif()
    endforeach()
endfunction()

# Sets folder for a list of specific targets
function(set_folder_for_targets_list targets_list folder)
    foreach(target ${targets_list})
        set_folder_for_target(${target} ${folder})
    endforeach()
endfunction()

# Generic helper function for organizing targets by pattern
# This can be called from any CMake file to organize targets
# 
# Arguments:
#   target_pattern - Regex pattern to match target names
#   folder - Folder to place matching targets in
#   base_folder - (Optional) Base folder prefix to use if folder doesn't start with deps/
#
# Example usage:
#   organize_targets("^my_lib$|^my_lib_.*" "my_folder")
#
function(organize_targets target_pattern folder)
    # Get optional base_folder parameter
    if(${ARGC} GREATER 2)
        set(base_folder ${ARGV2})
    else()
        # Default base folder based on source directory
        string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "/first/" is_first)
        string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "/third/" is_third)
        
        if(is_first GREATER -1)
            set(base_folder "deps/first")
        elseif(is_third GREATER -1)
            set(base_folder "deps/third")
        else()
            set(base_folder "")
        endif()
    endif()
    
    # Construct full folder path if needed
    if(NOT "${folder}" MATCHES "^deps/")
        if(NOT "${base_folder}" STREQUAL "")
            set(full_folder "${base_folder}/${folder}")
        else()
            set(full_folder "${folder}")
        endif()
    else()
        set(full_folder "${folder}")
    endif()
    
    # Get all targets
    set(ALL_TARGETS "")
    # Uses get_all_targets_recursive() defined earlier in this file
    get_all_targets_recursive(ALL_TARGETS ${CMAKE_SOURCE_DIR})
    
    # Find matching targets and set their folders
    set(found_match FALSE)
    foreach(target ${ALL_TARGETS})
        if(NOT TARGET ${target})
            continue()
        endif()
        
        if("${target}" MATCHES "${target_pattern}")
            # Skip if the target already has a folder set
            get_target_property(EXISTING_FOLDER ${target} FOLDER)
            if(NOT "${EXISTING_FOLDER}" STREQUAL "EXISTING_FOLDER-NOTFOUND")
                continue()
            endif()
            
            # Set the folder property
            set_target_properties(${target} PROPERTIES FOLDER "${full_folder}")
            set(found_match TRUE)
        endif()
    endforeach()
endfunction()

# Organizes all targets based on their source directory
function(organize_targets_by_source_dir)
    # Get all targets in the project
    set(ALL_TARGETS "")
    get_all_targets_recursive(ALL_TARGETS ${CMAKE_SOURCE_DIR})
    
    # Organize targets in IDE folders
    foreach(target ${ALL_TARGETS})
        # Skip non-target entries
        if(NOT TARGET ${target})
            continue()
        endif()

        # Only set folder if it's not already set
        # This allows explicit settings to take precedence
        get_target_property(EXISTING_FOLDER ${target} FOLDER)
        if(NOT "${EXISTING_FOLDER}" STREQUAL "EXISTING_FOLDER-NOTFOUND")
            continue()
        endif()

        # Get target type
        get_target_property(TARGET_TYPE ${target} TYPE)
        if(TARGET_TYPE STREQUAL "UTILITY")
            continue()  # Skip utility targets
        endif()
        
        # Standard organization based on source directory
        get_target_property(TARGET_SOURCE_DIR ${target} SOURCE_DIR)
        if(TARGET_SOURCE_DIR)
            string(REPLACE "${CMAKE_SOURCE_DIR}/" "" REL_SOURCE_DIR "${TARGET_SOURCE_DIR}")
            
            # Categorize targets
            if(REL_SOURCE_DIR MATCHES "^deps/first")
                set_target_properties(${target} PROPERTIES FOLDER "deps/first")
            elseif(REL_SOURCE_DIR MATCHES "^deps/third")
                set_target_properties(${target} PROPERTIES FOLDER "deps/third")
            elseif(REL_SOURCE_DIR MATCHES "^cppmods")
                set_target_properties(${target} PROPERTIES FOLDER "Mods")
            elseif(REL_SOURCE_DIR MATCHES "^UE4SS")
                set_target_properties(${target} PROPERTIES FOLDER "RE-UE4SS")
            elseif(REL_SOURCE_DIR MATCHES "^UVTD")
                set_target_properties(${target} PROPERTIES FOLDER "Programs")
            endif()
        endif()
    endforeach()
endfunction()

# Applies compiler settings to all targets
function(apply_compiler_settings_to_targets TARGET_COMPILE_OPTIONS TARGET_LINK_OPTIONS TARGET_COMPILE_DEFINITIONS)
    # Get all targets
    set(ALL_TARGETS "")
    get_all_targets_recursive(ALL_TARGETS ${CMAKE_SOURCE_DIR})
    
    foreach(target ${ALL_TARGETS})
        # Skip non-target entries
        if(NOT TARGET ${target})
            continue()
        endif()
        
        # Get target type
        get_target_property(TARGET_TYPE ${target} TYPE)
        
        # Apply compiler options to non-utility and non-interface targets
        if(NOT ${TARGET_TYPE} STREQUAL "UTILITY" AND NOT ${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${target} PRIVATE "${TARGET_COMPILE_OPTIONS}")
            target_link_options(${target} PRIVATE "${TARGET_LINK_OPTIONS}")
            
            # For UE4SS library, export definitions publicly so dependent mods inherit them
            if(${target} STREQUAL "UE4SS")
                target_compile_definitions(${target} PUBLIC "${TARGET_COMPILE_DEFINITIONS}")
            else()
                target_compile_definitions(${target} PRIVATE "${TARGET_COMPILE_DEFINITIONS}")
            endif()
        endif()
        
        # Set per-target output directories if subdirectories are defined
        # These should be set by the project using internal cache variables:
        # CMAKE_PROJECT_RUNTIME_OUTPUT_SUBDIR, CMAKE_PROJECT_LIBRARY_OUTPUT_SUBDIR, CMAKE_PROJECT_ARCHIVE_OUTPUT_SUBDIR
        if(DEFINED CMAKE_PROJECT_RUNTIME_OUTPUT_SUBDIR)
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${target}/${CMAKE_PROJECT_RUNTIME_OUTPUT_SUBDIR}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${target}/${CMAKE_PROJECT_LIBRARY_OUTPUT_SUBDIR}
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${target}/${CMAKE_PROJECT_ARCHIVE_OUTPUT_SUBDIR})
        endif()
    endforeach()
endfunction()

# Master function that organizes all targets 
function(organize_all_targets)
    # Apply source-based organization for all targets
    organize_targets_by_source_dir()
endfunction()
