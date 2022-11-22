cmake_minimum_required(VERSION 3.18)
enable_language(CXX)
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
message("IPO - Supported: ${supported}; ${error}")
message("Project: UE4SS")

set(MAKE_DEPENDENCIES_SHARED 0)

if (${MAKE_DEPENDENCIES_SHARED} EQUAL 0)
    set(MAKE_DEPENDENCIES_STATIC 1)
    set(DEPENDENCY_BUILD_TYPE STATIC)
    set(BUILD_SHARED_LIBS 0)
else ()
    set(MAKE_DEPENDENCIES_STATIC 0)
    set(DEPENDENCY_BUILD_TYPE SHARED)
    set(BUILD_SHARED_LIBS 1)
endif ()

# We're only using C++20 features but we need to use C++23 so that we get std::format which only exists in the "latest" flag in MSVC
set(CMAKE_CXX_STANDARD 23)

set(TARGET ue4ss)
project(${TARGET})

# Release Settings -> START
set(UE4SS_LIB_BETA_STARTED 1)           # Whether or not beta releases have started for the current major version
set(UE4SS_LIB_IS_BETA 1)                # Whether or not this is a beta release
# Release Settings -> END

# Settings -> START
set(UE4SS_CONSOLE_COLORS_ENABLED 0)
set(FORCE_WITH_CASE_PRESERVING_NAME 0)
set(UE4SS_GENERATED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/generated_include")
set(UE4SS_GENERATED_SOURCE_DIR "${PROJECT_SOURCE_DIR}/generated_src")
set(LLVM_PATH "D:\\LLVM\\14.0.6\\llvm+clang-14.0.6-x86_64-windows-msvc-release-mt")
set(UE4SS_GENERATE_LUA_BINDINGS 0)
set(BUILD_SCRIPT_DIR ${CMAKE_HOME_DIRECTORY})
# Settings -> END

# Output path -> START
set(UE4SS_OUTPUT_DEBUG_DIR ${CMAKE_SOURCE_DIR}/Output/${PROJECT_NAME}/Binaries/x64/Debug)
set(UE4SS_OUTPUT_RELEASE_DIR ${CMAKE_SOURCE_DIR}/Output/${PROJECT_NAME}/Binaries/x64/Release)
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    set(LIBRARY_OUTPUT_PATH ${UE4SS_OUTPUT_DEBUG_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UE4SS_OUTPUT_DEBUG_DIR})
elseif (${CMAKE_BUILD_TYPE} MATCHES Release)
    set(LIBRARY_OUTPUT_PATH ${UE4SS_OUTPUT_RELEASE_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UE4SS_OUTPUT_RELEASE_DIR})
else ()
    set(LIBRARY_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/Output/${PROJECT_NAME}/Binaries/x64)
    message("Unknown build type. (Build type: ${CMAKE_BUILD_TYPE})")
endif ()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${UE4SS_OUTPUT_DEBUG_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${UE4SS_OUTPUT_DEBUG_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${UE4SS_OUTPUT_RELEASE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${UE4SS_OUTPUT_RELEASE_DIR})
# Output path -> END

set(PRIVATE_COMPILE_DEFINITIONS
        $<${MAKE_DEPENDENCIES_SHARED}:RC_EXPORT RC_FUNCTION_EXPORTS>
        $<${MAKE_DEPENDENCIES_STATIC}:RC_FUNCTION_BUILD_STATIC>)
set(PRIVATE_COMPILE_OPTIONS /MP $<$<CONFIG:RELEASE>:/Zi> /W3 /wd4005 /wd4251)
set(PRIVATE_MASM_COMPILE_OPTIONS /MP $<$<CONFIG:RELEASE>:/Zi>)
set(PRIVATE_LINK_OPTIONS /DEBUG:FULL)
set(PUBLIC_COMPILE_FEATURES cxx_std_20)

set(UE4SS_COMPILE_DEFINITIONS
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
set(RC_DEPENDENCY_COMPILE_DEFINITIONS ${UE4SS_COMPILE_DEFINITIONS})

if (${UE4SS_CONSOLE_COLORS_ENABLED} EQUAL 1)
    add_compile_definitions(UE4SS_CONSOLE_COLORS_ENABLED)
endif()

add_subdirectory("Dependencies/File")
add_subdirectory("Dependencies/DynamicOutput")
add_subdirectory("Dependencies/Function")
add_subdirectory("Dependencies/Constructs")
add_subdirectory("Dependencies/Helpers")
add_subdirectory("Dependencies/SinglePassSigScanner")

# PolyHook
if (${MAKE_DEPENDENCIES_SHARED} EQUAL 1)
    set(MAKE_POLYHOOK_DEPENDENCIES_SHARED ON)
    set(MAKE_POLYHOOK_DEPENDENCIES_STATIC OFF)
else ()
    set(MAKE_POLYHOOK_DEPENDENCIES_SHARED OFF)
    set(MAKE_POLYHOOK_DEPENDENCIES_STATIC ON)
endif ()
set(POLYHOOK_BUILD_DLL ON CACHE BOOL "Build dll & lib instead of tests" FORCE)
set(POLYHOOK_BUILD_SHARED_LIB ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build polyhook as shared libary" FORCE)
set(POLYHOOK_BUILD_SHARED_ASMJIT ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build polyhook as shared libary" FORCE)
set(POLYHOOK_BUILD_SHARED_CAPSTONE ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build polyhook as shared libary" FORCE)
set(POLYHOOK_BUILD_SHARED_ZYDIS ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build polyhook as shared libary" FORCE)
set(ZYDIS_BUILD_SHARED_LIB ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build zydis as shared libary" FORCE)
set(ZYCORE_BUILD_SHARED_LIB ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build zycore as shared libary" FORCE)
set(ASMJIT_STATIC ${MAKE_POLYHOOK_DEPENDENCIES_STATIC} CACHE BOOL "Build asmjit as shared libary" FORCE)
set(POLYHOOK_BUILD_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)
set(POLYHOOK_DISASM_CAPSTONE OFF CACHE BOOL "Enable capstone support" FORCE)
set(POLYHOOK_DISASM_CAPSTONE_FULL OFF CACHE BOOL "Build all features of capstone." FORCE)
set(POLYHOOK_DISASM_ZYDIS ON CACHE BOOL "Enable Zydis support." FORCE)
add_subdirectory(Dependencies/PolyHook_2_0)
# For some reason, PolyHook isn't adding the capstone or zydis include directories
include_directories(Dependencies/PolyHook_2_0)
include_directories(Dependencies/PolyHook_2_0/capstone/include)
include_directories(Dependencies/PolyHook_2_0/zydis/include)
include_directories(Dependencies/PolyHook_2_0/zydis/dependencies/zycore/include)
include_directories(Dependencies/PolyHook_2_0/zydis/msvc)

add_subdirectory("Dependencies/Unreal")
add_subdirectory("Dependencies/MProgram")
add_subdirectory("Dependencies/Input")
add_subdirectory("Dependencies/LuaRaw")
add_subdirectory("Dependencies/LuaMadeSimple")
add_subdirectory("Dependencies/FunctionTimer")
add_subdirectory("Dependencies/ScopedTimer")

# fmtlib::fmt
# This is to be removed when std::format is fully available in the MSVC C++20 toolchain
# I'm using the header-only variant because I can't seem to figure out why it won't link
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(Dependencies/fmt)

add_subdirectory("Dependencies/ParserBase")
add_subdirectory("Dependencies/IniParser")
add_subdirectory("Dependencies/JSON")
add_subdirectory("Dependencies/ArgsParser")
add_subdirectory("Dependencies/LuaBindingsGenerator")

# UE4SS
# Settings override -> START
# Used for external manipulation until the entire cmake file has been redone
# An external tool may create these files before running cmake to alter build options
# The external tool must also remove these files after it's done running as to not interfere with the normal build process
if (${FORCE_WITH_CASE_PRESERVING_NAME} MATCHES 0)
    if (EXISTS "${UE4SS_GENERATED_SOURCE_DIR}/enable_with_case_preserving.build_option")
        message("WITH_CASE_PRESERVING_NAME overridden to 1 by '${UE4SS_GENERATED_SOURCE_DIR}/enable_with_case_preserving.build_option'")
        set(WITH_CASE_PRESERVING_NAME 1)
    elseif (EXISTS "${UE4SS_GENERATED_SOURCE_DIR}/disable_with_case_preserving.build_option")
        message("WITH_CASE_PRESERVING_NAME overridden to 0 by '${UE4SS_GENERATED_SOURCE_DIR}/disable_with_case_preserving.build_option'")
        set(WITH_CASE_PRESERVING_NAME 0)
    endif ()
else()
    set(WITH_CASE_PRESERVING_NAME 1)
endif ()
# Settings override -> END

# Version override -> START
file(READ ${UE4SS_GENERATED_SOURCE_DIR}/version.cache UE4SS_LIB_VERSION)
string(REGEX MATCHALL "([0-9]+).([0-9]+).([0-9]+).([0-9]+).([0-9]+)" TEST_OUTPUT_ABC "${UE4SS_LIB_VERSION}")
set(UE4SS_LIB_VERSION_MAJOR ${CMAKE_MATCH_1})
set(UE4SS_LIB_VERSION_MINOR ${CMAKE_MATCH_2})
set(UE4SS_LIB_VERSION_HOTFIX ${CMAKE_MATCH_3})
set(UE4SS_LIB_VERSION_PRERELEASE ${CMAKE_MATCH_4})  # Publicly available pre-release
set(UE4SS_LIB_VERSION_BETA ${CMAKE_MATCH_5})  # Internal version to keep track of pre-pre-releases
file(WRITE ${UE4SS_GENERATED_SOURCE_DIR}/version.cache "${UE4SS_LIB_VERSION}")

message("UE4SS")
message("Major: ${UE4SS_LIB_VERSION_MAJOR}, Minor: ${UE4SS_LIB_VERSION_MINOR}, Hotfix: ${UE4SS_LIB_VERSION_HOTFIX}, PreRelease: ${UE4SS_LIB_VERSION_PRERELEASE}, Beta: ${UE4SS_LIB_VERSION_BETA}")

# Make the hard-coded build numbers available to source files
add_compile_definitions(
        UE4SS_LIB_VERSION_MAJOR=${UE4SS_LIB_VERSION_MAJOR}
        UE4SS_LIB_VERSION_MINOR=${UE4SS_LIB_VERSION_MINOR}
        UE4SS_LIB_VERSION_HOTFIX=${UE4SS_LIB_VERSION_HOTFIX}
        UE4SS_LIB_VERSION_PRERELEASE=${UE4SS_LIB_VERSION_PRERELEASE}
        UE4SS_LIB_VERSION_BETA=${UE4SS_LIB_VERSION_BETA}
        UE4SS_LIB_BETA_STARTED=${UE4SS_LIB_BETA_STARTED}
        UE4SS_LIB_IS_BETA=${UE4SS_LIB_IS_BETA}
)
# Version override -> END

# Tell WinAPI macros to map to unicode functions instead of ansi
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main_ue4ss_rewritten.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UE4SSProgram.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsManager.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectDumper/ObjectToString.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Signatures.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UnrealCustom/CustomProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaLibrary.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaMod.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUObject.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUStruct.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUClass.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaAActor.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFName.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUScriptStruct.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaTArray.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUFunction.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaCustomProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFText.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFString.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXFieldClass.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXObjectProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXBoolProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXStructProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXEnumProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXArrayProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUEnum.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFOutputDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFWeakObjectPtr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFSoftObjectPath.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaTSoftClassPtr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUWorld.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/Common.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/Generator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/UEHeaderGenerator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/JSONDumper.cpp"
        #"${CMAKE_CURRENT_SOURCE_DIR}/src/LuaCustomMemberFunctions.cpp"
        #"${CMAKE_CURRENT_SOURCE_DIR}/src/LuaScriptMemoryAccess.cpp"
        #"${CMAKE_CURRENT_SOURCE_DIR}/src/LuaTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GUI.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/DX11.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Windows.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Console.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/ConsoleOutputDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/LiveView.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Dumpers.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/ImGuiUtility.cpp"

        # imgui
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/misc/cpp/imgui_stdlib.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_dx11.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/ImGuiColorTextEdit/TextEditor.cpp"
        #"${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_demo.cpp"
        )
add_library(${TARGET} SHARED ${${TARGET}_Sources})
target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_include> $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/LuaBindings/include>)
target_compile_options(${TARGET} PRIVATE ${PRIVATE_COMPILE_OPTIONS})
target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
target_compile_features(${TARGET} PUBLIC ${PUBLIC_COMPILE_FEATURES})
target_compile_definitions(${TARGET} PUBLIC $<${MAKE_DEPENDENCIES_SHARED}:LUA_BUILD_AS_DLL>)
set(UE4SS_LINK_WITH_LIBRARIES File DynamicOutput Unreal SinglePassSigScanner LuaMadeSimple FunctionTimer Function IniParser JSON Input PolyHook_2 d3d11)
set(UE4SS_LINK_WITH_INTERFACE_LIBRARIES fmt::fmt-header-only Function Constructs Helpers MProgram ScopedTimer)
target_link_libraries(${TARGET} PRIVATE ${UE4SS_LINK_WITH_LIBRARIES} ${UE4SS_LINK_WITH_INTERFACE_LIBRARIES})

set(XINPUT_TARGET xinput1_3)
enable_language(ASM_MASM)
file(GLOB XINPUT_SOURCES "xinput/*")
add_library(${XINPUT_TARGET} SHARED ${${TARGET}_Sources} ${XINPUT_SOURCES})
target_include_directories(${XINPUT_TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_include> $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/LuaBindings/include>)
target_compile_features(${XINPUT_TARGET} PUBLIC cxx_std_20)
target_compile_definitions(${XINPUT_TARGET} PUBLIC $<${MAKE_DEPENDENCIES_SHARED}:LUA_BUILD_AS_DLL> UE4SS_LIB_BUILD_AS_XINPUT=1)
target_compile_options(${XINPUT_TARGET} PRIVATE ${PRIVATE_MASM_COMPILE_OPTIONS})
target_link_options(${XINPUT_TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
target_link_libraries(${XINPUT_TARGET} PRIVATE ${UE4SS_LINK_WITH_LIBRARIES} ${UE4SS_LINK_WITH_INTERFACE_LIBRARIES})

if (${MAKE_DEPENDENCIES_STATIC} EQUAL 1)
    target_compile_definitions(${TARGET} PRIVATE ${UE4SS_COMPILE_DEFINITIONS})
    target_compile_definitions(${XINPUT_TARGET} PRIVATE ${UE4SS_COMPILE_DEFINITIONS})
endif()

set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
set_property(TARGET ${XINPUT_TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# imgui -> START
set(IMGUI_INCLUDES Dependencies/imgui Dependencies/ImGuiColorTextEdit)
target_include_directories(${TARGET} PUBLIC ${IMGUI_INCLUDES})
target_include_directories(${XINPUT_TARGET} PUBLIC ${IMGUI_INCLUDES})
# imgui -> END

# Auto incrementing build number -> START
# Fetch the auto incrementing build number
file(READ ${UE4SS_GENERATED_SOURCE_DIR}/build_number.cache UE4SS_LIB_BUILD_NUMBER)
message("Build #${UE4SS_LIB_BUILD_NUMBER} (Target: ${TARGET})")
add_custom_target(IncrementBuildNumber
        COMMAND ${CMAKE_COMMAND}
        -DRC_SOURCE_DIR:PATH="${PROJECT_SOURCE_DIR}"
        -DUE4SS_GENERATED_SOURCE_DIR:PATH="${UE4SS_GENERATED_SOURCE_DIR}"
        -DUE4SS_GENERATED_INCLUDE_DIR:PATH="${UE4SS_GENERATED_INCLUDE_DIR}"
        -P "${UE4SS_GENERATED_SOURCE_DIR}/build_number.cmake"
        )
add_dependencies(${TARGET} IncrementBuildNumber)
add_dependencies(${XINPUT_TARGET} IncrementBuildNumber)
# Auto incrementing build number -> END

# LuaBindingsGenerator -> START
# Create a list of all source files for this project so that LuaBindingsGenerator can know which files to parse.
get_directory_property(Unreal_Sources DIRECTORY Dependencies/Unreal DEFINITION Unreal_Sources)
set(UE4SS_LUA_BINDINGS_GENERATOR_FILES_TO_PARSE "${${TARGET}_Sources};${Unreal_Sources}")

foreach (FILE_TO_PARSE ${UE4SS_LUA_BINDINGS_GENERATOR_FILES_TO_PARSE})
    set(UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_SOURCES_ARGS "${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_SOURCES_ARGS} -sources=\"${FILE_TO_PARSE}\"")
endforeach ()

# https://discourse.cmake.org/t/cmake-list-of-all-project-targets/1077/17
function(_get_all_cmake_targets out_var current_dir)
    get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

    foreach (subdir ${subdirs})
        _get_all_cmake_targets(subdir_targets ${subdir})
        list(APPEND targets ${subdir_targets})
    endforeach ()

    set(${out_var} ${targets} PARENT_SCOPE)
endfunction()
_get_all_cmake_targets(ALL_TARGETS ${CMAKE_CURRENT_LIST_DIR})

# Create a list of all include directories so that LuaBindingsGenerator can parse #include statements properly.
set(UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS "")
foreach (THE_TARGET ${ALL_TARGETS})
    get_target_property(TARGET_INCLUDES ${THE_TARGET} INTERFACE_INCLUDE_DIRECTORIES)

    foreach (THE_INCLUDE ${TARGET_INCLUDES})
        if ((NOT "${THE_INCLUDE}" STREQUAL "TARGET_INCLUDES-NOTFOUND") AND (NOT ${THE_INCLUDE} MATCHES "<INSTALL_INTERFACE:"))
            if (${THE_INCLUDE} MATCHES <BUILD_INTERFACE:)
                string(REPLACE "$<BUILD_INTERFACE:" "" THE_INCLUDE ${THE_INCLUDE})
                string(REGEX REPLACE ">$" "" THE_INCLUDE ${THE_INCLUDE})
            endif ()
            set(THE_INCLUDE "-compiler_flags=\"-I${THE_INCLUDE}\"")
            #string(REPLACE "-I" "-compiler_flags=" THE_INCLUDE ${THE_INCLUDE})
            message("THE_INCLUDE: ${THE_INCLUDE}")
            list(APPEND UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS ${THE_INCLUDE})
        endif ()
    endforeach ()

    list(REMOVE_DUPLICATES UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS)
endforeach ()
string(REPLACE ";" " " UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS "${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS}")

get_target_property(ALL_COMPILE_DEFINITIONS ${XINPUT_TARGET} COMPILE_DEFINITIONS)
foreach (COMPILE_DEFINITION ${ALL_COMPILE_DEFINITIONS})
    if (NOT ${COMPILE_DEFINITION} MATCHES "(\\$).*")
        message("COMPILE_DEFINITION: ${COMPILE_DEFINITION}")
        set(UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS "${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS} -compiler_flags=\"-D${COMPILE_DEFINITION}\"")
    endif ()
endforeach()

# Create command line arguments for LuaBindingsGenerator.
set(UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_ARGUMENTS "-output=\"${CMAKE_SOURCE_DIR}/LuaBindings\" ${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_SOURCES_ARGS} -compiler_flags=\"-std=c++20\" ${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_COMPILER_FLAGS_ARGS}")

message("UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_ARGUMENTS: ${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_ARGUMENTS}")

if (UE4SS_GENERATE_LUA_BINDINGS EQUAL 1)
    # Create a custom target that executes before UE4SS compiles which will use LuaBindingGenerator to generate the Lua bindings.
    set(LuaBindingsGeneratorBin ${LIBRARY_OUTPUT_PATH})
    message("LuaBindingsGeneratorBin: ${LuaBindingsGeneratorBin}")
    add_custom_target(LuaBindingsGeneratorExec
            COMMAND ${LuaBindingsGeneratorBin}/LuaWrapperGenerator.exe "${UE4SS_LUA_BINDINGS_GENERATOR_COMMAND_LINE_ARGUMENTS}"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS LuaWrapperGenerator
            )
    add_dependencies(${TARGET} LuaBindingsGeneratorExec)
    add_dependencies(${XINPUT_TARGET} LuaBindingsGeneratorExec)
    # LuaBindingsGenerator -> END
endif ()
