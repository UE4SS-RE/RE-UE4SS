cmake_minimum_required(VERSION 3.18)

set(TARGET File)
project(${TARGET})

option(UE4SS_${TARGET}_BUILD_SHARED "Build as a shared lib" OFF)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/File.cpp"
        )

if (WIN32)
    list(APPEND ${TARGET}_Sources
            "${CMAKE_CURRENT_SOURCE_DIR}/src/FileType/WinFile.cpp"
            )
elseif (LINUX)
    list(APPEND ${TARGET}_Sources
            "${CMAKE_CURRENT_SOURCE_DIR}/src/FileType/StdFile.cpp"
            )
    # add definitions for LINUX
    add_definitions(-DLINUX)
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

target_link_libraries(${TARGET} PUBLIC Helpers)

target_compile_definitions(${TARGET} PRIVATE
        RC_${MODULE_NAME}_EXPORTS
        $<$<NOT:$<BOOL:${UE4SS_${TARGET}_BUILD_SHARED}>>:
            RC_${MODULE_NAME}_BUILD_STATIC>)

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
