cmake_minimum_required(VERSION 3.18)
message("Project: UnrealVTableDumper")

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

set(TARGET UnrealVTableDumper)
project(${TARGET})

set(PRIVATE_COMPILE_DEFINITIONS
        $<${MAKE_DEPENDENCIES_SHARED}:RC_EXPORT RC_FUNCTION_EXPORTS>
        $<${MAKE_DEPENDENCIES_STATIC}:RC_FUNCTION_BUILD_STATIC>)
set(PRIVATE_COMPILE_OPTIONS /MP $<$<CONFIG:RELEASE>:/Zi> /W3 /wd4005 /wd4251)
set(PRIVATE_LINK_OPTIONS /DEBUG:FULL)
set(PUBLIC_COMPILE_FEATURES cxx_std_20)

set(UVTD_COMPILE_DEFINITIONS
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
        )

add_subdirectory("Dependencies/File")
add_subdirectory("Dependencies/Input")
add_subdirectory("Dependencies/DynamicOutput")
add_subdirectory("Dependencies/Function")
add_subdirectory("Dependencies/Helpers")
add_subdirectory("Dependencies/raw_pdb")

# Tell WinAPI macros to map to unicode functions instead of ansi
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Helpers.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UVTD.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Symbols.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/TypeContainer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/VTableDumper.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UnrealVirtualGenerator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/TemplateClassParser.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/MemberVarsDumper.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/MemberVarsWrapperGenerator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SolBindingsGenerator.cpp"
        )

add_executable(${TARGET} ${${TARGET}_Sources})
target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_options(${TARGET} PRIVATE ${PRIVATE_COMPILE_OPTIONS})
target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
target_compile_features(${TARGET} PUBLIC ${PUBLIC_COMPILE_FEATURES})
set(UVTD_LINK_WITH_LIBRARIES File Input DynamicOutput)
set(UVTD_LINK_WITH_INTERFACE_LIBRARIES Function Helpers)
target_link_libraries(${TARGET} PRIVATE ${UVTD_LINK_WITH_LIBRARIES} ${UVTD_LINK_WITH_INTERFACE_LIBRARIES} raw_pdb)

if (${MAKE_DEPENDENCIES_STATIC} EQUAL 1)
    target_compile_definitions(${TARGET} PRIVATE ${UVTD_COMPILE_DEFINITIONS})
endif()
