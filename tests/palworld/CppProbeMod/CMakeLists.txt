cmake_minimum_required(VERSION 3.22)
project(UE4SSPalworldCppProbe LANGUAGES CXX)

set(UE4SS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH "Path to the RE-UE4SS source tree")
set(UE4SS_LIBRARY "" CACHE FILEPATH "Path to the native libUE4SS.so used by the server")

if(NOT EXISTS "${UE4SS_ROOT}/UE4SS/include/Mod/CppUserModBase.hpp")
    message(FATAL_ERROR "UE4SS_ROOT does not point to an RE-UE4SS source tree: ${UE4SS_ROOT}")
endif()
if(NOT EXISTS "${UE4SS_LIBRARY}")
    message(FATAL_ERROR "UE4SS_LIBRARY must point to an existing libUE4SS.so: ${UE4SS_LIBRARY}")
endif()

add_library(UE4SSRuntime SHARED IMPORTED GLOBAL)
set_target_properties(UE4SSRuntime PROPERTIES IMPORTED_LOCATION "${UE4SS_LIBRARY}")

add_library(CppProbeMod SHARED src/CppProbeMod.cpp)
target_compile_features(CppProbeMod PRIVATE cxx_std_23)
target_include_directories(CppProbeMod PRIVATE
    "${UE4SS_ROOT}/UE4SS/include"
    "${UE4SS_ROOT}/deps/first/File/include"
    "${UE4SS_ROOT}/deps/first/Input/include"
    "${UE4SS_ROOT}/deps/first/String/include"
)
target_compile_definitions(CppProbeMod PRIVATE RC_INPUT_BUILD_STATIC)
target_compile_options(CppProbeMod PRIVATE -fms-extensions)
target_link_libraries(CppProbeMod PRIVATE UE4SSRuntime)
target_link_options(CppProbeMod PRIVATE -Wl,-z,defs)
set_target_properties(CppProbeMod PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
