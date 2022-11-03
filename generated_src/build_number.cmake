cmake_minimum_required(VERSION 3.17)
set(CMAKE_VERBOSE_MAKEFILE on)

if (NOT RC_SOURCE_DIR)
    message("RC_SOURCE_DIR was undefined")
    return()
endif ()

# Config
set(BUILD_NUMBER_CACHE_FILE "${UE4SS_GENERATED_SOURCE_DIR}/build_number.cache")
set(RC_BUILD_NUMBER_HEADER_FILE "${UE4SS_GENERATED_INCLUDE_DIR}/build_number.hpp")

# Fetch & increment build number
if (EXISTS ${BUILD_NUMBER_CACHE_FILE})
    file(READ ${BUILD_NUMBER_CACHE_FILE} RC_BUILD_NUMBER)
    math(EXPR RC_BUILD_NUMBER "${RC_BUILD_NUMBER} + 1")
else ()
    set(RC_BUILD_NUMBER "0")
endif ()

file(WRITE ${BUILD_NUMBER_CACHE_FILE} "${RC_BUILD_NUMBER}")

# Make build number available by generating a header file
file(WRITE ${RC_BUILD_NUMBER_HEADER_FILE} "#define UE4SS_LIB_BUILD_NUMBER ${RC_BUILD_NUMBER}\n")

message("UE4SS_GENERATED_SOURCE_DIR: ${UE4SS_GENERATED_SOURCE_DIR}")
message("RC_SOURCE_DIR: ${RC_SOURCE_DIR}")
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("RC Build Number: ${RC_BUILD_NUMBER}")

