include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# ------------------------------------------------------------------------------
# Third-Party Dependencies
# ------------------------------------------------------------------------------

# glaze JSON library
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v2.9.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)
# Uses suppress_third_party_warnings() from cmake/modules/ThirdPartyWarnings.cmake
suppress_third_party_warnings(glaze)

# GLFW
add_subdirectory("GLFW")

# glad (OpenGL loader)
add_subdirectory("glad")

# ImGui and related libraries
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
    
FetchContent_Declare(
    ImGuiTextEdit
    GIT_REPOSITORY https://github.com/UE4SS-RE/ImGuiColorTextEdit.git
    GIT_TAG v1.2.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
    
FetchContent_Declare(
    IconFontCppHeaders
    GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
add_subdirectory("imgui")

# Zydis
FetchContent_Declare(
    zydis
    GIT_REPOSITORY https://github.com/zyantific/zydis.git
    GIT_TAG v4.1.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
add_subdirectory("zydis")

# PolyHook
FetchContent_Declare(
    PolyHook2
    GIT_REPOSITORY https://github.com/stevemk14ebr/PolyHook_2_0.git
    GIT_TAG 298d56210b9d9e66cde8f96481d6053925c6ae15
    GIT_PROGRESS ON
)
add_subdirectory("PolyHook_2_0")

# raw_pdb
FetchContent_Declare(
    raw_pdb
    GIT_REPOSITORY https://github.com/MolecularMatters/raw_pdb.git
    GIT_TAG 8c6a7146393c83d27fa101e8bc8017f2a7f151df
    GIT_PROGRESS ON
)
add_subdirectory("raw_pdb")

# Corrosion (Rust integration)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/UE4SS-RE/corrosion.git
    #TODO: Go back to main repo once this issue is fixed.
    GIT_TAG 52844733e14f095c947577627e367ee5f6458af7
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
add_subdirectory("corrosion")

# fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    PATCH_COMMAND ${CMAKE_COMMAND} 
        -DPATCH_SCRIPT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/fmt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/fmt/patch_fmt.cmake
)
add_subdirectory("fmt")

# Tracy profiler
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG 37aff70dfa50cf6307b3fee6074d627dc2929143 # v0.10
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
# Only build Tracy if it's selected as the profiler
if(RC_PROFILER_FLAVOR STREQUAL "Tracy")
    add_subdirectory(tracy)
endif()

# Organize third-party targets and make their headers visible
# This function automatically finds all targets created in this directory tree,
# places them in the deps/third folder in the IDE, and makes their headers visible
function(organize_third_party_targets)
    include(IDEVisibility)
    
    # Helper function to recursively get all targets from a directory
    function(get_all_targets_recursive_local targets dir)
        get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
        foreach(subdir ${subdirs})
            get_all_targets_recursive_local(${targets} ${subdir})
        endforeach()
        
        get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
        list(APPEND ${targets} ${current_targets})
        set(${targets} ${${targets}} PARENT_SCOPE)
    endfunction()
    
    # Get all targets from this directory and subdirectories
    set(ALL_THIRD_PARTY_TARGETS "")
    get_all_targets_recursive_local(ALL_THIRD_PARTY_TARGETS ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Process each valid target
    foreach(target ${ALL_THIRD_PARTY_TARGETS})
        if(TARGET ${target})
            # Check if it's an ALIAS or IMPORTED target
            get_target_property(is_imported ${target} IMPORTED)
            get_target_property(is_alias ${target} ALIASED_TARGET)
            
            if(NOT is_alias AND NOT is_imported)
                # Set folder if not already set
                get_target_property(existing_folder ${target} FOLDER)
                if(existing_folder STREQUAL "existing_folder-NOTFOUND")
                    set_target_properties(${target} PROPERTIES FOLDER "deps/third")
                endif()
                
                # Try to make headers visible by checking common include directory properties
                get_target_property(target_type ${target} TYPE)
                
                # Skip UTILITY and OBJECT targets as they typically don't have headers
                if(NOT target_type STREQUAL "UTILITY" AND NOT target_type STREQUAL "OBJECT_LIBRARY")
                    # Try to get include directories from the target
                    get_target_property(include_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
                    
                    if(include_dirs)
                        # Find the first valid include directory that exists
                        foreach(inc_dir ${include_dirs})
                            # Skip generator expressions and non-existent directories
                            if(NOT inc_dir MATCHES "^\\$<" AND EXISTS ${inc_dir})
                                # Check if this looks like a source/include directory (not a build directory)
                                string(FIND ${inc_dir} "${CMAKE_BINARY_DIR}" is_build_dir)
                                if(is_build_dir EQUAL -1)
                                    make_headers_visible(${target} ${inc_dir})
                                    break()  # Only use the first valid directory
                                endif()
                            endif()
                        endforeach()
                    endif()
                endif()
            endif()
        endif()
    endforeach()
endfunction()

# Call the function after all targets are created
# This needs to be called at the end to ensure all subdirectories have been processed
organize_third_party_targets()
