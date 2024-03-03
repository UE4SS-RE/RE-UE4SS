cmake_minimum_required(VERSION 3.18)

set(TARGET Input)
project(${TARGET})

option(UE4SS_${TARGET}_BUILD_SHARED "Build as a shared lib" OFF)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Handler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/KeyDef.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformInit.cpp"
        )

if (WIN32)
    list(APPEND ${TARGET}_Sources
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32AsyncInputSource.cpp"
            )
    add_compile_definitions(HAS_GLFW)
else()
    if (UE4SS_USE_TUI_ON_LINUX OR UE4SS_USE_GLFW_ON_LINUX)
        list(APPEND ${TARGET}_Sources
           "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/QueneInputSource.cpp"
           "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/NcursesInputSource.cpp"
           "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW3InputSource.cpp"
        )
        if (UE4SS_USE_TUI_ON_LINUX)
            add_compile_definitions(HAS_TUI)
        endif()
        if (UE4SS_USE_GLFW_ON_LINUX)
            add_compile_definitions(HAS_GLFW)
        endif()
    else()
        # error message 
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()
string(REGEX REPLACE "(.)([A-Z])" "\\1_\\2" MODULE_NAME ${TARGET})
string(TOUPPER ${MODULE_NAME} MODULE_NAME)

if (UE4SS_${TARGET}_BUILD_SHARED)
    message("Project: ${TARGET} (SHARED)")
    add_library(${TARGET} SHARED ${${TARGET}_Sources})
else ()
    message("Project: ${TARGET} (STATIC)")
    add_library(${TARGET} ${${TARGET}_Sources})
endif ()

# Enabling c++20 support
target_compile_features(${TARGET} PUBLIC cxx_std_20)

# DynamicOutput
target_link_libraries(${TARGET} PRIVATE DynamicOutput)

target_compile_definitions(${TARGET} PRIVATE
        RC_${MODULE_NAME}_EXPORTS
        $<$<NOT:$<BOOL:${UE4SS_${TARGET}_BUILD_SHARED}>>:
            RC_${MODULE_NAME}_BUILD_STATIC>)

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})
