cmake_minimum_required(VERSION 3.18)

set(TARGET UnrealVTableDumper)
project(${TARGET})
message("Project: UnrealVTableDumper")

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

if (NOT UE4SS_NO_CUSTOM_FLAGS)
        target_compile_options(${TARGET} PUBLIC "${PRIVATE_COMPILE_OPTIONS}")
        target_link_options(${TARGET} PUBLIC "${PRIVATE_LINK_OPTIONS}")
endif ()

# Enabling c++20 support
target_compile_features(${TARGET} PUBLIC cxx_std_20)

target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# First party static build -> START
target_compile_definitions(${TARGET} PRIVATE
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
# First party static build -> END

target_link_libraries(${TARGET} PRIVATE
    File Input DynamicOutput Helpers)

target_link_libraries(${TARGET} PRIVATE raw_pdb)
