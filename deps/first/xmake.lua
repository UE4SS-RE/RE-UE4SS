-- includes("ArgsParser")
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

add_requires("cargo::patternsleuth_bind", { debug = is_mode_debug(), configs = { cargo_toml = path.join(os.scriptdir(), "patternsleuth_bind/Cargo.toml"), runtimes = get_mode_runtimes() } })

target("patternsleuth_bind")
    set_kind("static")
    set_values("rust.cratetype", "staticlib")
    set_values("rust.edition", "2021")
    add_files("patternsleuth_bind/src/lib.rs")
    if is_plat("linux") and is_host("windows") then
        add_rcflags("--target=x86_64-unknown-linux-gnu", {force = true})
    end
    add_packages("cargo::patternsleuth_bind")
    if is_plat("windows") then
        add_links("ws2_32", "advapi32", "userenv", "ntdll", "oleaut32", "bcrypt", "ole32", { public = true })
    end
-- Patternsleuth -> END
