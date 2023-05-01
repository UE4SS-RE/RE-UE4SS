cmake_minimum_required(VERSION 3.18)

set(TARGET JSON)
project(${TARGET})

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

if ((NOT DEFINED BUILD_SHARED_LIBS) OR (NOT (${BUILD_SHARED_LIBS})))
    set(BUILD_STATIC 1)
    set(BUILD_SHARED 0)
    message("Project: ${TARGET} (STATIC)")
else ()
    set(BUILD_STATIC 0)
    set(BUILD_SHARED 1)
    message("Project: ${TARGET} (SHARED)")
endif ()

add_library(${TARGET} ${${TARGET}_Sources})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(${TARGET} PRIVATE ${PRIVATE_COMPILE_DEFINITIONS} $<${BUILD_SHARED}:RC_${MODULE_NAME}_EXPORTS> $<${BUILD_STATIC}:RC_${MODULE_NAME}_BUILD_STATIC RC_PARSER_BASE_BUILD_STATIC>)
target_compile_options(${TARGET} PRIVATE ${PRIVATE_COMPILE_OPTIONS})
target_compile_features(${TARGET} PUBLIC ${PUBLIC_COMPILE_FEATURES})
target_link_options(${TARGET} PRIVATE ${PRIVATE_LINK_OPTIONS})

target_link_libraries(${TARGET} PUBLIC File Constructs Helpers ParserBase)
