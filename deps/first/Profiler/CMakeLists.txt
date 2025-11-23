cmake_minimum_required(VERSION 3.18)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(TARGET Profiler)
project(${TARGET})
message("Project: ${TARGET} (HEADER-ONLY)")

set(ProfilerFlavors Tracy Superluminal None)
# Default to None - users can opt-in to Tracy or Superluminal if needed
# This is also set in ProjectConfig.cmake - whichever is evaluated last takes precedence
set(RC_PROFILER_FLAVOR "None" CACHE STRING "Profiler flavor (Tracy, Superluminal, or None)")
set_property(CACHE RC_PROFILER_FLAVOR PROPERTY STRINGS ${ProfilerFlavors})

add_library(${TARGET} INTERFACE)

# Enabling c++23 support
target_compile_features(${TARGET} INTERFACE cxx_std_23)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Make headers visible in the IDE
# Uses make_headers_visible() from cmake/modules/IDEVisibility.cmake
make_headers_visible(${TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (${RC_PROFILER_FLAVOR} STREQUAL None)
    message(STATUS "Profiler: Disabled (set RC_PROFILER_FLAVOR=Tracy to enable)")
    target_compile_definitions(${TARGET} INTERFACE DISABLE_PROFILER IS_TRACY=0 IS_SUPERLUMINAL=0)
elseif (${RC_PROFILER_FLAVOR} STREQUAL Tracy)
    message(STATUS "Profiler: Tracy (fetching from GitHub)")
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
    message(STATUS "Profiler: Superluminal")
    find_package(SuperluminalAPI REQUIRED)

    target_compile_definitions(${TARGET} INTERFACE IS_TRACY=0 IS_SUPERLUMINAL=1)
    target_link_libraries(${TARGET} INTERFACE SuperluminalAPI)
endif ()