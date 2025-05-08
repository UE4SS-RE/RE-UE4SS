cmake_minimum_required(VERSION 3.18)

set(TARGET Helpers)
project(${TARGET})
message("Project: ${TARGET} (STATIC)")

add_library(${TARGET} STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/Casting.cpp")

# Enabling c++23 support
target_compile_features(${TARGET} PUBLIC cxx_std_20)

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${TARGET} PUBLIC String)