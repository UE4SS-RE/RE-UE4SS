includes("ArgsParser")
includes("ASMHelper")
includes("Constructs")
includes("DynamicOutput")
includes("File")
includes("Function")
includes("Helpers")
includes("IniParser")
includes("Input")
includes("JSON")
includes("LuaMadeSimple")
includes("LuaRaw")
includes("MProgram")
includes("ParserBase")
includes("patternsleuth_bind")
includes("Profiler")
includes("ScopedTimer")
includes("SinglePassSigScanner")
includes("Unreal")

if is_config("patternsleuth", "local") then 
    -- The patternsleuth target is managed by the cargo.build rule.
    target("patternsleuth")
        set_kind("static")
        add_rules("cargo.build", {project_name = "patternsleuth", is_debug = is_mode_debug(), features= { "process-internal" }})
        add_files("patternsleuth/Cargo.toml")
        -- Exposes the rust *.rs files to the Visual Studio project filters.
        add_extrafiles("patternsleuth/**.rs")
end

-- This option allows users to choose if patternsleuth should be installed as a package
-- or if patternsleuth should be built as a dependency by xmake. The `package` option
-- should be used if you don't intend on ever modifying the patternsleuth source.
-- The `local` option should be used if you want changes in the patternsleuth
-- submodule to be included as part of the UE4SS build. 
option("patternsleuth")
    set_default("package")
    set_showmenu(true)
    set_values("package", "local")
    set_description("Install patternsleuth as a package or build it as a dependency.", "package", "local")
