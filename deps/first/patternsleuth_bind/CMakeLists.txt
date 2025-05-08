# This file is used to organize patternsleuth_bind targets in the IDE
# The actual build is handled by Corrosion in deps/third/corrosion/CMakeLists.txt

# Use the generic helper function to organize patternsleuth_bind targets
# Uses organize_targets() from cmake/modules/IDEOrganization.cmake
organize_targets("^_?cargo-.*_patternsleuth_bind" "rust/patternsleuth_bind")

# Handle generic Cargo targets (clean, prebuild) that aren't specific to patternsleuth_bind
# These will go to deps/rust instead of deps/first/rust/patternsleuth_bind
organize_targets("^cargo-clean$|^cargo-prebuild$" "deps/rust")
