cmake_minimum_required(VERSION 3.22)

set(TARGET UnrealVTableDumper)
project(${TARGET})
message("Project: UnrealVTableDumper")

# Use glob to find all source files
file(GLOB_RECURSE ${TARGET}_Sources "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")
file(GLOB_RECURSE ${TARGET}_Headers "${CMAKE_CURRENT_SOURCE_DIR}/include/**.hpp")

add_executable(${TARGET} ${${TARGET}_Sources} ${${TARGET}_Headers})

# Organize sources in IDE
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${${TARGET}_Sources} ${${TARGET}_Headers})

if (NOT UE4SS_NO_CUSTOM_FLAGS)
        target_compile_options(${TARGET} PUBLIC "${PRIVATE_COMPILE_OPTIONS}")
        target_link_options(${TARGET} PUBLIC "${PRIVATE_LINK_OPTIONS}")
endif ()

# Enabling c++23 support
target_compile_features(${TARGET} PUBLIC cxx_std_23)

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

target_link_libraries(${TARGET} PRIVATE glaze::glaze raw_pdb)

# Set IDE folder - make sure to use capital P for Programs
set_target_properties(${TARGET} PROPERTIES FOLDER "Programs")
