cmake_minimum_required(VERSION 3.18)
project(UE4SSMonorepo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_language(CXX ASM_MASM)
include(CheckIPOSupported)
include(GNUInstallDirs)

# CMake Module / Tool Path -> START
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# CMake Module / Tool Path -> END

check_ipo_supported(RESULT supported OUTPUT error)
message("IPO - Supported: ${supported}; ${error}")

# Settings -> START
option(MAKE_DEPENDENCIES_SHARED "Make dependencies shared" OFF)
option(UE4SS_CONSOLE_COLORS_ENABLED "Enable console colors" OFF)
# Settings -> END

# Projects -> START
set(PROJECTS "UE4SS" "UVTD")
# Projects -> END

# Very temporary fix for ninja/clang thinking we should use MSVC simulation
unset(CMAKE_CXX_SIMULATE_ID)

message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")


if (UE4SS_CONSOLE_COLORS_ENABLED)
    list(APPEND TARGET_COMPILE_DEFINITIONS UE4SS_CONSOLE_COLORS_ENABLED)
endif ()

# Tell WinAPI macros to map to unicode functions instead of ansi
list(APPEND TARGET_COMPILE_DEFINITIONS _UNICODE)
list(APPEND TARGET_COMPILE_DEFINITIONS UNICODE)


# CLion fixes -> START
# There's a bug with CLion that causes it to not recognize our API macros.
# This in turn causes it to break completely as it cannot see our struct definitions properly.
if (DEFINED ENV{CLION_IDE} AND DEFINED UE4SS_ENABLE_CLION_FIXES)
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
endif ()
# CLion fixes -> END

# Output path -> START
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
# Output path -> END

# Build configurations -> START
list(APPEND TARGET_TYPES "Game" "CasePreserving")
list(APPEND CONFIGURATION_TYPES "Debug" "Shipping" "Test")
# list(APPEND CONFIGURATION_TYPES "Development")
list(APPEND PLATFORM_TYPES "Win64")

# Definitions -> START
# Target definitions
set(Game_DEFINITIONS UE_GAME)
set(CasePreserving_DEFINITIONS ${Game_DEFINITIONS} WITH_CASE_PRESERVING_NAME)

# Configuration definitions
set(Dev_DEFINITIONS UE_BUILD_DEVELOPMENT STATS)
set(Debug_DEFINITIONS UE_BUILD_DEBUG)
set(Shipping_DEFINITIONS UE_BUILD_SHIPPING)
set(Test_DEFINITIONS UE_BUILD_TEST STATS)

# Platform definitions
set(Win64_DEFINITIONS PLATFORM_WINDOWS PLATFORM_MICROSOFT OVERRIDE_PLATFORM_HEADER_NAME=Windows UBT_COMPILED_PLATFORM=Win64)

# Definitions -> END

# Vars -> START
set(Win64_VARS CMAKE_SYSTEM_PROCESSOR=x86_64)
# Vars -> END

# Initializing compiler-specific options
add_subdirectory("cmake/CompilerOptions")

# Compiler Options -> START
add_compile_options("$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:${DEFAULT_COMPILER_FLAGS}>")
add_link_options("${DEFAULT_SHARED_LINKER_FLAGS}" "${DEFAULT_EXE_LINKER_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Compiler Options -> END

# Compiler Flags -> START
function (listToString VARNAME VALUE)
    string(REPLACE ";" " " result "${VALUE}")
    set(${VARNAME} "${result}" PARENT_SCOPE)
endfunction ()

function (stringToList VARNAME VALUE)
    string(REPLACE " " ";" result "${VALUE}")
    set(${VARNAME} "${result}" PARENT_SCOPE)
endfunction ()

# APPEND is needed here because compiler_options might set additional args
# Target compiler flags
list(APPEND Game_FLAGS "")
list(APPEND CasePreserving_FLAGS ${Game_FLAGS})

# Configuration compiler flags
stringToList(DEBUG_FLAGS_LIST "${CMAKE_CXX_FLAGS_DEBUG}")
list(APPEND Debug_FLAGS ${DEBUG_FLAGS_LIST})
# list(APPEND Dev_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
stringToList(RELEASE_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELEASE}")
list(APPEND Shipping_FLAGS ${RELEASE_FLAGS_LIST})
stringToList(RELWITHDEBINFO_FLAGS_LIST "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
list(APPEND Test_FLAGS ${RELWITHDEBINFO_FLAGS_LIST})
# Compiler Flags -> END

set(BUILD_CONFIGS)
set(TARGET_COMPILE_OPTIONS "$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:${DEFAULT_COMPILER_FLAGS}>")
set(TARGET_LINK_OPTIONS "${DEFAULT_EXE_LINKER_FLAGS}" "${DEFAULT_SHARED_LINKER_FLAGS}")

foreach (target_type ${TARGET_TYPES})
    foreach (configuration_type ${CONFIGURATION_TYPES})
        foreach (platform_type ${PLATFORM_TYPES})
            set(triplet ${target_type}__${configuration_type}__${platform_type})
            list(APPEND BUILD_CONFIGS ${triplet})

            set(definitions
                    ${${target_type}_DEFINITIONS}
                    ${${configuration_type}_DEFINITIONS}
                    ${${platform_type}_DEFINITIONS})
            list(APPEND TARGET_COMPILE_DEFINITIONS "$<$<STREQUAL:$<CONFIG>,${triplet}>:${definitions}>")
            add_compile_definitions("${TARGET_COMPILE_DEFINITIONS}")
            string(TOUPPER ${triplet} triplet_upper)

            set(compiler_flags
                    ${${target_type}_FLAGS}
                    ${${configuration_type}_FLAGS}
                    ${${platform_type}_FLAGS})

            listToString(final_compiler_flags "${DEFAULT_COMPILER_FLAGS}" "${compiler_flags}")
            set(CMAKE_CXX_FLAGS_${triplet_upper} "${final_compiler_flags}" CACHE STRING "" FORCE)
            set(CMAKE_C_FLAGS_${triplet_upper} "${final_compiler_flags}" CACHE STRING "" FORCE)

            list(APPEND TARGET_COMPILE_OPTIONS "$<$<NOT:$<COMPILE_LANGUAGE:ASM_MASM>>:$<$<STREQUAL:$<CONFIG>,${triplet}>:${compiler_flags}>>")

            set(linker_flags
                ${${target_type}_LINKER_FLAGS}
                ${${configuration_type}_LINKER_FLAGS}
                ${${platform_type}_LINKER_FLAGS})

            listToString(exe_linker_flags "${DEFAULT_EXE_LINKER_FLAGS}" "${linker_flags}")
            set(CMAKE_EXE_LINKER_FLAGS_${triplet_upper} "${exe_linker_flags}" CACHE STRING "" FORCE)

            listToString(shared_linker_flags "${DEFAULT_SHARED_LINKER_FLAGS}" "${linker_flags}")
            set(CMAKE_SHARED_LINKER_FLAGS_${triplet_upper} "${shared_linker_flags}" CACHE STRING "" FORCE)

            listToString(target_linker_flags "${DEFAULT_EXE_LINKER_FLAGS}" "${DEFAULT_SHARED_LINKER_FLAGS}" "${linker_flags}")
            list(APPEND TARGET_LINK_OPTIONS "$<$<STREQUAL:$<CONFIG>,${triplet}>:${target_linker_flags}>")

            foreach (variable in ${${platform_type}_VARS})
                string(REGEX MATCH "^[^=]+" name ${variable})
                string(REPLACE "${name}=" "" value ${variable})
                set(${name} ${value})
            endforeach ()
        endforeach ()
    endforeach ()
endforeach ()
# Build configurations -> END

add_subdirectory("deps")

foreach (project ${PROJECTS})
    add_subdirectory(${project})
endforeach ()

# Output path -> START
set(TARGETS)

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach (subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach ()

    get_property(dir_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${dir_targets})
endmacro()

foreach (project ${PROJECTS})
    get_all_targets_recursive(TARGETS ${project})
endforeach ()

foreach (target ${TARGETS})
    set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${target}/${CMAKE_INSTALL_BINDIR}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${target}/${CMAKE_INSTALL_BINDIR}
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Output/$<CONFIG>/${target}/${CMAKE_INSTALL_BINDIR})

    get_target_property(TARGET_TYPE ${target} TYPE)
    if (NOT ${TARGET_TYPE} STREQUAL UTILITY)
        target_compile_options(${target} PUBLIC "${TARGET_COMPILE_OPTIONS}")
        target_link_options(${target} PUBLIC "${TARGET_LINK_OPTIONS}")
        target_compile_definitions(${target} PUBLIC "${TARGET_COMPILE_DEFINITIONS}")
    endif ()

    get_target_property(DEPS ${target} LINK_LIBRARIES)
    foreach (dep_target ${DEPS})
        if (TARGET ${dep_target})
            get_target_property(DEP_IMPORTED ${dep_target} IMPORTED)
            get_target_property(DEP_TARGET_TYPE ${dep_target} TYPE)
            if (NOT ${DEP_IMPORTED} AND NOT ${DEP_TARGET_TYPE} STREQUAL INTERFACE_LIBRARY)
                target_compile_options(${dep_target} PUBLIC "${TARGET_COMPILE_OPTIONS}")
                target_link_options(${dep_target} PUBLIC "${TARGET_LINK_OPTIONS}")
                target_compile_definitions(${target} PUBLIC "${TARGET_COMPILE_DEFINITIONS}")
            endif ()
        endif ()
    endforeach ()
endforeach ()

# Default configuration -> START
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if (is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES ${BUILD_CONFIGS} CACHE STRING "" FORCE)
else ()
    if (NOT CMAKE_BUILD_TYPE)
        message("Defaulting to Game__Shipping__Win64")
        set(CMAKE_BUILD_TYPE Game__Shipping__Win64 CACHE STRING "" FORCE)
    endif ()

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_CONFIGS})
endif ()
# Default configuration -> END
