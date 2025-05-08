# IDE Organization helper module
# This module provides functions for organizing targets in Visual Studio and other IDEs

# Enable IDE organization with folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Recursive function to get all targets
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

# Function to organize special targets that might not be captured by automatic assignment
function(organize_special_targets)
    # Manually set folders for cargo-related targets
    set_folder_for_target(_cargo-build_patternsleuth_bind "deps/first/patternsleuth_bind")
    set_folder_for_target(cargo-build_patternsleuth_bind "deps/first/patternsleuth_bind")
    set_folder_for_target(cargo-clean "deps/first/patternsleuth_bind")
    set_folder_for_target(cargo-clean_patternsleuth_bind "deps/first/patternsleuth_bind")
    set_folder_for_target(cargo-prebuild "deps/first/patternsleuth_bind")
    set_folder_for_target(cargo-prebuild_patternsleuth_bind "deps/first/patternsleuth_bind")

    # Set folder for proxy_files
    set_folder_for_target(proxy_files "programs")

    # Third-party special targets
    set_folder_for_target(asmjit "deps/third")
    set_folder_for_target(asmtk "deps/third")
    set_folder_for_target(Examples "deps/third")
    set_folder_for_target(raw_pdb "deps/third")
endfunction()

# Function to organize all targets based on their source directory
function(organize_targets_by_source_dir)
    # Get all targets in the project
    set(ALL_TARGETS "")
    get_all_targets_recursive(ALL_TARGETS ${CMAKE_SOURCE_DIR})
    
    # Lists of special targets
    set(RUST_CARGO_TARGETS
        "cargo-build"
        "_cargo-build"
        "cargo-build_patternsleuth_bind"
        "_cargo-build_patternsleuth_bind"
        "cargo-clean"
        "cargo-clean_patternsleuth_bind"
        "cargo-prebuild"
        "cargo-prebuild_patternsleuth_bind"
    )

    set(THIRD_PARTY_SPECIAL_TARGETS
        "asmjit"
        "asmtk"
        "Examples" 
        "raw_pdb"
    )

    set(PROGRAM_SPECIAL_TARGETS
        "proxy_files"
    )

    # Organize targets in IDE folders
    foreach(target ${ALL_TARGETS})
        # Skip non-target entries
        if(NOT TARGET ${target})
            continue()
        endif()

        # Get target type
        get_target_property(TARGET_TYPE ${target} TYPE)
        if(TARGET_TYPE STREQUAL "UTILITY")
            continue()  # Skip utility targets
        endif()

        # Handle special target cases first
        set(handled FALSE)
        
        # Handle Rust Cargo targets
        foreach(rust_target ${RUST_CARGO_TARGETS})
            if("${target}" STREQUAL "${rust_target}")
                set_target_properties(${target} PROPERTIES FOLDER "deps/first/patternsleuth_bind")
                set(handled TRUE)
                break()
            endif()
        endforeach()
        
        if(handled)
            continue()
        endif()
        
        # Handle other special third-party targets
        foreach(special_target ${THIRD_PARTY_SPECIAL_TARGETS})
            if("${target}" STREQUAL "${special_target}")
                set_target_properties(${target} PROPERTIES FOLDER "deps/third")
                set(handled TRUE)
                break()
            endif()
        endforeach()
        
        if(handled)
            continue()
        endif()
        
        # Handle special program targets
        foreach(special_target ${PROGRAM_SPECIAL_TARGETS})
            if("${target}" STREQUAL "${special_target}")
                set_target_properties(${target} PROPERTIES FOLDER "programs")
                set(handled TRUE)
                break()
            endif()
        endforeach()
        
        if(handled)
            continue()
        endif()
        
        # Handle GLFW targets
        if("${target}" MATCHES "^glfw" OR "${target}" MATCHES "^uninstall")
            set_target_properties(${target} PROPERTIES FOLDER "deps/third/GLFW")
            set(handled TRUE)
            continue()
        endif()
        
        # Standard organization based on source directory
        if(NOT handled)
            get_target_property(TARGET_SOURCE_DIR ${target} SOURCE_DIR)
            if(TARGET_SOURCE_DIR)
                string(REPLACE "${CMAKE_SOURCE_DIR}/" "" REL_SOURCE_DIR "${TARGET_SOURCE_DIR}")
                
                # Categorize targets
                if(REL_SOURCE_DIR MATCHES "^deps/first")
                    set_target_properties(${target} PROPERTIES FOLDER "deps/first")
                elseif(REL_SOURCE_DIR MATCHES "^deps/third")
                    set_target_properties(${target} PROPERTIES FOLDER "deps/third")
                elseif(REL_SOURCE_DIR MATCHES "^cppmods")
                    set_target_properties(${target} PROPERTIES FOLDER "mods")
                elseif(REL_SOURCE_DIR MATCHES "^UE4SS|^UVTD")
                    set_target_properties(${target} PROPERTIES FOLDER "programs")
                endif()
            endif()
        endif()
    endforeach()
endfunction()

# Function to apply compiler settings to all targets
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
            target_compile_definitions(${target} PRIVATE "${TARGET_COMPILE_DEFINITIONS}")
        endif()
        
        # Set output directories
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${target}/${CMAKE_INSTALL_BINDIR}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${target}/${CMAKE_INSTALL_BINDIR}
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${target}/${CMAKE_INSTALL_BINDIR})
    endforeach()
endfunction()
