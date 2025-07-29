# Version checking module
# This module provides functions to check compiler and tool versions

# Minimum version requirements
set(UE4SS_MINIMUM_MSVC_TOOLSET_VERSION "14.39")
set(UE4SS_MINIMUM_RUST_VERSION "1.73.0")

# Maximum version requirements (optional - empty means no maximum)
set(UE4SS_MAXIMUM_MSVC_TOOLSET_VERSION "" CACHE STRING "Maximum MSVC toolset version (empty = no limit)")
set(UE4SS_MAXIMUM_RUST_VERSION "" CACHE STRING "Maximum Rust version (empty = no limit)")

# Checks if the MSVC toolset version meets minimum and maximum requirements
#
# This function verifies that the MSVC compiler toolset meets the minimum version
# and optionally does not exceed a maximum version when UE4SS_VERSION_CHECK is enabled.
#
# Arguments:
#   None
#
# Example usage:
#   check_msvc_version()
#
function(check_msvc_version)
    if(NOT UE4SS_VERSION_CHECK)
        message(STATUS "MSVC version checking: DISABLED")
        return()
    endif()
    
    if(MSVC)
        # Extract MSVC toolset version
        # MSVC_VERSION contains the compiler version (e.g., 1939 for VS2022 17.9)
        # We need to convert this to toolset format (14.39)
        math(EXPR MSVC_MAJOR "${MSVC_VERSION} / 100")
        math(EXPR MSVC_MINOR "${MSVC_VERSION} % 100")
        set(MSVC_TOOLSET_VERSION "${MSVC_MAJOR}.${MSVC_MINOR}")
        
        # Parse versions for comparison
        string(REPLACE "." ";" MIN_VERSION_LIST ${UE4SS_MINIMUM_MSVC_TOOLSET_VERSION})
        string(REPLACE "." ";" CUR_VERSION_LIST ${MSVC_TOOLSET_VERSION})
        
        list(GET MIN_VERSION_LIST 0 MIN_MAJOR)
        list(GET MIN_VERSION_LIST 1 MIN_MINOR)
        list(GET CUR_VERSION_LIST 0 CUR_MAJOR)
        list(GET CUR_VERSION_LIST 1 CUR_MINOR)
        
        # Compare minimum version
        if(CUR_MAJOR LESS MIN_MAJOR OR (CUR_MAJOR EQUAL MIN_MAJOR AND CUR_MINOR LESS MIN_MINOR))
            message(FATAL_ERROR "MSVC toolset version ${MSVC_TOOLSET_VERSION} is too low. Minimum required: ${UE4SS_MINIMUM_MSVC_TOOLSET_VERSION}\n"
                                "Please update Visual Studio or set -DUE4SS_VERSION_CHECK=OFF to bypass this check.")
        endif()
        
        # Check maximum version if specified
        if(UE4SS_MAXIMUM_MSVC_TOOLSET_VERSION)
            string(REPLACE "." ";" MAX_VERSION_LIST ${UE4SS_MAXIMUM_MSVC_TOOLSET_VERSION})
            list(GET MAX_VERSION_LIST 0 MAX_MAJOR)
            list(GET MAX_VERSION_LIST 1 MAX_MINOR)
            
            if(CUR_MAJOR GREATER MAX_MAJOR OR (CUR_MAJOR EQUAL MAX_MAJOR AND CUR_MINOR GREATER MAX_MINOR))
                message(FATAL_ERROR "MSVC toolset version ${MSVC_TOOLSET_VERSION} is too high. Maximum allowed: ${UE4SS_MAXIMUM_MSVC_TOOLSET_VERSION}\n"
                                    "Please use an older Visual Studio version or set -DUE4SS_VERSION_CHECK=OFF to bypass this check.")
            endif()
            
            message(STATUS "MSVC toolset version: ${MSVC_TOOLSET_VERSION} (minimum: ${UE4SS_MINIMUM_MSVC_TOOLSET_VERSION}, maximum: ${UE4SS_MAXIMUM_MSVC_TOOLSET_VERSION})")
        else()
            message(STATUS "MSVC toolset version: ${MSVC_TOOLSET_VERSION} (minimum: ${UE4SS_MINIMUM_MSVC_TOOLSET_VERSION}, no maximum limit)")
        endif()
    endif()
endfunction()

# Checks if the Rust version meets minimum and maximum requirements
#
# This function verifies that rustc is available and meets the minimum version
# and optionally does not exceed a maximum version when UE4SS_VERSION_CHECK is enabled.
#
# Arguments:
#   None
#
# Example usage:
#   check_rust_version()
#
function(check_rust_version)
    if(NOT UE4SS_VERSION_CHECK)
        message(STATUS "Rust version checking: DISABLED")
        return()
    endif()
    
    # Find rustc
    find_program(RUSTC_EXECUTABLE rustc)
    
    if(NOT RUSTC_EXECUTABLE)
        message(FATAL_ERROR "rustc not found! Please install Rust from https://www.rust-lang.org/tools/install\n"
                            "or set -DUE4SS_VERSION_CHECK=OFF to bypass this check.")
    endif()
    
    # Get Rust version
    execute_process(
        COMMAND ${RUSTC_EXECUTABLE} --version
        OUTPUT_VARIABLE RUST_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Extract version number (e.g., "rustc 1.73.0" -> "1.73.0")
    if(RUST_VERSION_OUTPUT MATCHES "rustc ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(RUST_VERSION ${CMAKE_MATCH_1})
        
        # Compare minimum version
        if(RUST_VERSION VERSION_LESS UE4SS_MINIMUM_RUST_VERSION)
            message(FATAL_ERROR "Rust version ${RUST_VERSION} is too low. Minimum required: ${UE4SS_MINIMUM_RUST_VERSION}\n"
                                "Please update Rust or set -DUE4SS_VERSION_CHECK=OFF to bypass this check.")
        endif()
        
        # Check maximum version if specified
        if(UE4SS_MAXIMUM_RUST_VERSION)
            if(RUST_VERSION VERSION_GREATER UE4SS_MAXIMUM_RUST_VERSION)
                message(FATAL_ERROR "Rust version ${RUST_VERSION} is too high. Maximum allowed: ${UE4SS_MAXIMUM_RUST_VERSION}\n"
                                    "Please use an older Rust version or set -DUE4SS_VERSION_CHECK=OFF to bypass this check.")
            endif()
            
            message(STATUS "Rust version: ${RUST_VERSION} (minimum: ${UE4SS_MINIMUM_RUST_VERSION}, maximum: ${UE4SS_MAXIMUM_RUST_VERSION})")
        else()
            message(STATUS "Rust version: ${RUST_VERSION} (minimum: ${UE4SS_MINIMUM_RUST_VERSION}, no maximum limit)")
        endif()
    else()
        message(WARNING "Could not determine Rust version from: ${RUST_VERSION_OUTPUT}")
    endif()
endfunction()

# Performs all version checks
#
# This is the main entry point that runs all version checks
# when UE4SS_VERSION_CHECK is enabled.
#
# Arguments:
#   None
#
# Example usage:
#   perform_version_checks()
#
function(perform_version_checks)
    if(UE4SS_VERSION_CHECK)
        message(STATUS "========================================")
        message(STATUS "Version Checks")
        message(STATUS "========================================")
        
        check_msvc_version()
        check_rust_version()
        
        message(STATUS "========================================")
    else()
        message(STATUS "Version checking is disabled (UE4SS_VERSION_CHECK=OFF)")
    endif()
endfunction()