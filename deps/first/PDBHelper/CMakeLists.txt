cmake_minimum_required(VERSION 3.22)

set(TARGET PDBHelper)
project(${TARGET})

option(UE4SS_${TARGET}_BUILD_SHARED "Build as a shared lib" OFF)

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PDBHelper.cpp"
        )

set(MODULE_NAME "PDB_HELPER")


if (UE4SS_ASM_HELPER_BUILD_SHARED)
    message("Project: ${TARGET} (SHARED)")
    add_library(${TARGET} SHARED ${${TARGET}_Sources})
else ()
    message("Project: ${TARGET} (STATIC)")
    add_library(${TARGET} ${${TARGET}_Sources})
endif ()

# Enabling c++23 support
target_compile_features(${TARGET} PUBLIC cxx_std_23)

target_compile_definitions(${TARGET} PRIVATE
        RC_${MODULE_NAME}_EXPORTS
        $<$<NOT:$<BOOL:${UE4SS_${TARGET}_BUILD_SHARED}>>:
            RC_${MODULE_NAME}_BUILD_STATIC>)

if (NOT ${UE4SS_${TARGET}_BUILD_SHARED})
    target_compile_definitions(${TARGET} PRIVATE
            RC_FILE_BUILD_STATIC
            RC_DYNAMIC_OUTPUT_BUILD_STATIC
            RC_SINGLE_PASS_SIG_SCANNER_BUILD_STATIC
            RC_HELPERS_BUILD_STATIC
    )
endif ()

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${TARGET} PUBLIC File DynamicOutput raw_pdb Helpers Constructs)

# Make headers visible in the IDE
# Uses make_headers_visible() from cmake/modules/IDEVisibility.cmake
make_headers_visible(${TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/include")
