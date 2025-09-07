cmake_minimum_required(VERSION 3.22)

set(TARGET Input)
project(${TARGET})

option(UE4SS_${TARGET}_BUILD_SHARED "Build as a shared lib" OFF)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Handler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/KeyDef.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformInit.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW3InputSource.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/QueueInputSource.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32AsyncInputSource.cpp"
        )

string(REGEX REPLACE "(.)([A-Z])" "\\1_\\2" MODULE_NAME ${TARGET})
string(TOUPPER ${MODULE_NAME} MODULE_NAME)

if (UE4SS_${TARGET}_BUILD_SHARED)
    message("Project: ${TARGET} (SHARED)")
    add_library(${TARGET} SHARED ${${TARGET}_Sources})
else ()
    message("Project: ${TARGET} (STATIC)")
    add_library(${TARGET} ${${TARGET}_Sources})
endif ()

# Enabling c++23 support
target_compile_features(${TARGET} PUBLIC cxx_std_23)

target_compile_definitions(${TARGET} PRIVATE
        RC_${MODULE_NAME}_EXPORTS
        $<$<NOT:$<BOOL:${UE4SS_${TARGET}_BUILD_SHARED}>>:
            RC_${MODULE_NAME}_BUILD_STATIC>
        $<$<BOOL:${UE4SS_INPUT_ENABLED}>:HAS_INPUT>)
        
if (NOT ${UE4SS_${TARGET}_BUILD_SHARED})
    target_compile_definitions(${TARGET} PRIVATE
            RC_DYNAMIC_OUTPUT_BUILD_STATIC
    )
endif ()

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})

target_link_libraries(${TARGET} PUBLIC DynamicOutput)

# Make headers visible in the IDE
# Uses make_headers_visible() from cmake/modules/IDEVisibility.cmake
make_headers_visible(${TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/include")