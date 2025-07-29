# Architecture and feature detection utilities
#
# This module provides generic functions to detect CPU features and apply
# appropriate compiler flags based on the detection results.

include(CheckCXXCompilerFlag)

# Generic function to detect CPU features and apply compiler flags
#
# Arguments:
#   target - Target name to configure
#   FEATURE - Feature name (e.g., "sse2", "avx2", "neon")
#   MSVC_FLAG - Flag to use with MSVC/clang-cl (e.g., "/arch:SSE2")
#   GNU_FLAG - Flag to use with GCC/Clang (e.g., "-msse2")
#   ARCHITECTURES - List of architectures that support this feature
#   DEFINE_IF_DISABLED - Define to set if feature is disabled
#   DEFINE_IF_ENABLED - Define to set if feature is enabled
#   ALWAYS_AVAILABLE_ON - List of architectures where feature is always available
#   DISABLE_ON_CROSS_COMPILE - If set, disables feature when cross-compiling
#
# Example usage:
#   detect_and_configure_feature(MyLib
#       FEATURE sse2
#       GNU_FLAG -msse2
#       ARCHITECTURES "x86_64|AMD64|i[3-6]86|x86"
#       DEFINE_IF_DISABLED MY_LIB_DISABLE_SSE2
#       ALWAYS_AVAILABLE_ON "x86_64|AMD64"
#       DISABLE_ON_CROSS_COMPILE  # Disable when cross-compiling
#   )
#
function(detect_and_configure_feature target)
    set(options DISABLE_ON_CROSS_COMPILE)
    set(oneValueArgs FEATURE MSVC_FLAG GNU_FLAG ARCHITECTURES DEFINE_IF_DISABLED DEFINE_IF_ENABLED ALWAYS_AVAILABLE_ON)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 1 FEAT "${options}" "${oneValueArgs}" "${multiValueArgs}")
    
    # Validate required arguments
    if(NOT FEAT_FEATURE)
        message(FATAL_ERROR "detect_and_configure_feature: FEATURE is required")
    endif()
    
    # Check if current architecture supports this feature
    set(architecture_supported FALSE)
    if(FEAT_ARCHITECTURES)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "${FEAT_ARCHITECTURES}")
            set(architecture_supported TRUE)
        endif()
    else()
        # If no architectures specified, assume it's supported everywhere
        set(architecture_supported TRUE)
    endif()
    
    if(NOT architecture_supported)
        # Architecture doesn't support this feature
        if(FEAT_DEFINE_IF_DISABLED)
            target_compile_definitions(${target} PRIVATE ${FEAT_DEFINE_IF_DISABLED})
        endif()
        message(STATUS "${target}: ${FEAT_FEATURE} disabled (unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR})")
        return()
    endif()
    
    # Check if we're cross-compiling and if the feature should be disabled
    # This check must come BEFORE the baseline architecture check
    if(CMAKE_CROSSCOMPILING AND FEAT_DISABLE_ON_CROSS_COMPILE)
        # User explicitly requested to disable this feature when cross-compiling
        if(FEAT_DEFINE_IF_DISABLED)
            target_compile_definitions(${target} PRIVATE ${FEAT_DEFINE_IF_DISABLED})
        endif()
        message(STATUS "${target}: ${FEAT_FEATURE} disabled (cross-compiling with DISABLE_ON_CROSS_COMPILE set)")
        return()
    endif()
    
    # Check if feature is always available on this architecture
    if(FEAT_ALWAYS_AVAILABLE_ON AND CMAKE_SYSTEM_PROCESSOR MATCHES "${FEAT_ALWAYS_AVAILABLE_ON}")
        # Feature is baseline for this architecture
        if(FEAT_DEFINE_IF_ENABLED)
            target_compile_definitions(${target} PRIVATE ${FEAT_DEFINE_IF_ENABLED})
        endif()
        message(STATUS "${target}: ${FEAT_FEATURE} enabled (baseline for ${CMAKE_SYSTEM_PROCESSOR})")
        return()
    endif()
    
    # If cross-compiling without DISABLE_ON_CROSS_COMPILE, continue with detection
    if(CMAKE_CROSSCOMPILING)
        message(STATUS "${target}: ${FEAT_FEATURE} detection for cross-compilation (${CMAKE_HOST_SYSTEM_NAME} -> ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR})")
    endif()
    
    # Determine which flag to test based on compiler
    set(flag_to_test "")
    set(flag_type "")
    
    if(MSVC OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
        if(FEAT_MSVC_FLAG)
            set(flag_to_test "${FEAT_MSVC_FLAG}")
            set(flag_type "MSVC")
        endif()
    else()
        if(FEAT_GNU_FLAG)
            set(flag_to_test "${FEAT_GNU_FLAG}")
            set(flag_type "GNU")
        endif()
    endif()
    
    # If we have a flag to test, check if compiler supports it
    if(flag_to_test)
        string(MAKE_C_IDENTIFIER "COMPILER_SUPPORTS_${FEAT_FEATURE}_${flag_type}" cache_var_name)
        check_cxx_compiler_flag("${flag_to_test}" ${cache_var_name})
        
        if(${cache_var_name})
            # Compiler supports the feature
            target_compile_options(${target} PRIVATE ${flag_to_test})
            if(FEAT_DEFINE_IF_ENABLED)
                target_compile_definitions(${target} PRIVATE ${FEAT_DEFINE_IF_ENABLED})
            endif()
            message(STATUS "${target}: ${FEAT_FEATURE} enabled with ${flag_to_test}")
        else()
            # Compiler doesn't support the feature
            if(FEAT_DEFINE_IF_DISABLED)
                target_compile_definitions(${target} PRIVATE ${FEAT_DEFINE_IF_DISABLED})
            endif()
            message(STATUS "${target}: ${FEAT_FEATURE} disabled (compiler doesn't support ${flag_to_test})")
        endif()
    else()
        # No flag to test for this compiler - assume feature is available if architecture supports it
        if(FEAT_DEFINE_IF_ENABLED)
            target_compile_definitions(${target} PRIVATE ${FEAT_DEFINE_IF_ENABLED})
        endif()
        message(STATUS "${target}: ${FEAT_FEATURE} enabled (no special flags needed)")
    endif()
endfunction()