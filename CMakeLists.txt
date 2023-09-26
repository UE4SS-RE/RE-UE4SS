cmake_minimum_required(VERSION 3.18)
project(UE4SSMonorepo)

enable_language(CXX ASM_MASM)
include(CheckIPOSupported)
include(GNUInstallDirs)

check_ipo_supported(RESULT supported OUTPUT error)
message("IPO - Supported: ${supported}; ${error}")

# Settings -> START
option(MAKE_DEPENDENCIES_SHARED "Make dependencies shared" OFF)
# Settings -> END

# Projects -> START
set(PROJECTS "UE4SS" "UVTD")
# Projects -> END

# Tell WinAPI macros to map to unicode functions instead of ansi
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

# Very temporary fix for ninja/clang thinking we should use MSVC simulation
unset(CMAKE_CXX_SIMULATE_ID)

message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

# Compile options -> START
set(PRIVATE_COMPILE_OPTIONS "")
set(PRIVATE_LINK_OPTIONS "")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,$<$<CONFIG:RELEASE>:/Zi>,/MP;$<$<CONFIG:RELEASE>:/Zi>;/W3;/wd4005;/wd4251;/wd4068>")
    set(PRIVATE_LINK_OPTIONS /DEBUG:FULL)
    set(CMAKE_CXX_FLAGS_GAME_WIN64_SHIPPING "/O2")
    set(CMAKE_CXX_FLAGS_GAME_WIN64_DEBUG "/Od")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-g;-gcodeview;-fcolor-diagnostics;-Wno-unknown-pragmas;-Wno-unused-parameter;-fms-extensions;-Wignored-attributes>")
    set(PRIVATE_LINK_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-g>")
    set(CMAKE_CXX_FLAGS_GAME_WIN64_SHIPPING "-O3")
    set(CMAKE_CXX_FLAGS_GAME_WIN64_DEBUG "/O0")
    # add_compile_definitions(printf_s=printf)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-fms-extensions>")
    set(CMAKE_CXX_FLAGS_GAME_WIN64_SHIPPING "-O3")
    set(CMAKE_CXX_FLAGS_GAME_WIN64_DEBUG "/O0")
endif ()

add_compile_options("${PRIVATE_COMPILE_OPTIONS}")
add_link_options("${PRIVATE_LINK_OPTIONS}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Compile options -> END

# CLion fixes -> START
# There's a bug with CLion that causes it to not recognize our API macros.
# This in turn causes it to break completely as it cannot see our struct definitions properly.
if ($ENV{CLION_IDE})
    add_compile_definitions(
            RC_UE4SS_API=
            RC_UE_API=
    )
endif ()
# CLion fixes -> END


# Output path -> START
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/bin/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/lib/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/archive/$<CONFIG>)
# Output path -> END

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
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/${target}/$<CONFIG>
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/${target}/$<CONFIG>
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output/${target}/$<CONFIG>)
endforeach ()
# Output path -> END

# Build configurations -> START
list(APPEND GAME_TYPES "Game")
list(APPEND PLATFORM_TYPES "Win64")
list(APPEND SHIPPING_TYPES "Dev" "Debug" "Shipping" "Test")

# Game definitions
set(Game_DEFINITIONS UE_GAME)

# Platform definitions
set(Win64_DEFINITIONS PLATFORM_WINDOWS)

# Shipping definitions
set(Dev_DEFINITIONS UE_BUILD_DEVELOPMENT)
set(Debug_DEFINITIONS UE_BUILD_DEBUG)
set(Shipping_DEFINITIONS UE_BUILD_SHIPPING)
set(Test_DEFINITIONS UE_BUILD_TEST)

# Fallback linker flags, if the triplet doesn't have flags defined
set(DEFAULT_SHARED_LINKER_FLAGS)
set(DEFAULT_EXE_LINKER_FLAGS)
# Fallback compiler flags if the triplet doesn't have flags defined
set(DEFAULT_CXX_FLAGS)
set(DEFAULT_C_FLAGS)

set(BUILD_CONFIGS)

foreach (game_type ${GAME_TYPES})
    foreach (platform_type ${PLATFORM_TYPES})
        foreach (shipping_type ${SHIPPING_TYPES})
            set(triplet ${game_type}_${platform_type}_${shipping_type})
            list(APPEND BUILD_CONFIGS ${triplet})

            set(definitions
                    ${${game_type}_DEFINITIONS}
                    ${${platform_type}_DEFINITIONS}
                    ${${shipping_type}_DEFINITIONS})
            add_compile_definitions("$<$<STREQUAL:$<CONFIG>,${triplet}>:${definitions}>")

            if (NOT DEFINED CMAKE_SHARED_LINKER_FLAGS_${triplet})
                set(CMAKE_SHARED_LINKER_FLAGS_${triplet} ${DEFAULT_SHARED_LINKER_FLAGS})
            endif ()

            if (NOT DEFINED CMAKE_EXE_LINKER_FLAGS_${triplet})
                set(CMAKE_EXE_LINKER_FLAGS_${triplet} ${DEFAULT_EXE_LINKER_FLAGS})
            endif ()

            if (NOT DEFINED CMAKE_CXX_FLAGS_${triplet})
                set(CMAKE_CXX_FLAGS_${triplet} ${DEFAULT_CXX_FLAGS})
            endif ()

            if (NOT DEFINED CMAKE_C_FLAGS_${triplet})
                set(CMAKE_C_FLAGS_${triplet} ${DEFAULT_C_FLAGS})
            endif ()
        endforeach ()
    endforeach ()
endforeach ()

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if (is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES ${BUILD_CONFIGS} CACHE STRING "" FORCE)
else ()
    if (NOT CMAKE_BUILD_TYPE)
        message("Defaulting to Game_Win64_Shipping")
        set(CMAKE_BUILD_TYPE Game_Win64_Shipping CACHE STRING "" FORCE)
    endif ()

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_CONFIGS})
endif ()

# Build configurations -> END