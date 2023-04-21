cmake_minimum_required(VERSION 3.18)

set(TARGET StriveHitboxes-UE4SS)
project(${TARGET})

set(${TARGET}_Sources
        "${CMAKE_CURRENT_SOURCE_DIR}/src/arcsys.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/bbscript.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/dllmain.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/math_util.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/sigscan.cpp"
        )
        
add_subdirectory("minhook")

add_library(${TARGET} SHARED ${${TARGET}_Sources})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${TARGET} PUBLIC cppsdk_xinput minhook)
