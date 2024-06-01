include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_MakeAvailable(corrosion)

corrosion_import_crate(MANIFEST_PATH ../../first/patternsleuth_bind/Cargo.toml)
