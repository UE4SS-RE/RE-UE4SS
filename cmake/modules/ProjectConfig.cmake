# Project configuration module
# This module centralizes project-wide configuration settings and constants

# Module dependencies
include(Utilities)  # For string manipulation functions

# Project structure configuration
# NOTE: The build mode lists below are referenced by CI workflows for dynamic matrix generation.
# Keep these declarations on single lines to enable automated parsing via grep/regex.
# Format: set(VAR_NAME "val1" "val2" ... CACHE STRING "description")
set(UE4SS_PROJECTS "UE4SS" "UVTD" CACHE STRING "List of main project targets")
set(UE4SS_TARGET_TYPES "Game" "CasePreserving" "LessEqual421" CACHE STRING "UE4-style target types")
set(UE4SS_CONFIGURATION_TYPES "Debug" "Dev" "Shipping" "Test" CACHE STRING "UE4-style configuration types")
set(UE4SS_PLATFORM_TYPES "Win64" CACHE STRING "Supported platform types")

# Feature toggles
option(MAKE_DEPENDENCIES_SHARED "Make dependencies shared" OFF)
option(UE4SS_CONSOLE_COLORS_ENABLED "Enable console colors" OFF)
option(UE4SS_INPUT_ENABLED "Enable the input system" ON)
option(ENABLE_IDE_SOURCE_VISIBILITY "Enable IDE visibility for source files" ON)
option(UE4SS_SUPPRESS_THIRD_PARTY_WARNINGS "Suppress warnings from third-party libraries" ON)
option(UE4SS_VERSION_CHECK "Enable compiler version checking" ON)

# Profiler configuration
# Default to None - users can opt-in to Tracy or Superluminal if needed
set(RC_PROFILER_FLAVOR "None" CACHE STRING "Select profiler: Tracy, Superluminal, or None")
set_property(CACHE RC_PROFILER_FLAVOR PROPERTY STRINGS Tracy Superluminal None)

# Proxy configuration
set(UE4SS_PROXY_PATH "" CACHE FILEPATH "Path to DLL for proxy generation (empty = use default dwmapi.dll)")

# Target type definitions (UE4-style)
set(UE4SS_Game_DEFINITIONS UE_GAME)
set(UE4SS_CasePreserving_DEFINITIONS ${UE4SS_Game_DEFINITIONS} WITH_CASE_PRESERVING_NAME)
set(UE4SS_LessEqual421_DEFINITIONS ${UE4SS_Game_DEFINITIONS} FNAME_ALIGN8 LESSEQUAL421)

# Target type flags (can be customized per target)
set(UE4SS_Game_FLAGS "")
set(UE4SS_CasePreserving_FLAGS ${UE4SS_Game_FLAGS})
set(UE4SS_LessEqual421_FLAGS ${UE4SS_Game_FLAGS})

# Configuration type definitions (UE4-style)
set(UE4SS_Debug_DEFINITIONS UE_BUILD_DEBUG)
set(UE4SS_Dev_DEFINITIONS UE_BUILD_DEVELOPMENT STATS UE4SS_PROFILERS)
set(UE4SS_Shipping_DEFINITIONS UE_BUILD_SHIPPING)
set(UE4SS_Test_DEFINITIONS UE_BUILD_TEST STATS UE4SS_PROFILERS)

# Platform definitions (UE4-style)
set(UE4SS_Win64_DEFINITIONS PLATFORM_WINDOWS PLATFORM_MICROSOFT OVERRIDE_PLATFORM_HEADER_NAME=Windows UBT_COMPILED_PLATFORM=Win64)
set(UE4SS_Win64_VARS CMAKE_SYSTEM_PROCESSOR=x86_64)

# Initializes the project configuration
#
# This function sets up all project-wide settings and must be called
# before any targets are defined.
#
# Arguments:
#   None
#
# Example usage:
#   ue4ss_initialize_project()
#
function(ue4ss_initialize_project)
    # Set global compile definitions
    set(TARGET_COMPILE_DEFINITIONS "" PARENT_SCOPE)
    
    # Add feature-based definitions
    if(UE4SS_CONSOLE_COLORS_ENABLED)
        list(APPEND TARGET_COMPILE_DEFINITIONS UE4SS_CONSOLE_COLORS_ENABLED)
    endif()
    
    if(UE4SS_INPUT_ENABLED)
        list(APPEND TARGET_COMPILE_DEFINITIONS HAS_INPUT)
    endif()
    
    # Unicode support
    list(APPEND TARGET_COMPILE_DEFINITIONS _UNICODE UNICODE)

    # Windows version targeting (Windows 10 and above)
    if(WIN32)
        list(APPEND TARGET_COMPILE_DEFINITIONS _WIN32_WINNT=0x0A00 WINVER=0x0A00)
    endif()

    # Export to parent scope
    set(TARGET_COMPILE_DEFINITIONS ${TARGET_COMPILE_DEFINITIONS} PARENT_SCOPE)
    
    # Set up the triplet build system
    set(TARGET_TYPES ${UE4SS_TARGET_TYPES} PARENT_SCOPE)
    set(CONFIGURATION_TYPES ${UE4SS_CONFIGURATION_TYPES} PARENT_SCOPE)
    set(PLATFORM_TYPES ${UE4SS_PLATFORM_TYPES} PARENT_SCOPE)
    
    # Set up definitions and flags for each type
    foreach(target_type ${UE4SS_TARGET_TYPES})
        set(${target_type}_DEFINITIONS ${UE4SS_${target_type}_DEFINITIONS} PARENT_SCOPE)
        set(${target_type}_FLAGS ${UE4SS_${target_type}_FLAGS} PARENT_SCOPE)
    endforeach()
    
    foreach(config_type ${UE4SS_CONFIGURATION_TYPES})
        set(${config_type}_DEFINITIONS ${UE4SS_${config_type}_DEFINITIONS} PARENT_SCOPE)
    endforeach()
    
    foreach(platform_type ${UE4SS_PLATFORM_TYPES})
        set(${platform_type}_DEFINITIONS ${UE4SS_${platform_type}_DEFINITIONS} PARENT_SCOPE)
        if(DEFINED UE4SS_${platform_type}_VARS)
            set(${platform_type}_VARS ${UE4SS_${platform_type}_VARS} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

# Initializes compiler flags based on configuration
#
# This function converts standard CMake flags to our custom format
# and sets up configuration-specific flags.
#
# Arguments:
#   None (uses CMake standard variables)
#
# Sets in parent scope:
#   Debug_FLAGS, Shipping_FLAGS, Test_FLAGS
#
function(ue4ss_initialize_compiler_flags)
    # Convert standard CMake flags to our format
    # Uses stringToList() from cmake/modules/Utilities.cmake
    stringToList(DEBUG_FLAGS_LIST "${CMAKE_CXX_FLAGS_DEBUG}")
    
    # Append to existing Debug_FLAGS if already set
    if(DEFINED Debug_FLAGS)
        list(APPEND Debug_FLAGS ${DEBUG_FLAGS_LIST})
    else()
        set(Debug_FLAGS ${DEBUG_FLAGS_LIST})
    endif()
    set(Debug_FLAGS ${Debug_FLAGS} PARENT_SCOPE)
    
    # Dev configuration uses debug flags without optimization
    if(DEFINED Dev_FLAGS)
        list(APPEND Dev_FLAGS ${DEBUG_FLAGS_LIST})
    else()
        set(Dev_FLAGS ${DEBUG_FLAGS_LIST})
    endif()
    set(Dev_FLAGS ${Dev_FLAGS} PARENT_SCOPE)
    
    stringToList(RELEASE_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELEASE}")
    
    # Append to existing Shipping_FLAGS if already set (preserves /Zi from msvc-compatible.cmake)
    if(DEFINED Shipping_FLAGS)
        list(APPEND Shipping_FLAGS ${RELEASE_FLAGS_LIST})
    else()
        set(Shipping_FLAGS ${RELEASE_FLAGS_LIST})
    endif()
    set(Shipping_FLAGS ${Shipping_FLAGS} PARENT_SCOPE)
    
    stringToList(RELWITHDEBINFO_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    
    # Append to existing Test_FLAGS if already set
    if(DEFINED Test_FLAGS)
        list(APPEND Test_FLAGS ${RELWITHDEBINFO_FLAGS_LIST})
    else()
        set(Test_FLAGS ${RELWITHDEBINFO_FLAGS_LIST})
    endif()
    set(Test_FLAGS ${Test_FLAGS} PARENT_SCOPE)
endfunction()