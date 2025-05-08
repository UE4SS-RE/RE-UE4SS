cmake_minimum_required(VERSION 3.18)
project(UE4SSMonorepo)

# Basic setup
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(CXX ASM_MASM)
include(CheckIPOSupported)
include(GNUInstallDirs)

# CMake Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Include helper modules
# The following are defined:
# - From IDEOrganization.cmake:
#   - get_all_targets_recursive(targets dir)
#   - set_folder_for_target(target folder)
#   - organize_special_targets()
#   - organize_targets_by_source_dir()
#   - apply_compiler_settings_to_targets(TARGET_COMPILE_OPTIONS TARGET_LINK_OPTIONS TARGET_COMPILE_DEFINITIONS)
#
# - From Utilities.cmake:
#   - listToString(VARNAME VALUE)
#   - stringToList(VARNAME VALUE)
#   - generate_build_configurations()
#   - setup_build_config()
include(IDEOrganization)
include(Utilities)

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

# Compiler flags setup
list(APPEND Game_FLAGS "")
list(APPEND CasePreserving_FLAGS ${Game_FLAGS})

# Convert standard CMake flags to our format
# Uses stringToList() from cmake/modules/Utilities.cmake
stringToList(DEBUG_FLAGS_LIST "${CMAKE_CXX_FLAGS_DEBUG}")
list(APPEND Debug_FLAGS ${DEBUG_FLAGS_LIST})

stringToList(RELEASE_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELEASE}")
list(APPEND Shipping_FLAGS ${RELEASE_FLAGS_LIST})

stringToList(RELWITHDEBINFO_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
list(APPEND Test_FLAGS ${RELWITHDEBINFO_FLAGS_LIST})

# Generate build configurations
# Uses generate_build_configurations() from cmake/modules/Utilities.cmake
generate_build_configurations()

# Add subdirectories for dependencies and projects
add_subdirectory("deps")
add_subdirectory("cppmods")

foreach(project ${PROJECTS})
    add_subdirectory(${project})
endforeach()

# Organize special targets that might not be captured by automatic assignment
# Uses organize_special_targets() from cmake/modules/IDEOrganization.cmake
organize_special_targets()

# Organize all targets based on their source directory
# Uses organize_targets_by_source_dir() from cmake/modules/IDEOrganization.cmake
organize_targets_by_source_dir()

# Apply compiler settings to all targets
# Uses apply_compiler_settings_to_targets() from cmake/modules/IDEOrganization.cmake
apply_compiler_settings_to_targets("${TARGET_COMPILE_OPTIONS}" "${TARGET_LINK_OPTIONS}" "${TARGET_COMPILE_DEFINITIONS}")

# Set default configuration
# Uses setup_build_config() from cmake/modules/Utilities.cmake
setup_build_config()
