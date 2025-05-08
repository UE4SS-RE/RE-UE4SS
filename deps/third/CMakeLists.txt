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

# GLFW
add_subdirectory("GLFW")

# ImGui and related libraries
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND ""
)
    
FetchContent_Declare(
    ImGuiTextEdit
    GIT_REPOSITORY https://github.com/UE4SS-RE/ImGuiColorTextEdit.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND ""
)
    
FetchContent_Declare(
    IconFontCppHeaders
    GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    CONFIGURE_COMMAND ""
)

add_subdirectory("imgui")

# Zydis
FetchContent_Declare(
    zydis
    GIT_REPOSITORY https://github.com/zyantific/zydis.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
add_subdirectory("zydis")

# PolyHook
FetchContent_Declare(
    PolyHook2
    GIT_REPOSITORY https://github.com/stevemk14ebr/PolyHook_2_0.git
    GIT_TAG fd2a88f09c8ae89440858fc52573656141013c7f
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
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG fce4fe54328ada11b823f9ae72346b4e97a27844
)
add_subdirectory("corrosion")

# fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
set(FMT_HEADER_ONLY OFF CACHE BOOL "Build fmt as a compiled library, not header-only")
FetchContent_MakeAvailable(fmt)

# Tracy profiler
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG 37aff70dfa50cf6307b3fee6074d627dc2929143 # v0.10
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy profiler")
set(TRACY_ON_DEMAND OFF CACHE BOOL "On-demand profiling")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library")
FetchContent_MakeAvailable(tracy)

# Organize third-party targets
function(organize_third_party_targets)
    # List of known third-party targets to organize
    set(THIRD_PARTY_TARGETS
        fmt glfw imgui PolyHook_2 Zydis ZydisDecoder ZycoreStatic
        TracyClient
        Zycore Zydis_static Zydis-shared
    )
    
    foreach(target ${THIRD_PARTY_TARGETS})
        if(TARGET ${target})
            # Check if it's an ALIAS target before setting properties
            get_target_property(is_imported ${target} IMPORTED)
            get_target_property(is_alias ${target} ALIASED_TARGET)
            
            if(NOT is_alias AND NOT is_imported)
                # Skip if the target already has a folder set
                get_target_property(EXISTING_FOLDER ${target} FOLDER)
                if(NOT "${EXISTING_FOLDER}" STREQUAL "EXISTING_FOLDER-NOTFOUND")
                    continue()
                endif()
                
                set_target_properties(${target} PROPERTIES FOLDER "deps/third")
            endif()
        endif()
    endforeach()
    
    # Look for other targets from FetchContent
    get_property(FETCHCONTENT_TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    
    foreach(target ${FETCHCONTENT_TARGETS})
        # Check if it's an ALIAS target before setting properties
        if(TARGET ${target})
            get_target_property(is_imported ${target} IMPORTED)
            get_target_property(is_alias ${target} ALIASED_TARGET)
            
            if(NOT is_alias AND NOT is_imported)
                # Skip if the target already has a folder set
                get_target_property(EXISTING_FOLDER ${target} FOLDER)
                if(NOT "${EXISTING_FOLDER}" STREQUAL "EXISTING_FOLDER-NOTFOUND")
                    continue()
                endif()
                
                set_target_properties(${target} PROPERTIES FOLDER "deps/third")
            endif()
        endif()
    endforeach()
endfunction()

# Call the function after all targets are created
organize_third_party_targets()
