# This file is used to organize patternsleuth_bind targets in the IDE
# The actual build is handled by Corrosion in deps/third/corrosion/CMakeLists.txt

# Use the generic helper function to organize patternsleuth_bind targets
# Uses organize_targets() from cmake/modules/IDEOrganization.cmake
organize_targets("^_?cargo-.*_patternsleuth_bind" "rust/patternsleuth_bind")

# Handle generic Cargo targets (clean, prebuild) that aren't specific to patternsleuth_bind
# These will go to deps/rust instead of deps/first/rust/patternsleuth_bind
organize_targets("^cargo-clean$|^cargo-prebuild$" "deps/rust")

# Create targets for IDE visibility of Rust source files (only if IDE visibility is enabled)
if(DEFINED ENABLE_IDE_SOURCE_VISIBILITY AND ENABLE_IDE_SOURCE_VISIBILITY)
    # Make patternsleuth_bind Rust files visible in IDE
    make_rust_visible(
        rust_ide_patternsleuth_bind 
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "deps/first/rust/patternsleuth_bind"
    )
    
    # Make patternsleuth Rust files visible in IDE
    make_rust_visible(
        rust_ide_patternsleuth
        "${CMAKE_CURRENT_SOURCE_DIR}/../patternsleuth/patternsleuth/src"
        "deps/first/rust/patternsleuth"
    )
endif()
