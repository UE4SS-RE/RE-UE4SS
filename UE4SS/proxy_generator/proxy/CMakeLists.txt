cmake_minimum_required(VERSION 3.18)

set(TARGET proxy)
project(${TARGET})
enable_language(ASM_MASM)

if(NOT DEFINED UE4SS_PROXY_PATH OR NOT UE4SS_PROXY_PATH)
    set(UE4SS_PROXY_PATH "C:\\Windows\\System32\\dwmapi.dll")
endif()

cmake_path(GET UE4SS_PROXY_PATH STEM PROXY_NAME)
add_library(${TARGET} SHARED
        "proxy.rc"
        "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def")

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${PROXY_NAME})
target_link_libraries(${TARGET} PRIVATE File)

add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def"
    COMMAND "$<TARGET_FILE:proxy_generator>" ${UE4SS_PROXY_PATH} ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
        proxy_generator
        ${UE4SS_PROXY_PATH}
)

add_custom_target(proxy_files DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def")
add_dependencies(${TARGET} proxy_files)