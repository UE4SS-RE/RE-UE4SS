cmake_minimum_required(VERSION 3.18)

set(TARGET Helpers)
project(${TARGET})
message("Project: ${TARGET} (HEADER-ONLY)")

add_library(${TARGET} INTERFACE)

# Enabling c++20 support
target_compile_features(${TARGET} INTERFACE cxx_std_20)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${TARGET} INTERFACE File Function safetyhook)
