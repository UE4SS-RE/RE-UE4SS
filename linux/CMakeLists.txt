cmake_minimum_required(VERSION 3.22)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "The UE4SS Linux bootstrap can only be built on Linux")
endif()

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "The initial UE4SS Linux port supports x86_64 only")
endif()

include(GNUInstallDirs)
include(FetchContent)
find_package(Threads REQUIRED)

set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    ue4ss_linux_fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ue4ss_linux_fmt)
set_property(DIRECTORY "${ue4ss_linux_fmt_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL YES)

option(UE4SS_HEADLESS "Build UE4SS without graphical interfaces" ON)
option(UE4SS_WITH_GUI "Build the UE4SS GUI" OFF)
option(UE4SS_WITH_INPUT "Build the UE4SS input subsystem" OFF)
option(UE4SS_WITH_CRASH_DUMPER "Build the platform crash dumper" OFF)
option(UE4SS_WITH_CPP_MODS "Build support for native C++ mods" OFF)
option(UE4SS_WITH_SDK_GENERATOR "Build the SDK generator" OFF)
option(UE4SS_WITH_UVTD "Build UVTD" OFF)
option(UE4SS_WITH_HOOK_BACKEND "Build the Linux x86_64 inline-hook backend" ON)
option(UE4SS_WITH_PATTERNSLEUTH "Build the Linux ELF patternsleuth resolver bridge" ON)
option(UE4SS_WITH_UNREAL_ABI "Compile-check the UEPseudo Linux ABI platform layer" ON)
option(UE4SS_WITH_UNREAL_SOURCE_GATE "Compile the portable UEPseudo reflection source subset" ON)
option(UE4SS_WITH_UNREAL_RUNTIME "Bind validated resolver results to UEPseudo state" ON)
option(UE4SS_WITH_LUA_RUNTIME "Build the isolated headless Lua VM" ON)
option(UE4SS_USE_LIBCXX "Build the isolated Linux core with libc++" ON)

if(UE4SS_WITH_LUA_RUNTIME AND NOT UE4SS_WITH_HOOK_BACKEND)
    message(FATAL_ERROR "The Linux Lua runtime currently requires UE4SS_WITH_HOOK_BACKEND=ON for safe game-thread dispatch")
endif()

if(UE4SS_USE_LIBCXX)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "UE4SS_USE_LIBCXX=ON requires Clang")
    endif()
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
    add_link_options($<$<LINK_LANGUAGE:CXX>:-stdlib=libc++> $<$<LINK_LANGUAGE:CXX>:-static-libstdc++>)
    set(UE4SS_LINUX_CXX_RUNTIME "libc++-static")
else()
    set(UE4SS_LINUX_CXX_RUNTIME "compiler-default")
endif()

foreach(unsupported_option
        UE4SS_WITH_GUI
        UE4SS_WITH_INPUT
        UE4SS_WITH_CRASH_DUMPER
        UE4SS_WITH_CPP_MODS
        UE4SS_WITH_SDK_GENERATOR
        UE4SS_WITH_UVTD)
    if(${unsupported_option})
        message(FATAL_ERROR "${unsupported_option}=ON is not implemented by the Linux bootstrap")
    endif()
endforeach()

if(NOT UE4SS_HEADLESS)
    message(FATAL_ERROR "The initial Linux port requires UE4SS_HEADLESS=ON")
endif()

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE UE4SS_LINUX_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
set(UE4SS_LINUX_GIT_SHA_OVERRIDE "" CACHE STRING "Override the package git SHA for source archives and container builds")
if(UE4SS_LINUX_GIT_SHA_OVERRIDE)
    set(UE4SS_LINUX_GIT_SHA "${UE4SS_LINUX_GIT_SHA_OVERRIDE}")
endif()
if(NOT UE4SS_LINUX_GIT_SHA)
    set(UE4SS_LINUX_GIT_SHA "unknown")
endif()

set(UE4SS_LINUX_VERSION_MAJOR 0)
set(UE4SS_LINUX_VERSION_MINOR 0)
set(UE4SS_LINUX_VERSION_HOTFIX 0)
set(UE4SS_VERSION_CACHE "${CMAKE_SOURCE_DIR}/UE4SS/generated_src/version.cache")
if(EXISTS "${UE4SS_VERSION_CACHE}")
    file(READ "${UE4SS_VERSION_CACHE}" UE4SS_LINUX_VERSION_TEXT)
    string(REGEX MATCH
        "^([0-9]+)\\.([0-9]+)\\.([0-9]+)"
        UE4SS_LINUX_VERSION_MATCH
        "${UE4SS_LINUX_VERSION_TEXT}")
    if(UE4SS_LINUX_VERSION_MATCH)
        set(UE4SS_LINUX_VERSION_MAJOR "${CMAKE_MATCH_1}")
        set(UE4SS_LINUX_VERSION_MINOR "${CMAKE_MATCH_2}")
        set(UE4SS_LINUX_VERSION_HOTFIX "${CMAKE_MATCH_3}")
    endif()
endif()

set(UE4SS_LINUX_COMMON_SOURCES
    src/case_path.cpp
    src/elf_image.cpp
    src/mod_catalog.cpp
    src/sha256.c
    src/utf.cpp
)

add_library(ue4ss_linux_platform STATIC ${UE4SS_LINUX_COMMON_SOURCES})
set_target_properties(ue4ss_linux_platform PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(ue4ss_linux_platform PUBLIC cxx_std_23)
target_include_directories(ue4ss_linux_platform
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_definitions(ue4ss_linux_platform PUBLIC
    PLATFORM_LINUX=1
    PLATFORM_UNIX=1
    UE4SS_HEADLESS=1
    UE4SS_WITH_GUI=0
    UE4SS_WITH_INPUT=0
    UE4SS_WITH_CRASH_DUMPER=0
    UE4SS_WITH_CPP_MODS=0
    UE4SS_WITH_SDK_GENERATOR=0
    UE4SS_WITH_UVTD=0
    UE4SS_WITH_HOOK_BACKEND=$<BOOL:${UE4SS_WITH_HOOK_BACKEND}>
    UE4SS_WITH_PATTERNSLEUTH=$<BOOL:${UE4SS_WITH_PATTERNSLEUTH}>
    UE4SS_WITH_UNREAL_ABI=$<BOOL:${UE4SS_WITH_UNREAL_ABI}>
    UE4SS_WITH_UNREAL_SOURCE_GATE=$<BOOL:${UE4SS_WITH_UNREAL_SOURCE_GATE}>
    UE4SS_WITH_UNREAL_RUNTIME=$<BOOL:${UE4SS_WITH_UNREAL_RUNTIME}>
    UE4SS_WITH_LUA_RUNTIME=$<BOOL:${UE4SS_WITH_LUA_RUNTIME}>
    UE4SS_USE_LIBCXX=$<BOOL:${UE4SS_USE_LIBCXX}>
    UE4SS_LINUX_VERSION_MAJOR=${UE4SS_LINUX_VERSION_MAJOR}
    UE4SS_LINUX_VERSION_MINOR=${UE4SS_LINUX_VERSION_MINOR}
    UE4SS_LINUX_VERSION_HOTFIX=${UE4SS_LINUX_VERSION_HOTFIX}
)

if(UE4SS_WITH_LUA_RUNTIME)
    set(UE4SS_LUA_RAW_DIR "${CMAKE_SOURCE_DIR}/deps/first/LuaRaw")
    file(GLOB UE4SS_LUA_RAW_SOURCES CONFIGURE_DEPENDS "${UE4SS_LUA_RAW_DIR}/src/*.c")
    list(FILTER UE4SS_LUA_RAW_SOURCES EXCLUDE REGEX "/luac?\\.c$")
    add_library(ue4ss_lua_raw STATIC ${UE4SS_LUA_RAW_SOURCES})
    set_target_properties(ue4ss_lua_raw PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(ue4ss_lua_raw PUBLIC "${UE4SS_LUA_RAW_DIR}/include")
    target_compile_definitions(ue4ss_lua_raw PUBLIC LUA_USE_LINUX=1 RC_LUA_RAW_BUILD_STATIC=1)
    target_compile_definitions(ue4ss_lua_raw PRIVATE _POSIX_C_SOURCE=200809L)
    target_link_libraries(ue4ss_lua_raw PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

if(UE4SS_WITH_HOOK_BACKEND)
    set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZYDIS_BUILD_DOXYGEN OFF CACHE BOOL "" FORCE)
    set(ZYDIS_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(ZYAN_NO_LIBC OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        ue4ss_linux_zydis
        GIT_REPOSITORY https://github.com/zyantific/zydis.git
        GIT_TAG a2278f1d254e492f6a6b39f6cb5d1f5d515659dc # v4.1.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(ue4ss_linux_zydis)
    # Zydis is linked privately. Exclude its development archive, headers, and
    # CMake package metadata from the UE4SS runtime installer.
    set_property(DIRECTORY "${ue4ss_linux_zydis_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL YES)

    add_library(ue4ss_linux_hooks STATIC src/inline_hook.cpp)
    set_target_properties(ue4ss_linux_hooks PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_features(ue4ss_linux_hooks PUBLIC cxx_std_23)
    target_include_directories(ue4ss_linux_hooks PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_definitions(ue4ss_linux_hooks PUBLIC PLATFORM_LINUX=1 PLATFORM_UNIX=1)
    target_link_libraries(ue4ss_linux_hooks PRIVATE Zydis)
endif()

if(UE4SS_WITH_PATTERNSLEUTH)
    find_program(CARGO_EXECUTABLE cargo REQUIRED)
    set(PATTERNSLEUTH_BIND_DIR "${CMAKE_SOURCE_DIR}/deps/first/patternsleuth_bind")
    set(PATTERNSLEUTH_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/first/patternsleuth/patternsleuth")
    if(NOT EXISTS "${PATTERNSLEUTH_SOURCE_DIR}/Cargo.toml")
        message(FATAL_ERROR
            "patternsleuth submodule is missing. Run: git submodule update --init deps/first/patternsleuth")
    endif()
    set(PATTERNSLEUTH_CARGO_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo")
    set(PATTERNSLEUTH_LIBRARY "${PATTERNSLEUTH_CARGO_TARGET_DIR}/release/libpatternsleuth_bind.a")
    file(GLOB_RECURSE PATTERNSLEUTH_BIND_RUST_SOURCES CONFIGURE_DEPENDS
        "${PATTERNSLEUTH_BIND_DIR}/src/*.rs"
    )
    file(GLOB_RECURSE PATTERNSLEUTH_RUST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/deps/first/patternsleuth/patternsleuth*/src/*.rs"
    )
    add_custom_command(
        OUTPUT "${PATTERNSLEUTH_LIBRARY}"
        COMMAND "${CMAKE_COMMAND}" -E env "CARGO_TARGET_DIR=${PATTERNSLEUTH_CARGO_TARGET_DIR}"
            "${CARGO_EXECUTABLE}" build --release --locked
            --manifest-path "${PATTERNSLEUTH_BIND_DIR}/Cargo.toml"
        DEPENDS
            "${PATTERNSLEUTH_BIND_DIR}/Cargo.toml"
            "${PATTERNSLEUTH_BIND_DIR}/Cargo.lock"
            ${PATTERNSLEUTH_BIND_RUST_SOURCES}
            ${PATTERNSLEUTH_RUST_SOURCES}
        WORKING_DIRECTORY "${PATTERNSLEUTH_BIND_DIR}"
        COMMENT "Building the patternsleuth Linux ELF bridge"
        VERBATIM
    )
    add_custom_target(patternsleuth_bind_linux_build DEPENDS "${PATTERNSLEUTH_LIBRARY}")
    add_library(patternsleuth_bind_linux STATIC IMPORTED GLOBAL)
    set_target_properties(patternsleuth_bind_linux PROPERTIES IMPORTED_LOCATION "${PATTERNSLEUTH_LIBRARY}")
    add_dependencies(patternsleuth_bind_linux patternsleuth_bind_linux_build)
endif()

add_library(ue4ss_preload SHARED preload/preload.c src/sha256.c)
set_target_properties(ue4ss_preload PROPERTIES
    OUTPUT_NAME "ue4ss_preload"
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
target_include_directories(ue4ss_preload PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ue4ss_preload PRIVATE ${CMAKE_DL_LIBS})
target_link_options(ue4ss_preload PRIVATE "LINKER:-z,defs" "LINKER:-z,relro" "LINKER:-z,now")

add_library(UE4SS SHARED
    core/entry.cpp
    core/object_registry_validator.cpp
    core/pattern_resolver.cpp
    core/status.cpp
    core/unreal_readonly.cpp
)
set_target_properties(UE4SS PROPERTIES
    OUTPUT_NAME "UE4SS"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
target_compile_features(UE4SS PRIVATE cxx_std_23)
target_compile_definitions(UE4SS PRIVATE
    UE4SS_BUILDING_CORE=1
    UE4SS_LINUX_GIT_SHA="${UE4SS_LINUX_GIT_SHA}"
)
target_compile_definitions(UE4SS PUBLIC
    PLATFORM_LINUX=1
    PLATFORM_UNIX=1
    UE4SS_HEADLESS=1
    UE4SS_WITH_GUI=0
    UE4SS_WITH_INPUT=0
    UE4SS_WITH_CRASH_DUMPER=0
    UE4SS_WITH_CPP_MODS=0
    UE4SS_WITH_SDK_GENERATOR=0
    UE4SS_WITH_UVTD=0
    UE4SS_WITH_HOOK_BACKEND=$<BOOL:${UE4SS_WITH_HOOK_BACKEND}>
    UE4SS_WITH_PATTERNSLEUTH=$<BOOL:${UE4SS_WITH_PATTERNSLEUTH}>
    UE4SS_WITH_UNREAL_ABI=$<BOOL:${UE4SS_WITH_UNREAL_ABI}>
    UE4SS_WITH_UNREAL_SOURCE_GATE=$<BOOL:${UE4SS_WITH_UNREAL_SOURCE_GATE}>
    UE4SS_WITH_UNREAL_RUNTIME=$<BOOL:${UE4SS_WITH_UNREAL_RUNTIME}>
    UE4SS_WITH_LUA_RUNTIME=$<BOOL:${UE4SS_WITH_LUA_RUNTIME}>
    UE4SS_USE_LIBCXX=$<BOOL:${UE4SS_USE_LIBCXX}>
)
target_include_directories(UE4SS
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(UE4SS PRIVATE ue4ss_linux_platform Threads::Threads ${CMAKE_DL_LIBS})
if(UE4SS_WITH_HOOK_BACKEND)
    target_link_libraries(UE4SS PRIVATE ue4ss_linux_hooks)
    target_sources(UE4SS PRIVATE
        core/game_thread_scheduler.cpp
        core/ufunction_hooks.cpp
        core/object_notifications.cpp
        core/blueprint_hooks.cpp
        core/begin_play_hooks.cpp
        core/load_map_hooks.cpp
        core/console_exec_hooks.cpp
    )
endif()
if(UE4SS_WITH_PATTERNSLEUTH)
    target_link_libraries(UE4SS PRIVATE patternsleuth_bind_linux m ${CMAKE_DL_LIBS} Threads::Threads)
endif()
if(UE4SS_WITH_LUA_RUNTIME)
    target_sources(UE4SS PRIVATE core/headless_lua.cpp)
    target_link_libraries(UE4SS PRIVATE ue4ss_lua_raw)
endif()
target_link_options(UE4SS PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/core/exports.map"
    "LINKER:-Bsymbolic-functions"
    "LINKER:-z,defs"
    "LINKER:-z,relro"
    "LINKER:-z,now"
)

add_executable(ue4ss_probe tools/probe.cpp)
target_compile_features(ue4ss_probe PRIVATE cxx_std_23)
target_link_libraries(ue4ss_probe PRIVATE ue4ss_linux_platform)

if(UE4SS_WITH_PATTERNSLEUTH)
    add_executable(ue4ss_resolver_probe tools/resolver_probe.cpp)
    target_compile_features(ue4ss_resolver_probe PRIVATE cxx_std_23)
    target_link_libraries(ue4ss_resolver_probe PRIVATE ue4ss_linux_platform patternsleuth_bind_linux m ${CMAKE_DL_LIBS} Threads::Threads)
endif()

if(UE4SS_WITH_UNREAL_ABI OR UE4SS_WITH_UNREAL_SOURCE_GATE OR UE4SS_WITH_UNREAL_RUNTIME)
    set(UE4SS_UNREAL_DIR "${CMAKE_SOURCE_DIR}/deps/first/Unreal")
    if(NOT EXISTS "${UE4SS_UNREAL_DIR}/include/Unreal/Core/HAL/Platform.hpp")
        message(FATAL_ERROR
            "UEPseudo is missing. Initialize deps/first/Unreal with an Epic-linked GitHub account")
    endif()

    # Every UEPseudo consumer must see exactly the same platform and feature
    # definitions. Keeping these PUBLIC on one interface prevents layout drift
    # between the compile gate, the ABI tests, and the future runtime target.
    add_library(ue4ss_unreal_linux_config INTERFACE)
    target_compile_features(ue4ss_unreal_linux_config INTERFACE cxx_std_23)
    target_include_directories(ue4ss_unreal_linux_config INTERFACE
        "${UE4SS_UNREAL_DIR}/include"
        "${UE4SS_UNREAL_DIR}/generated_include"
        "${UE4SS_UNREAL_DIR}/include/Unreal"
        "${UE4SS_UNREAL_DIR}/include/Unreal/Core"
        "${CMAKE_SOURCE_DIR}/deps/first/File/include"
        "${CMAKE_SOURCE_DIR}/deps/first/String/include"
        "${CMAKE_SOURCE_DIR}/deps/first/Function/include"
        "${CMAKE_SOURCE_DIR}/deps/first/Helpers/include"
        "${CMAKE_SOURCE_DIR}/deps/first/Constructs/include"
        "${CMAKE_SOURCE_DIR}/deps/first/DynamicOutput/include"
        "${CMAKE_SOURCE_DIR}/deps/first/SinglePassSigScanner/include"
        "${CMAKE_SOURCE_DIR}/deps/first/ASMHelper/include"
        "${CMAKE_SOURCE_DIR}/deps/first/Input/include"
    )
    target_compile_definitions(ue4ss_unreal_linux_config INTERFACE
        PLATFORM_LINUX=1
        PLATFORM_UNIX=1
        PLATFORM_WINDOWS=0
        PLATFORM_MICROSOFT=0
        OVERRIDE_PLATFORM_HEADER_NAME=Linux
        UBT_COMPILED_PLATFORM=Linux
        UE_BUILD_DEBUG=0
        UE_BUILD_DEVELOPMENT=0
        UE_BUILD_TEST=0
        UE_BUILD_SHIPPING=1
        UE_SERVER=1
        RC_UNREAL_BUILD_STATIC=1
        RC_FILE_BUILD_STATIC=1
        RC_DYNAMIC_OUTPUT_BUILD_STATIC=1
        RC_SINGLE_PASS_SIG_SCANNER_BUILD_STATIC=1
        RC_ASM_HELPER_BUILD_STATIC=1
        RC_INPUT_BUILD_STATIC=1
    )
    if(UE4SS_WITH_HOOK_BACKEND)
        target_link_libraries(ue4ss_unreal_linux_config INTERFACE Zydis)
    endif()
    target_link_libraries(ue4ss_unreal_linux_config INTERFACE fmt::fmt-header-only)
endif()

if(UE4SS_WITH_UNREAL_RUNTIME)
    target_sources(UE4SS PRIVATE
        core/unreal_runtime.cpp
        core/unreal_container_bridge.cpp
        "${UE4SS_UNREAL_DIR}/src/ResolverState.cpp"
    )
    target_link_libraries(UE4SS PRIVATE ue4ss_unreal_linux_config)
endif()

if(UE4SS_WITH_UNREAL_SOURCE_GATE)
    file(GLOB UE4SS_UNREAL_VERSIONED_SOURCES CONFIGURE_DEPENDS
        "${UE4SS_UNREAL_DIR}/src/VersionedContainer/*.cpp"
        "${UE4SS_UNREAL_DIR}/src/VersionedContainer/UnrealVirtualImpl/*.cpp"
    )
    add_library(ue4ss_unreal_source_gate OBJECT
        "${CMAKE_SOURCE_DIR}/deps/first/Helpers/src/Casting.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Containers/BitArray.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Containers/ContainerAllocationPolicies.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Containers/ContainerHelpers.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Containers/FAnsiString.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Containers/FString.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Containers/FUtf8String.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/GenericPlatform/GenericPlatformMath.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/GenericPlatform/GenericPlatformStricmp.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/GenericPlatform/GenericPlatformString.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/HAL/UnrealMemory.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Misc/CString.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Misc/CoreMisc.cpp"
        "${UE4SS_UNREAL_DIR}/src/Core/Misc/Crc.cpp"
        "${UE4SS_UNREAL_DIR}/src/FArchive.cpp"
        "${UE4SS_UNREAL_DIR}/src/FMemory.cpp"
        "${UE4SS_UNREAL_DIR}/src/FWeakObjectPtr.cpp"
        "${UE4SS_UNREAL_DIR}/src/ResolverState.cpp"
        "${UE4SS_UNREAL_DIR}/src/UnrealVersion.cpp"
        "${UE4SS_UNREAL_DIR}/src/NameTypes.cpp"
        "${UE4SS_UNREAL_DIR}/src/PackageName.cpp"
        "${UE4SS_UNREAL_DIR}/src/Searcher/ObjectSearcher.cpp"
        "${UE4SS_UNREAL_DIR}/src/ClassListener.cpp"
        "${UE4SS_UNREAL_DIR}/src/TypeChecker.cpp"
        "${UE4SS_UNREAL_DIR}/src/UObject.cpp"
        "${UE4SS_UNREAL_DIR}/src/UObjectArray.cpp"
        "${UE4SS_UNREAL_DIR}/src/UObjectGlobals.cpp"
        "${UE4SS_UNREAL_DIR}/src/UClass.cpp"
        "${UE4SS_UNREAL_DIR}/src/UEnum.cpp"
        "${UE4SS_UNREAL_DIR}/src/UField.cpp"
        "${UE4SS_UNREAL_DIR}/src/UFunction.cpp"
        "${UE4SS_UNREAL_DIR}/src/UFunctionStructs.cpp"
        "${UE4SS_UNREAL_DIR}/src/UPackage.cpp"
        "${UE4SS_UNREAL_DIR}/src/UScriptStruct.cpp"
        "${UE4SS_UNREAL_DIR}/src/UStruct.cpp"
        "${UE4SS_UNREAL_DIR}/src/FField.cpp"
        "${UE4SS_UNREAL_DIR}/src/FProperty.cpp"
        ${UE4SS_UNREAL_VERSIONED_SOURCES}
    )
    set_target_properties(ue4ss_unreal_source_gate PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(ue4ss_unreal_source_gate PRIVATE ue4ss_unreal_linux_config)
endif()

if(BUILD_TESTING)
    set(UE4SS_INVALID_SHA256 "0000000000000000000000000000000000000000000000000000000000000000")
    add_executable(ue4ss_linux_unit_tests tests/unit_tests.cpp)
    target_compile_features(ue4ss_linux_unit_tests PRIVATE cxx_std_23)
    target_link_libraries(ue4ss_linux_unit_tests PRIVATE ue4ss_linux_platform)
    if(UE4SS_WITH_HOOK_BACKEND)
        target_link_libraries(ue4ss_linux_unit_tests PRIVATE ue4ss_linux_hooks)
        target_compile_definitions(ue4ss_linux_unit_tests PRIVATE UE4SS_WITH_HOOK_BACKEND=1)
    endif()
    add_test(NAME ue4ss_linux_unit_tests COMMAND ue4ss_linux_unit_tests)

    if(UE4SS_WITH_UNREAL_ABI)
        add_executable(ue4ss_unreal_abi_compile tests/unreal_abi_compile.cpp)
        target_link_libraries(ue4ss_unreal_abi_compile PRIVATE ue4ss_unreal_linux_config)
        add_test(NAME ue4ss_unreal_abi_compile COMMAND ue4ss_unreal_abi_compile)
    endif()

    if(UE4SS_WITH_UNREAL_RUNTIME)
        add_executable(ue4ss_unreal_resolver_state_test
            tests/unreal_resolver_state_test.cpp
            core/object_registry_validator.cpp
            core/unreal_runtime.cpp
            "${UE4SS_UNREAL_DIR}/src/ResolverState.cpp"
        )
        target_include_directories(ue4ss_unreal_resolver_state_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
        target_link_libraries(ue4ss_unreal_resolver_state_test PRIVATE ue4ss_unreal_linux_config ue4ss_linux_platform)
        add_test(NAME ue4ss_unreal_resolver_state_test COMMAND ue4ss_unreal_resolver_state_test)

        add_executable(ue4ss_unreal_readonly_test
            tests/unreal_readonly_test.cpp
            core/unreal_readonly.cpp
            core/unreal_container_bridge.cpp
        )
        target_include_directories(ue4ss_unreal_readonly_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
        target_link_libraries(ue4ss_unreal_readonly_test PRIVATE ue4ss_linux_platform ue4ss_unreal_linux_config)
        if(UE4SS_WITH_LUA_RUNTIME)
            target_compile_definitions(ue4ss_unreal_readonly_test PRIVATE UE4SS_LINUX_TESTING=1)
            target_sources(ue4ss_unreal_readonly_test PRIVATE
                core/headless_lua.cpp
                core/game_thread_scheduler.cpp
                core/ufunction_hooks.cpp
                core/object_notifications.cpp
                core/blueprint_hooks.cpp
                core/begin_play_hooks.cpp
                core/load_map_hooks.cpp
                core/console_exec_hooks.cpp
            )
            target_link_libraries(ue4ss_unreal_readonly_test PRIVATE ue4ss_lua_raw ue4ss_linux_hooks Zydis)
        endif()
        add_test(NAME ue4ss_unreal_readonly_test COMMAND ue4ss_unreal_readonly_test)
    endif()

    if(UE4SS_WITH_LUA_RUNTIME)
        add_executable(ue4ss_headless_lua_test
            tests/headless_lua_test.cpp
            core/headless_lua.cpp
            core/unreal_readonly.cpp
            core/game_thread_scheduler.cpp
            core/ufunction_hooks.cpp
            core/object_notifications.cpp
            core/blueprint_hooks.cpp
            core/begin_play_hooks.cpp
            core/load_map_hooks.cpp
            core/console_exec_hooks.cpp
            core/unreal_container_bridge.cpp
        )
        target_compile_features(ue4ss_headless_lua_test PRIVATE cxx_std_23)
        target_include_directories(ue4ss_headless_lua_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/core"
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(ue4ss_headless_lua_test PRIVATE ue4ss_lua_raw ue4ss_linux_platform ue4ss_linux_hooks Zydis ue4ss_unreal_linux_config)
        add_test(NAME ue4ss_headless_lua_test COMMAND ue4ss_headless_lua_test)
    endif()

    if(UE4SS_WITH_HOOK_BACKEND)
        add_executable(ue4ss_game_thread_scheduler_test
            tests/game_thread_scheduler_test.cpp
            core/game_thread_scheduler.cpp
            core/unreal_readonly.cpp
            core/unreal_container_bridge.cpp
        )
        target_include_directories(ue4ss_game_thread_scheduler_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
        target_link_libraries(ue4ss_game_thread_scheduler_test PRIVATE ue4ss_linux_platform ue4ss_linux_hooks ue4ss_unreal_linux_config)
        add_test(NAME ue4ss_game_thread_scheduler_test COMMAND ue4ss_game_thread_scheduler_test)
    endif()

    add_executable(ue4ss_preload_smoke tests/preload_smoke.c)
    add_test(
        NAME ue4ss_docker_runtime_test
        COMMAND bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/docker_runtime_test.sh"
            "${CMAKE_CURRENT_SOURCE_DIR}/docker/palserver-entrypoint.sh"
            "${CMAKE_CURRENT_SOURCE_DIR}/docker/ue4ss-healthcheck.sh"
    )
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-state")
    add_test(
        NAME ue4ss_preload_smoke
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_STATE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-state"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_preload_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "UE4SS preload smoke application"
    )

    add_test(
        NAME ue4ss_preload_fail_open
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=${CMAKE_CURRENT_BINARY_DIR}/missing-core"
            "UE4SS_REQUIRED=false"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_preload_fail_open PROPERTIES
        PASS_REGULAR_EXPRESSION "starting the application in vanilla mode"
    )

    add_test(
        NAME ue4ss_preload_fail_closed
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=${CMAKE_CURRENT_BINARY_DIR}/missing-core"
            "UE4SS_REQUIRED=true"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_preload_fail_closed PROPERTIES
        WILL_FAIL TRUE
    )

    add_test(
        NAME ue4ss_core_hash_fail_open
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_EXPECTED_SHA256=${UE4SS_INVALID_SHA256}"
            "UE4SS_REQUIRED=false"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_core_hash_fail_open PROPERTIES
        PASS_REGULAR_EXPRESSION "starting the application in vanilla mode"
    )

    add_test(
        NAME ue4ss_core_hash_fail_closed
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_EXPECTED_SHA256=${UE4SS_INVALID_SHA256}"
            "UE4SS_REQUIRED=true"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_core_hash_fail_closed PROPERTIES
        WILL_FAIL TRUE
    )

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-server-hash-fail-open")
    add_test(
        NAME ue4ss_server_hash_fail_open
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_STATE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-server-hash-fail-open"
            "UE4SS_SERVER_SHA256=${UE4SS_INVALID_SHA256}"
            "UE4SS_REQUIRED=false"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_server_hash_fail_open PROPERTIES
        PASS_REGULAR_EXPRESSION "server SHA-256 does not match"
    )

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-server-hash-fail-closed")
    add_test(
        NAME ue4ss_server_hash_fail_closed
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_STATE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-server-hash-fail-closed"
            "UE4SS_SERVER_SHA256=${UE4SS_INVALID_SHA256}"
            "UE4SS_REQUIRED=true"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_server_hash_fail_closed PROPERTIES
        WILL_FAIL TRUE
    )

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-scan-fail-open")
    add_test(
        NAME ue4ss_resolver_fail_open
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_STATE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-scan-fail-open"
            "UE4SS_SCAN_UNREAL=true"
            "UE4SS_REQUIRED=false"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_resolver_fail_open PROPERTIES
        PASS_REGULAR_EXPRESSION "critical Unreal ELF resolvers are unavailable"
    )

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-scan-fail-closed")
    add_test(
        NAME ue4ss_resolver_fail_closed
        COMMAND "${CMAKE_COMMAND}" -E env
            "LD_PRELOAD=$<TARGET_FILE:ue4ss_preload>"
            "UE4SS_ROOT=$<TARGET_FILE_DIR:UE4SS>"
            "UE4SS_STATE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-scan-fail-closed"
            "UE4SS_SCAN_UNREAL=true"
            "UE4SS_REQUIRED=true"
            "$<TARGET_FILE:ue4ss_preload_smoke>"
    )
    set_tests_properties(ue4ss_resolver_fail_closed PROPERTIES
        WILL_FAIL TRUE
    )

    add_test(
        NAME ue4ss_probe_self
        COMMAND ue4ss_probe --json "$<TARGET_FILE:ue4ss_probe>"
    )
    set_tests_properties(ue4ss_probe_self PROPERTIES
        PASS_REGULAR_EXPRESSION "\"elf_class\":64"
    )

    if(UE4SS_WITH_PATTERNSLEUTH)
        add_test(
            NAME ue4ss_resolver_probe_self
            COMMAND ue4ss_resolver_probe --json "$<TARGET_FILE:ue4ss_resolver_probe>"
        )
        set_tests_properties(ue4ss_resolver_probe_self PROPERTIES
            PASS_REGULAR_EXPRESSION "\"scan_error\": 0"
        )
    endif()

    # CMake's Release flags define NDEBUG. These executables intentionally
    # use assert as their test oracle (and, historically, around setup calls),
    # so compiling assertions out would turn the release suite into no-ops and
    # can leave coordination threads waiting for setup that never ran.
    foreach(UE4SS_ASSERT_TEST_TARGET IN ITEMS
            ue4ss_linux_unit_tests
            ue4ss_unreal_abi_compile
            ue4ss_unreal_resolver_state_test
            ue4ss_unreal_readonly_test
            ue4ss_headless_lua_test
            ue4ss_game_thread_scheduler_test)
        if(TARGET ${UE4SS_ASSERT_TEST_TARGET})
            target_compile_options(${UE4SS_ASSERT_TEST_TARGET} PRIVATE -UNDEBUG)
        endif()
    endforeach()
endif()

set(UE4SS_BUILD_UNREAL_ABI false)
set(UE4SS_BUILD_UNREAL_SOURCE_GATE false)
set(UE4SS_BUILD_UNREAL_RUNTIME false)
set(UE4SS_BUILD_LUA_RUNTIME false)
if(UE4SS_WITH_UNREAL_ABI)
    set(UE4SS_BUILD_UNREAL_ABI true)
endif()
if(UE4SS_WITH_UNREAL_SOURCE_GATE)
    set(UE4SS_BUILD_UNREAL_SOURCE_GATE true)
endif()
if(UE4SS_WITH_UNREAL_RUNTIME)
    set(UE4SS_BUILD_UNREAL_RUNTIME true)
endif()
if(UE4SS_WITH_LUA_RUNTIME)
    set(UE4SS_BUILD_LUA_RUNTIME true)
endif()

configure_file(packaging/UE4SS-settings.ini.in UE4SS-settings.ini @ONLY)
configure_file(packaging/ue4ss-build.json.in ue4ss-build.json @ONLY)

set(UE4SS_LINUX_INSTALL_TARGETS UE4SS ue4ss_preload ue4ss_probe)
if(UE4SS_WITH_PATTERNSLEUTH)
    list(APPEND UE4SS_LINUX_INSTALL_TARGETS ue4ss_resolver_probe)
endif()
install(TARGETS ${UE4SS_LINUX_INSTALL_TARGETS}
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/UE4SS-settings.ini"
    "${CMAKE_CURRENT_BINARY_DIR}/ue4ss-build.json"
    DESTINATION .
)
install(DIRECTORY packaging/Mods/ DESTINATION Mods)

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "UE4SS-Linux-x86_64")
set(CPACK_PACKAGE_VERSION "0.2.0")
set(CPACK_PACKAGE_FILE_NAME "UE4SS-Linux-x86_64-${UE4SS_LINUX_GIT_SHA}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
