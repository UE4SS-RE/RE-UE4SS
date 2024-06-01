cmake_minimum_required(VERSION 3.18)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(TARGET Profiler)
project(${TARGET})
message("Project: ${TARGET} (HEADER-ONLY)")

set(ProfilerFlavors Tracy Superluminal)
set(RC_PROFILER_FLAVOR Tracy CACHE STRING "Profiler flavor")
set_property(CACHE RC_PROFILER_FLAVOR PROPERTY STRINGS ${ProfilerFlavors})

option(RC_PROFILER_DISABLE "Disable profiling" OFF)

add_library(${TARGET} INTERFACE)

# Enabling c++20 support
target_compile_features(${TARGET} INTERFACE cxx_std_20)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (${RC_PROFILER_DISABLE})
    target_compile_definitions(${TARGET} INTERFACE DISABLE_PROFILER)
endif ()

if (${RC_PROFILER_FLAVOR} STREQUAL Tracy)
    # Tracy start
    FetchContent_Declare(Tracy
        GIT_REPOSITORY git@github.com:wolfpld/tracy.git
        GIT_TAG v0.10
        GIT_PROGRESS ON)

    add_subdirectory("deps/Tracy")
    # Tracy end

    target_compile_definitions(${TARGET} INTERFACE IS_TRACY=1 IS_SUPERLUMINAL=0)
    target_link_libraries(${TARGET} INTERFACE TracyClient)
elseif (${RC_PROFILER_FLAVOR} STREQUAL Superluminal)
    find_package(SuperluminalAPI REQUIRED)

    target_compile_definitions(${TARGET} INTERFACE IS_TRACY=0 IS_SUPERLUMINAL=1)
    target_link_libraries(${TARGET} INTERFACE SuperluminalAPI)
endif ()