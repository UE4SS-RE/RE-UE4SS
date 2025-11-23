include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Set fmt options
set(FMT_HEADER_ONLY OFF CACHE BOOL "Build fmt as a compiled library, not header-only")

# Use the modern FetchContent approach
# The patching is now done via PATCH_COMMAND in the parent CMakeLists.txt
FetchContent_MakeAvailable(fmt)

# Uses suppress_third_party_warnings() from cmake/modules/ThirdPartyWarnings.cmake
suppress_third_party_warnings(fmt)