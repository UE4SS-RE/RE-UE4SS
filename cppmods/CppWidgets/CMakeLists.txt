cmake_minimum_required(VERSION 3.22)

set(TARGET CppWidgets)
project(${TARGET})

add_library(${TARGET} SHARED "main.cpp")
target_include_directories(${TARGET} PRIVATE .)

target_link_libraries(${TARGET} PRIVATE UE4SS)
