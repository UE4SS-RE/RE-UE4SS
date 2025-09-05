cmake_minimum_required(VERSION 3.22)

set(TARGET proxy)
project(${TARGET})
enable_language(ASM_MASM)

# Determine proxy name and path with priority:
# 1. User-specified path (if provided)
# 2. Check exports directory (if no user path)
# 3. Default system DLL path (fallback)

if(UE4SS_PROXY_PATH)
    # User explicitly specified a path
    message(STATUS "Using user-specified proxy path: ${UE4SS_PROXY_PATH}")
    if(CMAKE_CROSSCOMPILING AND UE4SS_PROXY_PATH MATCHES "^[A-Za-z]:")
        # Windows absolute path on non-Windows host - extract just the filename
        get_filename_component(PROXY_NAME ${UE4SS_PROXY_PATH} NAME_WE)
    else()
        cmake_path(GET UE4SS_PROXY_PATH STEM PROXY_NAME)
    endif()
else()
    # No user path specified, check exports directory first
    set(DEFAULT_PROXY_NAME "dwmapi")
    set(EXPORT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../exports/${DEFAULT_PROXY_NAME}.exports")
    
    if(EXISTS ${EXPORT_FILE})
        # Found export file, use it
        message(STATUS "Found export file: ${EXPORT_FILE}")
        set(UE4SS_PROXY_PATH ${EXPORT_FILE})
        set(PROXY_NAME ${DEFAULT_PROXY_NAME})
        set(USE_EXPORT_FILE TRUE)
    else()
        # No export file, use platform default
        if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
            set(UE4SS_PROXY_PATH "C:\\Windows\\System32\\${DEFAULT_PROXY_NAME}.dll")
            message(STATUS "Using default Windows DLL path: ${UE4SS_PROXY_PATH}")
        else()
            # Cross-compiling without export file
            message(WARNING "No export file found for ${DEFAULT_PROXY_NAME}, build may fail")
            set(UE4SS_PROXY_PATH "${DEFAULT_PROXY_NAME}.dll")
        endif()
        set(PROXY_NAME ${DEFAULT_PROXY_NAME})
    endif()
endif()
add_library(${TARGET} SHARED
        "proxy.rc"
        "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def")

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${PROXY_NAME})
target_link_libraries(${TARGET} PRIVATE File)

# Determine if we're using an export file or a DLL
if(UE4SS_PROXY_PATH MATCHES "\\.exports$")
    set(USING_EXPORT_FILE TRUE)
elseif(USE_EXPORT_FILE)
    set(USING_EXPORT_FILE TRUE)
else()
    set(USING_EXPORT_FILE FALSE)
endif()

# Generate proxy files
if(USING_EXPORT_FILE)
    # Using pre-generated export file
    add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def"
        COMMAND "$<TARGET_FILE:proxy_generator>" ${UE4SS_PROXY_PATH} ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS
            proxy_generator
            ${UE4SS_PROXY_PATH}
        COMMENT "Generating proxy files from export file: ${UE4SS_PROXY_PATH}"
    )
elseif(CMAKE_CROSSCOMPILING)
    # Cross-compiling without export file - try to find one
    set(FALLBACK_EXPORT "${CMAKE_CURRENT_SOURCE_DIR}/../exports/${PROXY_NAME}.exports")
    if(EXISTS ${FALLBACK_EXPORT})
        add_custom_command(
            OUTPUT
                "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
                "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
                "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def"
            COMMAND "$<TARGET_FILE:proxy_generator>" ${FALLBACK_EXPORT} ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS
                proxy_generator
                ${FALLBACK_EXPORT}
            COMMENT "Generating proxy files from fallback export: ${FALLBACK_EXPORT}"
        )
    else()
        message(FATAL_ERROR "Cross-compilation requires export file for ${PROXY_NAME}, but none found")
    endif()
else()
    # Native Windows build - analyze DLL directly
    add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def"
        COMMAND "$<TARGET_FILE:proxy_generator>" ${UE4SS_PROXY_PATH} ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS
            proxy_generator
            ${UE4SS_PROXY_PATH}
        COMMENT "Generating proxy files from DLL: ${UE4SS_PROXY_PATH}"
    )
endif()

add_custom_target(proxy_files DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.asm"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROXY_NAME}.def")
add_dependencies(${TARGET} proxy_files)

# Directly set folders for proxy and proxy_files in Programs/proxy
set_target_properties(${TARGET} PROPERTIES FOLDER "Programs/proxy")
set_target_properties(proxy_files PROPERTIES FOLDER "Programs/proxy")