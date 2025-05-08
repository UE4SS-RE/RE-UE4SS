# First party dependencies -> START
add_subdirectory("ArgsParser")
add_subdirectory("ASMHelper")
add_subdirectory("Constructs")
add_subdirectory("String")
add_subdirectory("DynamicOutput")
add_subdirectory("File")
add_subdirectory("Function")
add_subdirectory("Helpers")
add_subdirectory("IniParser")
add_subdirectory("Input")
add_subdirectory("JSON")
add_subdirectory("LuaMadeSimple")
add_subdirectory("LuaRaw")
add_subdirectory("MProgram")
add_subdirectory("ParserBase")
add_subdirectory("ScopedTimer")
add_subdirectory("SinglePassSigScanner")
add_subdirectory("Unreal")
add_subdirectory("Profiler")
# We don't need to add patternsleuth_bind here as it's handled by Corrosion in the third party deps
# First party dependencies -> END

# Organize generic Rust targets into deps/rust
# Uses organize_targets() from cmake/modules/IDEOrganization.cmake
organize_targets("^cargo-clean$|^cargo-prebuild$" "deps/rust")
