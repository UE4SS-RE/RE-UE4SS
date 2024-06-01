cmake_minimum_required(VERSION 3.18)

set(TARGET ArgsParser)
project(${TARGET})

set(${TARGET}_Sources
        #"${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET}/main.cpp"
        )

string(REGEX REPLACE "(.)([A-Z])" "\\1_\\2" MODULE_NAME ${TARGET})
string(TOUPPER ${MODULE_NAME} MODULE_NAME)

add_library(${TARGET} INTERFACE)
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enabling c++20 support
target_compile_features(${TARGET} INTERFACE cxx_std_20)

# First-party dependencies
target_link_libraries(${TARGET} INTERFACE Helpers)

add_executable(${TARGET}_DEV_TEST "${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET}/main.cpp")
target_link_libraries(${TARGET}_DEV_TEST ArgsParser)
