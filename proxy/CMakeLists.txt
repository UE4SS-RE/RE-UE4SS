cmake_minimum_required(VERSION 3.18)

set(TARGET proxy)
project(${TARGET})
enable_language(ASM_MASM)

set(UE4SS_PROXY_PATH "C:\\Windows\\System32\\xinput1_3.dll")

cmake_path(GET UE4SS_PROXY_PATH STEM PROXY_NAME)

add_library(${TARGET} SHARED "${LIBRARY_OUTPUT_PATH}/dllmain.cpp" "${LIBRARY_OUTPUT_PATH}/${PROXY_NAME}.asm" "${LIBRARY_OUTPUT_PATH}/${PROXY_NAME}.def")
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${PROXY_NAME})
target_link_libraries(${TARGET} PRIVATE File)

add_custom_command(
    OUTPUT
        "${LIBRARY_OUTPUT_PATH}/dllmain.cpp"
        "${LIBRARY_OUTPUT_PATH}/${PROXY_NAME}.asm"
        "${LIBRARY_OUTPUT_PATH}/${PROXY_NAME}.def"
    COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/proxy_generator.exe" ${UE4SS_PROXY_PATH} ${LIBRARY_OUTPUT_PATH}
    DEPENDS
        proxy_generator
        ${UE4SS_PROXY_PATH}
)

add_custom_target(proxy_files DEPENDS 
        "${LIBRARY_OUTPUT_PATH}/dllmain.cpp"
        "${LIBRARY_OUTPUT_PATH}/${PROXY_NAME}.asm"
        "${LIBRARY_OUTPUT_PATH}/${PROXY_NAME}.def")
add_dependencies(${TARGET} proxy_files)