cmake_minimum_required(VERSION 3.22)

set(TARGET Concurrency)
project(${TARGET})
message("Project: ${TARGET} (HEADER-ONLY)")

add_library(${TARGET} INTERFACE)

# Enabling c++23 support
target_compile_features(${TARGET} INTERFACE cxx_std_23)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Make headers visible in the IDE
make_headers_visible(${TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Tests are commented out until GTest is available in the build environment
# if(BUILD_TESTING)
#     add_subdirectory(tests)
# endif()
