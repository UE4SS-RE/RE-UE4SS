include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
  corrosion
  GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
  GIT_TAG v0.4 # Using a specific tag/version
)

FetchContent_MakeAvailable(corrosion)

# Import Rust crate
corrosion_import_crate(MANIFEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../first/patternsleuth_bind/Cargo.toml")
