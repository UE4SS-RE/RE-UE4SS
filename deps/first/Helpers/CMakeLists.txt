cmake_minimum_required(VERSION 3.22)
set(TARGET Helpers)
project(${TARGET})
message("Project: ${TARGET} (HEADER-ONLY)")

add_library(${TARGET} INTERFACE)

# Enabling c++23 support
target_compile_features(${TARGET} PUBLIC cxx_std_23)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${TARGET} PUBLIC String)