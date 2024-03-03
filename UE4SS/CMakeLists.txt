cmake_minimum_required(VERSION 3.18)

set(TARGET UE4SS)
project(${TARGET})
message("Project: UE4SS")

if (WIN32)
add_subdirectory("proxy_generator")
endif()

# Release settings -> START
option(UE4SS_LIB_BETA_IS_STARTED "Have beta releases started for the current major version" ON)
option(UE4SS_LIB_IS_BETA "Is this a beta release" ON)
# Release settings -> END

# Codegen settings -> START
set(UE4SS_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_include")
set(UE4SS_GENERATED_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_src")
# Codegen settings -> END

# Versioning -> START
file(READ ${UE4SS_GENERATED_SOURCE_DIR}/version.cache UE4SS_LIB_VERSION)
string(REGEX MATCHALL "([0-9]+).([0-9]+).([0-9]+).([0-9]+).([0-9]+)" TEST_OUTPUT_ABC "${UE4SS_LIB_VERSION}")
set(UE4SS_LIB_VERSION_MAJOR ${CMAKE_MATCH_1})
set(UE4SS_LIB_VERSION_MINOR ${CMAKE_MATCH_2})
set(UE4SS_LIB_VERSION_HOTFIX ${CMAKE_MATCH_3})
set(UE4SS_LIB_VERSION_PRERELEASE ${CMAKE_MATCH_4})  # Publicly available pre-release
set(UE4SS_LIB_VERSION_BETA ${CMAKE_MATCH_5})  # Internal version to keep track of pre-pre-releases

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE res
    OUTPUT_VARIABLE UE4SS_LIB_BUILD_GITSHA
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT res EQUAL 0)
    set(UE4SS_LIB_BUILD_GITSHA "0")
endif ()

# Make the hard-coded build numbers and current configuration available to source files
add_compile_definitions(
        UE4SS_LIB_VERSION_MAJOR=${UE4SS_LIB_VERSION_MAJOR}
        UE4SS_LIB_VERSION_MINOR=${UE4SS_LIB_VERSION_MINOR}
        UE4SS_LIB_VERSION_HOTFIX=${UE4SS_LIB_VERSION_HOTFIX}
        UE4SS_LIB_VERSION_PRERELEASE=${UE4SS_LIB_VERSION_PRERELEASE}
        UE4SS_LIB_VERSION_BETA=${UE4SS_LIB_VERSION_BETA}
        UE4SS_LIB_BETA_STARTED=$<BOOL:${UE4SS_LIB_BETA_IS_STARTED}>
        UE4SS_LIB_IS_BETA=$<BOOL:${UE4SS_LIB_IS_BETA}>
        UE4SS_LIB_BUILD_GITSHA="${UE4SS_LIB_BUILD_GITSHA}"
        UE4SS_CONFIGURATION="$<CONFIG>"
)
# Versioning -> END

message("UE4SS")
message("Major: ${UE4SS_LIB_VERSION_MAJOR}, Minor: ${UE4SS_LIB_VERSION_MINOR}, Hotfix: ${UE4SS_LIB_VERSION_HOTFIX}, PreRelease: ${UE4SS_LIB_VERSION_PRERELEASE}, Beta: ${UE4SS_LIB_VERSION_BETA}")

set(GUI_Sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GUI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GUITab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Console.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/ConsoleOutputDevice.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/LiveView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Dumpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/BPMods.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/ImGuiUtility.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/UFunctionCallerWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/SearcherWidget.cpp"
)

set(GUI_Sources_Win32
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/DX11.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GLFW3_OpenGL3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/Windows.cpp"
)

set(GUI_Sources_LinuxTUI
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/TUI.cpp"
)

set(GUI_Sources_LinuxGLFW3
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/GLFW3_OpenGL3.cpp"
)

set(${TARGET}_Sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UE4SSProgram.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectDumper/ObjectToString.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Signatures.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/Mod.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/LuaMod.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/CppUserModBase.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UnrealCustom/CustomProperty.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaLibrary.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaModRef.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUStruct.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaUClass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaAActor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFName.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaType/LuaFURL.cpp"
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

    "${CMAKE_CURRENT_SOURCE_DIR}/src/Mod/CppMod.cpp"
)

if (WIN32)
    # append GUI sources
    list(APPEND ${TARGET}_Sources ${GUI_Sources} ${GUI_Sources_Win32}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EntryWin32.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CrashDumper.cpp"
    )
    # set c definition HAS_GUI
    add_compile_definitions(HAS_UI)
    add_compile_definitions(HAS_GUI)
    add_compile_definitions(HAS_INPUT)
else()
    if (UE4SS_USE_TUI_ON_LINUX)
        list(APPEND ${TARGET}_Sources ${GUI_Sources} ${GUI_Sources_LinuxTUI})
        add_compile_definitions(HAS_UI)
        add_compile_definitions(HAS_TUI)
        add_compile_definitions(HAS_INPUT)
    endif()
    if (UE4SS_USE_GLFW_ON_LINUX)
        list(APPEND ${TARGET}_Sources ${GUI_Sources} ${GUI_Sources_LinuxGLFW3})
        add_compile_definitions(HAS_UI)
        add_compile_definitions(HAS_GUI)
        add_compile_definitions(HAS_INPUT)
    endif()
    list(APPEND ${TARGET}_Sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EntryLinux.cpp")
endif()

add_library(${TARGET} SHARED ${UE4SS_Sources})

if (NOT UE4SS_NO_CUSTOM_FLAGS)
    target_compile_options(${TARGET} PUBLIC "${PRIVATE_COMPILE_OPTIONS}")
    target_link_options(${TARGET} PUBLIC "${PRIVATE_LINK_OPTIONS}")
endif ()

# Enabling c++20 support
target_compile_features(${TARGET} PUBLIC cxx_std_23)

target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated_include>)

file(GLOB_RECURSE UE4SS_HEADERS include/**.hpp)
target_sources(${TARGET} PUBLIC
    FILE_SET ue4ss_headers TYPE HEADERS
    BASE_DIRS include
    FILES ${UE4SS_HEADERS})
get_property("TARGET_SOURCE_FILES" TARGET "${TARGET}" PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES} ${UE4SS_HEADERS})

# First party static build -> START
target_compile_definitions(${TARGET} PRIVATE
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
# First party static build -> END

target_link_libraries(${TARGET} PUBLIC
    File glaze::glaze DynamicOutput
    Unreal SinglePassSigScanner LuaMadeSimple
    Function IniParser JSON
    Constructs Helpers
    MProgram ScopedTimer Profiler
)
if (WIN32)
    target_link_libraries(${TARGET} PUBLIC
        Input ImGui PolyHook_2 d3d11 GLFW opengl32 dbghelp psapi 
        patternsleuth_bind)
    add_compile_definitions(HAS_D3D11)
    add_compile_definitions(HAS_GLFW)
else()
    target_link_libraries(${TARGET} PUBLIC
        patternsleuth_bind PolyHook_2 dl)
    if (UE4SS_USE_TUI_ON_LINUX)
        target_link_libraries(${TARGET} PUBLIC imtui imtui-ncurses ImGuiText Input)
    endif()
    if (UE4SS_USE_GLFW_ON_LINUX)
        target_link_libraries(${TARGET} PUBLIC ImGui Input GLFW)
        add_compile_definitions(HAS_GLFW)
    endif()
    # add pthread?
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${TARGET} PRIVATE Threads::Threads)
    set(THREADS_PREFER_PTHREAD_FLAG OFF)
endif()

if (WIN32)
    set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Making sure that the linked C runtime is a dll so that C++ mods will work correctly
if (WIN32)
set_property(TARGET ${TARGET} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "UE4SS" CXX_STANDARD 23)