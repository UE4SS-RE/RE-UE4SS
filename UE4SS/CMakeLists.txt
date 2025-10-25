cmake_minimum_required(VERSION 3.22)
project(UE4SS)

message("Configuring UE4SS")

# Add proxy generator subproject
add_subdirectory("proxy_generator")

# Release settings
option(UE4SS_LIB_BETA_IS_STARTED "Have beta releases started for the current major version" ON)
option(UE4SS_LIB_IS_BETA "Is this a beta release" ON)

# Define generated directories
set(UE4SS_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_include")
set(UE4SS_GENERATED_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_src")

# ---------------------------------------------------------------------------
# Version Information
# ---------------------------------------------------------------------------

# Read version from file
file(READ ${UE4SS_GENERATED_SOURCE_DIR}/version.cache UE4SS_LIB_VERSION)
string(REGEX MATCHALL "([0-9]+).([0-9]+).([0-9]+).([0-9]+).([0-9]+)" VERSION_PARTS "${UE4SS_LIB_VERSION}")
set(UE4SS_LIB_VERSION_MAJOR ${CMAKE_MATCH_1})
set(UE4SS_LIB_VERSION_MINOR ${CMAKE_MATCH_2})
set(UE4SS_LIB_VERSION_HOTFIX ${CMAKE_MATCH_3})
set(UE4SS_LIB_VERSION_PRERELEASE ${CMAKE_MATCH_4})  # Publicly available pre-release
set(UE4SS_LIB_VERSION_BETA ${CMAKE_MATCH_5})  # Pre-pre-release version

# Get Git SHA
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE UE4SS_LIB_BUILD_GITSHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT GIT_RESULT EQUAL 0)
        set(UE4SS_LIB_BUILD_GITSHA "unknown")
    endif()
else()
    set(UE4SS_LIB_BUILD_GITSHA "no-git")
endif()

message("UE4SS Version: ${UE4SS_LIB_VERSION_MAJOR}.${UE4SS_LIB_VERSION_MINOR}.${UE4SS_LIB_VERSION_HOTFIX}.${UE4SS_LIB_VERSION_PRERELEASE}.${UE4SS_LIB_VERSION_BETA} (${UE4SS_LIB_BUILD_GITSHA})")

# ---------------------------------------------------------------------------
# Target Configuration
# ---------------------------------------------------------------------------

# Find source files
file(GLOB_RECURSE UE4SS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")
file(GLOB_RECURSE UE4SS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.hpp")

# Create the main library
add_library(UE4SS SHARED ${UE4SS_SOURCES})

# Set C++23 standard 
target_compile_features(UE4SS PUBLIC cxx_std_23)

# Add include directories
target_include_directories(UE4SS PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_include>
)

# Add version definitions with PUBLIC visibility so external consumers get them
target_compile_definitions(UE4SS PUBLIC
    UE4SS_LIB_VERSION_MAJOR=${UE4SS_LIB_VERSION_MAJOR}
    UE4SS_LIB_VERSION_MINOR=${UE4SS_LIB_VERSION_MINOR}
    UE4SS_LIB_VERSION_HOTFIX=${UE4SS_LIB_VERSION_HOTFIX}
    UE4SS_LIB_VERSION_PRERELEASE=${UE4SS_LIB_VERSION_PRERELEASE}
    UE4SS_LIB_VERSION_BETA=${UE4SS_LIB_VERSION_BETA}
    UE4SS_LIB_BETA_STARTED=$<BOOL:${UE4SS_LIB_BETA_IS_STARTED}>
    UE4SS_LIB_IS_BETA=$<BOOL:${UE4SS_LIB_IS_BETA}>
    UE4SS_LIB_BUILD_GITSHA="${UE4SS_LIB_BUILD_GITSHA}"
    UE4SS_CONFIGURATION="$<CONFIG>"
)

# Add headers as sources for IDE integration
target_sources(UE4SS PUBLIC
    FILE_SET ue4ss_headers TYPE HEADERS
    BASE_DIRS include
    FILES ${UE4SS_HEADERS}
)

# Organize in IDE
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${UE4SS_SOURCES} ${UE4SS_HEADERS})

# First party static build definitions
target_compile_definitions(UE4SS PRIVATE
    RC_UE4SS_EXPORTS
    RC_ASM_HELPER_BUILD_STATIC
    RC_FILE_BUILD_STATIC
    RC_DYNAMIC_OUTPUT_BUILD_STATIC
    RC_SINGLE_PASS_SIG_SCANNER_BUILD_STATIC
    RC_SINGLE_PASS_SIG_SCANNER_STATIC
    RC_UNREAL_BUILD_STATIC
    RC_INPUT_BUILD_STATIC
    RC_LUA_MADE_SIMPLE_BUILD_STATIC
    RC_FUNCTION_TIMER_BUILD_STATIC
    RC_PARSER_BASE_BUILD_STATIC
    RC_INI_PARSER_BUILD_STATIC
    RC_JSON_BUILD_STATIC
    RC_JSON_PARSER_BUILD_STATIC
    RC_LUA_WRAPPER_GENERATOR_BUILD_STATIC
)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# Link first-party dependencies
target_link_libraries(UE4SS PUBLIC
    File glaze::glaze DynamicOutput
    Unreal SinglePassSigScanner LuaMadeSimple
    Function IniParser JSON
    Input Constructs Helpers
    MProgram ScopedTimer Profiler
    patternsleuth_bind
)

# Link third-party dependencies
target_link_libraries(UE4SS PUBLIC
    fmt ImGui PolyHook_2 
    d3d11 glfw glad opengl32 
    dbghelp psapi ws2_32 ntdll userenv
)

# ---------------------------------------------------------------------------
# Output Configuration
# ---------------------------------------------------------------------------

# Enable link-time optimization
set_property(TARGET UE4SS PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)

# Set runtime library to dynamic CRT (/MD, /MDd)
# This matches Visual Studio's default, so external mods don't need to set it explicitly.
set_property(TARGET UE4SS PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Set output name and standard
set_target_properties(UE4SS PROPERTIES 
    OUTPUT_NAME "UE4SS"
    CXX_STANDARD 23
)

# Use the organize_targets function from IDEOrganization.cmake
# The organize_special_targets function will also handle this target
organize_targets("^UE4SS$" "RE-UE4SS")
