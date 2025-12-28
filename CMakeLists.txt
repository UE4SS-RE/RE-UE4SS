cmake_minimum_required(VERSION 3.22)

project(UE4SSMonorepo)

# Basic setup
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_language(CXX ASM_MASM)
include(CheckIPOSupported)
include(GNUInstallDirs)

# CMake Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Include helper modules
include(IDEOrganization)    # For organizing targets in the IDE
include(Utilities)          # For utility functions
include(IDEVisibility)      # For IDE visibility of source files
include(ThirdPartyWarnings) # For suppressing warnings from third-party libraries
include(CompilerDetection)  # For compiler detection and reporting
include(ProjectConfig)      # For centralized project configuration
include(VersionChecks)      # For compiler and tool version checking
include(ArchitectureDetection) # For CPU architecture and feature detection

# Profilers option
option(UE4SS_PROFILERS "Enable UE4SS profiling features" OFF)
if(UE4SS_PROFILERS)
    add_compile_definitions(UE4SS_PROFILERS)
endif()

# Check IPO/LTO support
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
message("IPO/LTO support: ${IPO_SUPPORTED}; ${IPO_ERROR}")

# Initialize project configuration
# Uses ue4ss_initialize_project() from cmake/modules/ProjectConfig.cmake
ue4ss_initialize_project()

# Projects list is now in ProjectConfig.cmake
set(PROJECTS ${UE4SS_PROJECTS})

# Fix for ninja/clang
unset(CMAKE_CXX_SIMULATE_ID)

# Detailed compiler detection output
# Detect and print compiler information
# Uses detect_and_print_compiler() from cmake/modules/CompilerDetection.cmake
detect_and_print_compiler()

# Perform version checks
# Uses perform_version_checks() from cmake/modules/VersionChecks.cmake
perform_version_checks()

# Global compile definitions are now set by ue4ss_initialize_project()

# Set default output directories
# These create the structure: [BuildDir]/Game__Shipping__Win64/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${CMAKE_INSTALL_LIBDIR})

# Initialize compiler options
add_subdirectory("cmake/modules/CompilerOptions")

# Set compiler options
# NOTE: Do NOT use add_compile_options/add_link_options here as they won't propagate to external consumers
# Instead, these are applied via apply_compiler_settings_to_targets() which uses target_*_options with PUBLIC
# visibility on the UE4SS target, ensuring proper propagation to downstream projects
# add_compile_options("$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:${DEFAULT_COMPILER_FLAGS}>")
# add_link_options("${DEFAULT_SHARED_LINKER_FLAGS}" "${DEFAULT_EXE_LINKER_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize Compiler flags after compiler detection.
# Uses ue4ss_initialize_compiler_flags() from cmake/modules/Utilities.cmake
ue4ss_initialize_compiler_flags()

# Build configurations are now defined in ProjectConfig.cmake
# Generate build configurations
# Uses generate_build_configurations() from cmake/modules/Utilities.cmake
generate_build_configurations()

# Set default configuration
# Uses setup_build_configuration() from cmake/modules/Utilities.cmake
setup_build_configuration()

# Add subdirectories for dependencies and projects
add_subdirectory("deps")
add_subdirectory("cppmods")

foreach(project ${PROJECTS})
    add_subdirectory(${project})
endforeach()

# Organize all targets using the master function
# Uses organize_all_targets() from cmake/modules/IDEOrganization.cmake
organize_all_targets()

# Explicitly place UE4SS at the root under RE-UE4SS
if(TARGET UE4SS)
    set_target_properties(UE4SS PROPERTIES FOLDER "RE-UE4SS")
endif()

# Apply compiler settings to all targets
# Uses apply_compiler_settings_to_targets() from cmake/modules/IDEOrganization.cmake
apply_compiler_settings_to_targets("${TARGET_COMPILE_OPTIONS}" "${TARGET_LINK_OPTIONS}" "${TARGET_COMPILE_DEFINITIONS}")