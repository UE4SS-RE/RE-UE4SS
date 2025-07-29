cmake_minimum_required(VERSION 3.22)
set(TARGET String)
project(${TARGET})

add_library(${TARGET} INTERFACE)

# Enabling c++23 support
target_compile_features(${TARGET} INTERFACE cxx_std_23)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

string(REGEX REPLACE "(.)([A-Z])" "\\1_\\2" MODULE_NAME ${TARGET})
string(TOUPPER ${MODULE_NAME} MODULE_NAME)
target_compile_definitions(${TARGET} INTERFACE 
    RC_${MODULE_NAME}_BUILD_STATIC)
    
# Make headers visible in the IDE
# Uses make_headers_visible() from cmake/modules/IDEVisibility.cmake
make_headers_visible(${TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/include")

message("Project: ${TARGET} (HEADER-ONLY)")
