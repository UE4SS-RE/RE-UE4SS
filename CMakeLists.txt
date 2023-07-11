cmake_minimum_required(VERSION 3.18)
enable_language(CXX)
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
message("IPO - Supported: ${supported}; ${error}")
message("Project: UE4SS")

set(MAKE_DEPENDENCIES_SHARED 0)
set(STATIC_EXPORTS 1)

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
set(CMAKE_CXX_STANDARD 20)

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

# UE4SS
# Options -> START
option(UE4SS_WITH_CASE_PRESERVING_NAME "Use case-preserving FNames" OFF)

if (UE4SS_WITH_CASE_PRESERVING_NAME)
    add_definitions(-DWITH_CASE_PRESERVING_NAME)
endif()
# Options-> END

# Very temporary fix for ninja/clang thinking we should use MSVC simulation
unset(CMAKE_CXX_SIMULATE_ID)

set(PRIVATE_COMPILE_DEFINITIONS
        $<${MAKE_DEPENDENCIES_SHARED}:RC_EXPORT RC_FUNCTION_EXPORTS>
        $<${MAKE_DEPENDENCIES_STATIC}:RC_FUNCTION_BUILD_STATIC>)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,$<$<CONFIG:RELEASE>:/Zi>,/MP;$<$<CONFIG:RELEASE>:/Zi>;/W3;/wd4005;/wd4251;/wd4068>")
    set(PRIVATE_LINK_OPTIONS /DEBUG:FULL)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(PRIVATE_COMPILE_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-g;-gcodeview;-fcolor-diagnostics;-Wno-unknown-pragmas;-Wno-unused-parameter>")
    set(PRIVATE_LINK_OPTIONS "$<IF:$<COMPILE_LANGUAGE:ASM_MASM>,,-g>")
endif ()
set(PUBLIC_COMPILE_FEATURES cxx_std_20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(UE4SS_COMPILE_DEFINITIONS
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

# Zydis
if (${MAKE_DEPENDENCIES_SHARED} EQUAL 1)
    set(ZYDIS_BUILD_SHARED_LIB ON CACHE BOOL "Build zydis as shared libary" FORCE)
    set(ZYCORE_BUILD_SHARED_LIB ON CACHE BOOL "Build zycore as shared libary" FORCE)
else ()
    set(ZYDIS_BUILD_SHARED_LIB OFF CACHE BOOL "Build zydis as shared libary" FORCE)
    set(ZYCORE_BUILD_SHARED_LIB OFF CACHE BOOL "Build zycore as shared libary" FORCE)
endif ()
set(ZYDIS_BUILD_TOOLS OFF CACHE BOOL "")
set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory("Dependencies/zydis")
include_directories(Dependencies/zydis/include)
include_directories(Dependencies/zydis/dependencies/zycore/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/Dependencies/zydis)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/Dependencies/zydis/zycore)
set(CMAKE_PREFIX_PATH
    ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/zydis/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/zydis/dependencies/zycore/include
    )
# needed for PolyHook
set(ZYDIS_LIBRARY Zydis)
set(ZYCORE_LIBRARY Zycore)
set(ZYDIS_INCLUDE_DIR Dependencies/zydis/include)
set(ZYCORE_INCLUDE_DIR Dependencies/zydis/dependencies/zycore/include)

add_subdirectory(Dependencies/ASMHelper)

# PolyHook
if (${MAKE_DEPENDENCIES_SHARED} EQUAL 1)
    set(MAKE_POLYHOOK_DEPENDENCIES_SHARED ON)
    set(MAKE_POLYHOOK_DEPENDENCIES_STATIC OFF)
else ()
    set(MAKE_POLYHOOK_DEPENDENCIES_SHARED OFF)
    set(MAKE_POLYHOOK_DEPENDENCIES_STATIC ON)
endif ()
set(POLYHOOK_BUILD_DLL ON CACHE BOOL "Build dll & lib instead of tests" FORCE)
set(POLYHOOK_BUILD_SHARED_LIB ${MAKE_POLYHOOK_DEPENDENCIES_SHARED} CACHE BOOL "Build polyhook2 as shared libary" FORCE)
set(POLYHOOK_USE_EXTERNAL_ZYDIS ON CACHE BOOL "Use external zydis libary" FORCE)
set(ASMJIT_STATIC ${MAKE_POLYHOOK_DEPENDENCIES_STATIC} CACHE BOOL "Build 'asmjit' library as static" FORCE)
set(POLYHOOK_BUILD_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime" FORCE)

# add a conditional for disabling 'set_target_properties' on demand to prevent PolyHook from setting MSVC_RUNTIME_LIBRARY to "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
if(NOT DEFINED _DISABLE_SET_PROPERTIES)
  set(_DISABLE_SET_PROPERTIES TRUE)

  macro(set_target_properties)
    if (NOT _DISABLE_SET_PROPERTIES)
        _set_target_properties(${ARGV})
    endif()
  endmacro()
endif()

# add a conditional for disabling 'install' on demand
if(NOT DEFINED _DISABLE_INSTALL)
  set(_DISABLE_INSTALL TRUE)

  macro(install)
    if (NOT _DISABLE_INSTALL)
        _install(${ARGV})
    endif()
  endmacro()
endif()

add_subdirectory(Dependencies/PolyHook_2_0)

# re-enable 'set_target_properties' and 'install' since we're done adding PolyHook
set(_DISABLE_SET_PROPERTIES FALSE)
set(_DISABLE_INSTALL FALSE)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    target_compile_options(asmjit PRIVATE -Wno-everything -Wno-all -Wno-extra -Wno-pedantic -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-bitwise-instead-of-logical -Wno-sign-conversion -Wno-unused-command-line-argument -Wno-unused-but-set-variable)
    target_compile_options(PolyHook_2 PRIVATE -Wno-everything -Wno-all -Wno-extra -Wno-pedantic -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-value -Wno-missing-field-initializers -Wno-missing-braces -Wno-unused-variable -Wno-bitwise-instead-of-logical -Wno-reorder-ctor -Wno-unused-private-field)
endif ()

add_subdirectory("Dependencies/Unreal")
add_subdirectory("Dependencies/MProgram")
add_subdirectory("Dependencies/Input")
add_subdirectory("Dependencies/LuaRaw")
add_subdirectory("Dependencies/LuaMadeSimple")
add_subdirectory("Dependencies/FunctionTimer")
add_subdirectory("Dependencies/ScopedTimer")
add_subdirectory("Dependencies/ParserBase")
add_subdirectory("Dependencies/IniParser")
add_subdirectory("Dependencies/JSON")
add_subdirectory("Dependencies/ArgsParser")

# Version override -> START
file(READ ${UE4SS_GENERATED_SOURCE_DIR}/version.cache UE4SS_LIB_VERSION)
string(REGEX MATCHALL "([0-9]+).([0-9]+).([0-9]+).([0-9]+).([0-9]+)" TEST_OUTPUT_ABC "${UE4SS_LIB_VERSION}")
set(UE4SS_LIB_VERSION_MAJOR ${CMAKE_MATCH_1})
set(UE4SS_LIB_VERSION_MINOR ${CMAKE_MATCH_2})
set(UE4SS_LIB_VERSION_HOTFIX ${CMAKE_MATCH_3})
set(UE4SS_LIB_VERSION_PRERELEASE ${CMAKE_MATCH_4})  # Publicly available pre-release
set(UE4SS_LIB_VERSION_BETA ${CMAKE_MATCH_5})  # Internal version to keep track of pre-pre-releases

message("UE4SS")
message("Major: ${UE4SS_LIB_VERSION_MAJOR}, Minor: ${UE4SS_LIB_VERSION_MINOR}, Hotfix: ${UE4SS_LIB_VERSION_HOTFIX}, PreRelease: ${UE4SS_LIB_VERSION_PRERELEASE}, Beta: ${UE4SS_LIB_VERSION_BETA}")

find_package(Git REQUIRED)

# Build number is now git sha/hash
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE res
    OUTPUT_VARIABLE UE4SS_LIB_BUILD_GITSHA
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT res EQUAL 0)
    set(UE4SS_LIB_BUILD_GITSHA "0")
  endif()

set(RC_BUILD_NUMBER_HEADER_FILE "${UE4SS_GENERATED_INCLUDE_DIR}/build_number.hpp")
file(WRITE ${RC_BUILD_NUMBER_HEADER_FILE} "#define UE4SS_LIB_BUILD_GITSHA \"${UE4SS_LIB_BUILD_GITSHA}\"")

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
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/Mod.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/LuaMod.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/CppMod.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/CppUserModBase.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UnrealCustom/CustomProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaLibrary.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaModRef.cpp"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaXInterfaceProperty.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUEnum.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFOutputDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFWeakObjectPtr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFSoftObjectPath.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaTSoftClassPtr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUWorld.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUInterface.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/Common.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/Generator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/UEHeaderGenerator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/JSONDumper.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SDKGenerator/TMapOverrideGen.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/USMapGenerator/Generator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GUI.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GUITab.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/DX11.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GLFW3_OpenGL3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Windows.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Console.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/ConsoleOutputDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/LiveView.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Dumpers.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/BPMods.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/ImGuiUtility.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/UFunctionCallerWidget.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/SearcherWidget.cpp"

        # imgui
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/misc/cpp/imgui_stdlib.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_dx11.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_opengl3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/ImGuiColorTextEdit/TextEditor.cpp"
        #"${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui/imgui_demo.cpp"
        )
add_library(${TARGET} SHARED ${${TARGET}_Sources})
target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_include>)
target_compile_options(${TARGET} PRIVATE ${PRIVATE_COMPILE_OPTIONS})
target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
target_compile_features(${TARGET} PUBLIC ${PUBLIC_COMPILE_FEATURES})
target_compile_definitions(${TARGET} PUBLIC $<${MAKE_DEPENDENCIES_SHARED}:LUA_BUILD_AS_DLL>)
set(UE4SS_LINK_WITH_LIBRARIES File DynamicOutput Unreal SinglePassSigScanner LuaMadeSimple FunctionTimer Function IniParser JSON Input PolyHook_2 d3d11 GLFW opengl32)
set(UE4SS_LINK_WITH_INTERFACE_LIBRARIES Function Constructs Helpers MProgram ScopedTimer)
target_link_libraries(${TARGET} PUBLIC ${UE4SS_LINK_WITH_LIBRARIES} ${UE4SS_LINK_WITH_INTERFACE_LIBRARIES})

set(XINPUT_TARGET xinput1_3)
enable_language(ASM_MASM)
file(GLOB XINPUT_SOURCES "xinput/*")
add_library(${XINPUT_TARGET} SHARED ${${TARGET}_Sources} ${XINPUT_SOURCES})
target_include_directories(${XINPUT_TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_include>)
target_compile_features(${XINPUT_TARGET} PUBLIC cxx_std_20)
target_compile_definitions(${XINPUT_TARGET} PUBLIC $<${MAKE_DEPENDENCIES_SHARED}:LUA_BUILD_AS_DLL> UE4SS_LIB_BUILD_AS_XINPUT=1)
target_compile_options(${XINPUT_TARGET} PRIVATE ${PRIVATE_COMPILE_OPTIONS})
target_link_options(${XINPUT_TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
target_link_libraries(${XINPUT_TARGET} PUBLIC ${UE4SS_LINK_WITH_LIBRARIES} ${UE4SS_LINK_WITH_INTERFACE_LIBRARIES})

if (${MAKE_DEPENDENCIES_STATIC} EQUAL 1)
    target_compile_definitions(${TARGET} PRIVATE ${UE4SS_COMPILE_DEFINITIONS})
    target_compile_definitions(${XINPUT_TARGET} PRIVATE ${UE4SS_COMPILE_DEFINITIONS})
endif()

set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
set_property(TARGET ${XINPUT_TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# Making sure that the linked C runtime is a dll so that C++ mods will work correctly
set_property(TARGET ${TARGET} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set_property(TARGET ${XINPUT_TARGET} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# imgui -> START
set(IMGUI_INCLUDES Dependencies/imgui Dependencies/ImGuiColorTextEdit)
target_include_directories(${TARGET} PUBLIC ${IMGUI_INCLUDES})
target_include_directories(${XINPUT_TARGET} PUBLIC ${IMGUI_INCLUDES})
# imgui -> END

# GLFW -> START
add_library(GLFW STATIC IMPORTED)
set_property(TARGET GLFW PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GLFW/lib-vc2022/glfw3.lib")
target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GLFW/include")
target_include_directories(${XINPUT_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/GLFW/include")
# GLFW -> END

add_subdirectory("cppsdk")
