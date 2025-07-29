# Tracy profiler configuration

# Tracy configuration options
set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy profiler" FORCE)
set(TRACY_ON_DEMAND OFF CACHE BOOL "On-demand profiling")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library")

FetchContent_MakeAvailable(tracy)

# Uses suppress_third_party_warnings() from cmake/modules/ThirdPartyWarnings.cmake
suppress_third_party_warnings(TracyClient)