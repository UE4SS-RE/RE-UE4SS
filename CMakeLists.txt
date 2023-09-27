cmake_minimum_required(VERSION 3.18)
project(UE4SSRepo)

enable_language(CXX ASM_MASM)
include(CheckIPOSupported)
include(GNUInstallDirs)

check_ipo_supported(RESULT supported OUTPUT error)
message("IPO - Supported: ${supported}; ${error}")

# Settings -> START
option(MAKE_DEPENDENCIES_SHARED "Make dependencies shared" OFF)
option(UE4SS_NO_CUSTOM_FLAGS "Disable compilation flags added by UE4SS" OFF)
option(UE4SS_CONSOLE_COLORS_ENABLED "Enable console colors" OFF)
# Settings -> END

# Projects -> START
set(PROJECTS "UE4SS" "UVTD")
# Projects -> END

# Very temporary fix for ninja/clang thinking we should use MSVC simulation
unset(CMAKE_CXX_SIMULATE_ID)

message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")


if (UE4SS_CONSOLE_COLORS_ENABLED)
    add_compile_definitions(UE4SS_CONSOLE_COLORS_ENABLED)
endif ()

# Tell WinAPI macros to map to unicode functions instead of ansi
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)


# CLion fixes -> START
# There's a bug with CLion that causes it to not recognize our API macros.
# This in turn causes it to break completely as it cannot see our struct definitions properly.
if ($ENV{CLION_IDE})
    add_compile_definitions(
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

# Default output path -> START
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR})
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
set(Dev_DEFINITIONS UE_BUILD_DEVELOPMENT)
set(Debug_DEFINITIONS UE_BUILD_DEBUG)
set(Shipping_DEFINITIONS UE_BUILD_SHIPPING)
set(Test_DEFINITIONS UE_BUILD_TEST)

# Platform definitions
set(Win64_DEFINITIONS PLATFORM_WINDOWS)
# Definitions -> END

# Compiler Flags and Options -> START
set(PRIVATE_COMPILE_OPTIONS "")
set(PRIVATE_LINK_OPTIONS "")

# Compiler Flags -> START
# Target compiler flags
set(Game_FLAGS "")
set(CasePreserving_FLAGS ${Game_FLAGS})

# Configuration compiler flags
set(Debug_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
# set(Dev_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
set(Shipping_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
set(Test_FLAGS ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

# Platform compiler flags
set(Win64_FLAGS "")

# Fallback linker flags, if the triplet doesn't have flags defined
set(DEFAULT_SHARED_LINKER_FLAGS)
set(DEFAULT_EXE_LINKER_FLAGS)
# Fallback compiler flags if the triplet doesn't have flags defined
set(DEFAULT_CXX_FLAGS)
set(DEFAULT_C_FLAGS)
# Compiler Flags -> END

# Compiler Options -> START
if (NOT UE4SS_NO_CUSTOM_FLAGS)
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
        set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,$<$<CONFIG:GAME_SHIPPING_WIN64>:/Zi>,/MP;$<$<CONFIG:GAME_SHIPPING_WIN64>:/Zi>;/W3;/wd4005;/wd4251;/wd4068;/Zc:inline;/Zc:strictStrings>")
        set(PRIVATE_LINK_OPTIONS /DEBUG:FULL)
    elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
        set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-g;-gcodeview;-fcolor-diagnostics;-Wno-unknown-pragmas;-Wno-unused-parameter;-fms-extensions;-Wignored-attributes;>")
        set(PRIVATE_LINK_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-g>")
        # add_compile_definitions(printf_s=printf)
    elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
        set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-fms-extensions>")
    endif ()
endif ()

add_compile_options("${PRIVATE_COMPILE_OPTIONS}")
add_link_options("${PRIVATE_LINK_OPTIONS}")
# Compiler Options -> END

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Compiler Flags and Options -> END

set(BUILD_CONFIGS)

foreach (target_type ${TARGET_TYPES})
    foreach (configuration_type ${CONFIGURATION_TYPES})
        foreach (platform_type ${PLATFORM_TYPES})
            set(triplet ${target_type}_${configuration_type}_${platform_type})
            list(APPEND BUILD_CONFIGS ${triplet})

            set(definitions
                    ${${target_type}_DEFINITIONS}
                    ${${configuration_type}_DEFINITIONS}
                    ${${platform_type}_DEFINITIONS})
            add_compile_definitions("$<$<STREQUAL:$<CONFIG>,${triplet}>:${definitions}>")
            string(TOUPPER ${triplet} triplet_upper)
            if (NOT UE4SS_NO_CUSTOM_FLAGS)
                set(compiler_flags
                        ${${target_type}_FLAGS}
                        ${${configuration_type}_FLAGS}
                        ${${platform_type}_FLAGS})
                set(CMAKE_CXX_FLAGS_${triplet_upper} ${compiler_flags})
                set(CMAKE_C_FLAGS_${triplet_upper} ${compiler_flags})
            endif ()

            if (NOT DEFINED CMAKE_SHARED_LINKER_FLAGS_${triplet_upper})
                set(CMAKE_SHARED_LINKER_FLAGS_${triplet_upper} ${DEFAULT_SHARED_LINKER_FLAGS})
            endif ()

            if (NOT DEFINED CMAKE_EXE_LINKER_FLAGS_${triplet_upper})
                set(CMAKE_EXE_LINKER_FLAGS_${triplet_upper} ${DEFAULT_EXE_LINKER_FLAGS})
            endif ()

            if (NOT DEFINED CMAKE_CXX_FLAGS_${triplet_upper})
                set(CMAKE_CXX_FLAGS_${triplet_upper} ${DEFAULT_CXX_FLAGS})
            endif ()

            if (NOT DEFINED CMAKE_C_FLAGS_${triplet_upper})
                set(CMAKE_C_FLAGS_${triplet_upper} ${DEFAULT_C_FLAGS})
            endif ()
        endforeach ()
    endforeach ()
endforeach ()
# Build configurations -> END

# ASM Flags -> START
set(ASMTK_NO_CUSTOM_FLAGS ON CACHE BOOL "" FORCE)
set(ASMJIT_NO_CUSTOM_FLAGS ON CACHE BOOL "" FORCE)
# ASM Flags -> END

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
    message("${target}")
    set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR}/${target}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR}/${target}
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/$<CONFIG>/${CMAKE_INSTALL_BINDIR}/${target})
endforeach ()
# Output path -> END

# Default configuration -> START
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if (is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES ${BUILD_CONFIGS} CACHE STRING "" FORCE)
else ()
    if (NOT CMAKE_BUILD_TYPE)
        message("Defaulting to Game_Shipping_Win64")
        set(CMAKE_BUILD_TYPE Game_Shipping_Win64 CACHE STRING "" FORCE)
    endif ()

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_CONFIGS})
endif ()
# Default configuration -> END