cmake_minimum_required(VERSION 3.18)

set(TARGET proxy_generator)
project(${TARGET})

add_executable(${TARGET} "main.cpp")
target_link_libraries(${TARGET} PRIVATE Constructs File ImageHlp)
target_compile_definitions(${TARGET} PRIVATE RC_FILE_BUILD_STATIC)