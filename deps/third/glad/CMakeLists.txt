cmake_minimum_required(VERSION 3.18)
project(glad)

# Create glad library
add_library(glad STATIC
    src/glad.c
)

# Set include directories
target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link with OpenGL
if(WIN32)
    target_link_libraries(glad PUBLIC opengl32)
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(glad PUBLIC ${OPENGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(glad PUBLIC OpenGL::GL)
endif()

# Set properties
set_target_properties(glad PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)