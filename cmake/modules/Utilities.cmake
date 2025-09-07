# CMake utility functions
# This module provides utility functions for the build system

# Converts a CMake list to a space-separated string
#
# Arguments:
#   VARNAME - Variable name to store the result in parent scope
#   VALUE - CMake list to convert
#
function(listToString VARNAME VALUE)
    string(REPLACE ";" " " result "${VALUE}")
    set(${VARNAME} "${result}" PARENT_SCOPE)
endfunction()

# Converts a space-separated string to a CMake list
#
# Arguments:
#   VARNAME - Variable name to store the result in parent scope
#   VALUE - Space-separated string to convert
#
function(stringToList VARNAME VALUE)
    string(REPLACE " " ";" result "${VALUE}")
    set(${VARNAME} "${result}" PARENT_SCOPE)
endfunction()

# Generates build configurations based on target types, configuration types, and platform types
#
# Arguments:
#   None (uses global variables TARGET_TYPES, CONFIGURATION_TYPES, PLATFORM_TYPES)
#
# Sets in parent scope:
#   BUILD_CONFIGS - List of generated configuration names
#   TARGET_COMPILE_OPTIONS - Compile options for each configuration
#   TARGET_LINK_OPTIONS - Link options for each configuration
#   TARGET_COMPILE_DEFINITIONS - Compile definitions for each configuration
#
function(generate_build_configurations)
    # These variables will be set in the parent scope
    set(BUILD_CONFIGS "" PARENT_SCOPE)
    set(TARGET_COMPILE_OPTIONS "$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:${DEFAULT_COMPILER_FLAGS}>" PARENT_SCOPE)
    set(TARGET_LINK_OPTIONS "${DEFAULT_EXE_LINKER_FLAGS}" "${DEFAULT_SHARED_LINKER_FLAGS}" PARENT_SCOPE)
    set(TARGET_COMPILE_DEFINITIONS_LOCAL ${TARGET_COMPILE_DEFINITIONS})

    # Build configs to return
    set(BUILD_CONFIGS_LOCAL "")
    
    # Generate triplet build configurations
    foreach(target_type ${TARGET_TYPES})
        foreach(configuration_type ${CONFIGURATION_TYPES})
            foreach(platform_type ${PLATFORM_TYPES})
                # Create triplet name (e.g., Game__Shipping__Win64)
                set(triplet ${target_type}__${configuration_type}__${platform_type})
                list(APPEND BUILD_CONFIGS_LOCAL ${triplet})

                # Combine definitions for this triplet
                set(definitions
                    ${${target_type}_DEFINITIONS}
                    ${${configuration_type}_DEFINITIONS}
                    ${${platform_type}_DEFINITIONS})
                list(APPEND TARGET_COMPILE_DEFINITIONS_LOCAL "$<$<STREQUAL:$<CONFIG>,${triplet}>:${definitions}>")
                
                # This line is critical - it adds the definitions to the current directory scope
                add_compile_definitions("${TARGET_COMPILE_DEFINITIONS_LOCAL}")
                
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

                list(APPEND TARGET_COMPILE_OPTIONS_LOCAL "$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:$<$<STREQUAL:$<CONFIG>,${triplet}>:${compiler_flags}>>")

                # Set up linker flags
                set(linker_flags
                    ${${target_type}_LINKER_FLAGS}
                    ${${configuration_type}_LINKER_FLAGS}
                    ${${platform_type}_LINKER_FLAGS})

                listToString(exe_linker_flags "${DEFAULT_EXE_LINKER_FLAGS}" "${linker_flags}")
                set(CMAKE_EXE_LINKER_FLAGS_${triplet_upper} "${exe_linker_flags}" CACHE STRING "" FORCE)

                listToString(shared_linker_flags "${DEFAULT_SHARED_LINKER_FLAGS}" "${linker_flags}")
                set(CMAKE_SHARED_LINKER_FLAGS_${triplet_upper} "${shared_linker_flags}" CACHE STRING "" FORCE)

                list(APPEND TARGET_LINK_OPTIONS_LOCAL "$<$<STREQUAL:$<CONFIG>,${triplet}>:${linker_flags}>")

                # Set platform-specific variables
                foreach(variable ${${platform_type}_VARS})
                    string(REGEX MATCH "^[^=]+" name ${variable})
                    string(REPLACE "${name}=" "" value ${variable})
                    set(${name} ${value} PARENT_SCOPE)
                endforeach()
            endforeach()
        endforeach()
    endforeach()
    
    # Set the values in the parent scope
    set(BUILD_CONFIGS ${BUILD_CONFIGS_LOCAL} PARENT_SCOPE)
    set(TARGET_COMPILE_OPTIONS "${TARGET_COMPILE_OPTIONS_LOCAL}" PARENT_SCOPE)
    set(TARGET_LINK_OPTIONS "${TARGET_LINK_OPTIONS_LOCAL}" PARENT_SCOPE)
    set(TARGET_COMPILE_DEFINITIONS "${TARGET_COMPILE_DEFINITIONS_LOCAL}" PARENT_SCOPE)
endfunction()

# Sets up the default build configuration
#
# Arguments:
#   None (uses BUILD_CONFIGS from parent scope)
#
function(setup_build_configuration)
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
endfunction()