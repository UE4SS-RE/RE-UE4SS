cmake_minimum_required(VERSION 3.22)

set(TARGET SDKRuntimeTestMod)
project(${TARGET})

add_subdirectory("${UE4SS_SDK_RUNTIME_TEST_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/generated-sdk")

add_library(${TARGET} SHARED src/dllmain.cpp)
target_link_libraries(${TARGET} PRIVATE UE4SS UE4SS_SDK)

# One translation unit pulls in every generated type, which passes the default section limit
# on larger games.
if(MSVC)
    target_compile_options(${TARGET} PRIVATE /bigobj)
endif()
