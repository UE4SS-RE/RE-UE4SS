include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_MakeAvailable(corrosion)

# Import Rust crate
corrosion_import_crate(MANIFEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../first/patternsleuth_bind/Cargo.toml")

# Include the patternsleuth_bind CMakeLists.txt for folder organization
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../first/patternsleuth_bind" "${CMAKE_CURRENT_BINARY_DIR}/patternsleuth_bind")
