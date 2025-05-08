cmake_minimum_required(VERSION 3.18)
project(UE4SSMonorepo)

# Basic setup
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(CXX ASM_MASM)
include(CheckIPOSupported)
include(GNUInstallDirs)

# CMake Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable IDE organization with folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check IPO/LTO support
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
message("IPO/LTO support: ${IPO_SUPPORTED}; ${IPO_ERROR}")

# Force function-level linking for all targets
if(MSVC)
    add_compile_options(/Gy)
endif()

# Project options
option(MAKE_DEPENDENCIES_SHARED "Make dependencies shared" OFF)
option(UE4SS_CONSOLE_COLORS_ENABLED "Enable console colors" OFF)
option(UE4SS_INPUT_ENABLED "Enable the input system" ON)

# Projects 
set(PROJECTS "UE4SS" "UVTD")

# Fix for ninja/clang
unset(CMAKE_CXX_SIMULATE_ID)
message("Compiler: ${CMAKE_CXX_COMPILER_ID}")

# Define global compile definitions
set(TARGET_COMPILE_DEFINITIONS)

# Add option-based definitions
if(UE4SS_CONSOLE_COLORS_ENABLED)
    list(APPEND TARGET_COMPILE_DEFINITIONS UE4SS_CONSOLE_COLORS_ENABLED)
endif()
if(UE4SS_INPUT_ENABLED)
    list(APPEND TARGET_COMPILE_DEFINITIONS HAS_INPUT)
endif()

# Unicode support
list(APPEND TARGET_COMPILE_DEFINITIONS _UNICODE UNICODE)

# CLion fixes
if(DEFINED ENV{CLION_IDE} AND DEFINED UE4SS_ENABLE_CLION_FIXES)
    list(APPEND TARGET_COMPILE_DEFINITIONS
        RC_UE4SS_API=
        RC_UE_API=
        RC_ASM_API=
        RC_DYNOUT_API=
        RC_FILE_API=
        RC_FNCTMR_API=
        RC_INI_PARSER_API=
        RC_INPUT_API=
        RC_JSON_API=
        RC_LMS_API=
        RC_PB_API=
        RC_SPSS_API=
    )
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})

# Build configurations
set(TARGET_TYPES "Game" "CasePreserving")
set(CONFIGURATION_TYPES "Debug" "Shipping" "Test")
set(PLATFORM_TYPES "Win64")

# Definitions
# Target definitions
set(Game_DEFINITIONS UE_GAME)
set(CasePreserving_DEFINITIONS ${Game_DEFINITIONS} WITH_CASE_PRESERVING_NAME)

# Configuration definitions
set(Debug_DEFINITIONS UE_BUILD_DEBUG)
set(Shipping_DEFINITIONS UE_BUILD_SHIPPING)
set(Test_DEFINITIONS UE_BUILD_TEST STATS)

# Platform definitions
set(Win64_DEFINITIONS PLATFORM_WINDOWS PLATFORM_MICROSOFT OVERRIDE_PLATFORM_HEADER_NAME=Windows UBT_COMPILED_PLATFORM=Win64)
set(Win64_VARS CMAKE_SYSTEM_PROCESSOR=x86_64)

# Initialize compiler options
add_subdirectory("cmake/modules/CompilerOptions")

# Set compiler options
add_compile_options("$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:${DEFAULT_COMPILER_FLAGS}>")
add_link_options("${DEFAULT_SHARED_LINKER_FLAGS}" "${DEFAULT_EXE_LINKER_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Helper functions
function(listToString VARNAME VALUE)
    string(REPLACE ";" " " result "${VALUE}")
    set(${VARNAME} "${result}" PARENT_SCOPE)
endfunction()

function(stringToList VARNAME VALUE)
    string(REPLACE " " ";" result "${VALUE}")
    set(${VARNAME} "${result}" PARENT_SCOPE)
endfunction()

# Compiler flags setup
list(APPEND Game_FLAGS "")
list(APPEND CasePreserving_FLAGS ${Game_FLAGS})

# Convert standard CMake flags to our format
stringToList(DEBUG_FLAGS_LIST "${CMAKE_CXX_FLAGS_DEBUG}")
list(APPEND Debug_FLAGS ${DEBUG_FLAGS_LIST})

stringToList(RELEASE_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELEASE}")
list(APPEND Shipping_FLAGS ${RELEASE_FLAGS_LIST})

stringToList(RELWITHDEBINFO_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
list(APPEND Test_FLAGS ${RELWITHDEBINFO_FLAGS_LIST})

# Build configuration setup
set(BUILD_CONFIGS "")
set(TARGET_COMPILE_OPTIONS "$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:${DEFAULT_COMPILER_FLAGS}>")
set(TARGET_LINK_OPTIONS "${DEFAULT_EXE_LINKER_FLAGS}" "${DEFAULT_SHARED_LINKER_FLAGS}")

# Generate triplet build configurations
foreach(target_type ${TARGET_TYPES})
    foreach(configuration_type ${CONFIGURATION_TYPES})
        foreach(platform_type ${PLATFORM_TYPES})
            # Create triplet name (e.g., Game__Shipping__Win64)
            set(triplet ${target_type}__${configuration_type}__${platform_type})
            list(APPEND BUILD_CONFIGS ${triplet})

            # Combine definitions for this triplet
            set(definitions
                ${${target_type}_DEFINITIONS}
                ${${configuration_type}_DEFINITIONS}
                ${${platform_type}_DEFINITIONS})
            list(APPEND TARGET_COMPILE_DEFINITIONS "$<$<STREQUAL:$<CONFIG>,${triplet}>:${definitions}>")
            
            # This line is critical - it adds the definitions to the current directory scope
            add_compile_definitions("${TARGET_COMPILE_DEFINITIONS}")
            
            # Set up compiler flags
            string(TOUPPER ${triplet} triplet_upper)
            set(compiler_flags
                ${${target_type}_FLAGS}
                ${${configuration_type}_FLAGS}
                ${${platform_type}_FLAGS})

            # Convert lists to strings for CMake variables
            listToString(final_compiler_flags "${DEFAULT_COMPILER_FLAGS}" "${compiler_flags}")
            set(CMAKE_CXX_FLAGS_${triplet_upper} "${final_compiler_flags}" CACHE STRING "" FORCE)
            set(CMAKE_C_FLAGS_${triplet_upper} "${final_compiler_flags}" CACHE STRING "" FORCE)

            list(APPEND TARGET_COMPILE_OPTIONS "$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:$<$<STREQUAL:$<CONFIG>,${triplet}>:${compiler_flags}>>")

            # Set up linker flags
            set(linker_flags
                ${${target_type}_LINKER_FLAGS}
                ${${configuration_type}_LINKER_FLAGS}
                ${${platform_type}_LINKER_FLAGS})

            listToString(exe_linker_flags "${DEFAULT_EXE_LINKER_FLAGS}" "${linker_flags}")
            set(CMAKE_EXE_LINKER_FLAGS_${triplet_upper} "${exe_linker_flags}" CACHE STRING "" FORCE)

            listToString(shared_linker_flags "${DEFAULT_SHARED_LINKER_FLAGS}" "${linker_flags}")
            set(CMAKE_SHARED_LINKER_FLAGS_${triplet_upper} "${shared_linker_flags}" CACHE STRING "" FORCE)

            list(APPEND TARGET_LINK_OPTIONS "$<$<STREQUAL:$<CONFIG>,${triplet}>:${linker_flags}>")

            # Set platform-specific variables
            foreach(variable ${${platform_type}_VARS})
                string(REGEX MATCH "^[^=]+" name ${variable})
                string(REPLACE "${name}=" "" value ${variable})
                set(${name} ${value})
            endforeach()
        endforeach()
    endforeach()
endforeach()

# Add subdirectories for dependencies and projects
add_subdirectory("deps")
add_subdirectory("cppmods")

foreach(project ${PROJECTS})
    add_subdirectory(${project})
endforeach()

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

# Get all targets
set(ALL_TARGETS "")
get_all_targets_recursive(ALL_TARGETS ${CMAKE_CURRENT_SOURCE_DIR})

# Manually set folders for specific targets that aren't captured by the automatic assignment
# This needs to be done after all targets are defined and before any other folder assignments
macro(set_folder_for_target target folder)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER ${folder})
        message(STATUS "Set folder for target ${target} to ${folder}")
    endif()
endmacro()

# Manually set folders for cargo-related targets
set_folder_for_target(_cargo-build_patternsleuth_bind "deps/first/patternsleuth_bind")
set_folder_for_target(cargo-build_patternsleuth_bind "deps/first/patternsleuth_bind")
set_folder_for_target(cargo-clean "deps/first/patternsleuth_bind")
set_folder_for_target(cargo-clean_patternsleuth_bind "deps/first/patternsleuth_bind")
set_folder_for_target(cargo-prebuild "deps/first/patternsleuth_bind")
set_folder_for_target(cargo-prebuild_patternsleuth_bind "deps/first/patternsleuth_bind")

# Set folder for proxy_files
set_folder_for_target(proxy_files "programs")

# Define specific target mappings for special cases
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
            string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" REL_SOURCE_DIR "${TARGET_SOURCE_DIR}")
            
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

    # Apply compiler options to non-utility targets if they support them
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

# Set default configuration
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES ${BUILD_CONFIGS} CACHE STRING "" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        message("Defaulting to Game__Shipping__Win64")
        set(CMAKE_BUILD_TYPE Game__Shipping__Win64 CACHE STRING "" FORCE)
    endif()

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_CONFIGS})
endif()
