cmake_minimum_required(VERSION 3.18)

set(TARGET JSON)
project(${TARGET})

option(UE4SS_${TARGET}_BUILD_SHARED "Build as a shared lib" OFF)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Object.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Array.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/String.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Number.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Null.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Bool.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Parser/Parser.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Parser/TokenParser.cpp"
        )

# This regex doesn't work with a full caps name
# Since the module name needs to be full caps, and the name is full caps, just use the name
#string(REGEX REPLACE "(.)([A-Z])" "\\1_\\2" MODULE_NAME ${TARGET})
#string(TOUPPER ${MODULE_NAME} MODULE_NAME)
set(MODULE_NAME ${TARGET})

if (UE4SS_${TARGET}_BUILD_SHARED)
    message("Project: ${TARGET} (SHARED)")
    add_library(${TARGET} SHARED ${${TARGET}_Sources})
else ()
    message("Project: ${TARGET} (STATIC)")
    add_library(${TARGET} ${${TARGET}_Sources})
endif ()

# Enabling c++20 support
target_compile_features(${TARGET} PUBLIC cxx_std_20)

target_compile_definitions(${TARGET} PRIVATE
        RC_${MODULE_NAME}_EXPORTS
        $<$<NOT:$<BOOL:${UE4SS_${TARGET}_BUILD_SHARED}>>:
        RC_${MODULE_NAME}_BUILD_STATIC>)

if (NOT ${UE4SS_${TARGET}_BUILD_SHARED})
    target_compile_definitions(${TARGET} PRIVATE
            RC_PARSER_BASE_BUILD_STATIC
            RC_FILE_BUILD_STATIC
    )
endif ()

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${TARGET} PUBLIC File Constructs Helpers ParserBase)
