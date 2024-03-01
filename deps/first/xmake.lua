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
includes("Profiler")
includes("ScopedTimer")
includes("SinglePassSigScanner")
includes("Unreal")

-- Patternsleuth -> START

add_requires("cargo::patternsleuth_bind", { debug = is_mode_debug(), configs = { cargo_toml = path.join(os.scriptdir(), "patternsleuth_bind/Cargo.toml"), runtimes = get_mode_runtimes() } })

target("patternsleuth_bind")
    set_kind("static")
    set_values("rust.cratetype", "staticlib")
    add_files("patternsleuth_bind/src/lib.rs")
    add_packages("cargo::patternsleuth_bind")

    add_links("ws2_32", "advapi32", "userenv", "ntdll", "oleaut32", "bcrypt", "ole32", { public = true })

-- Patternsleuth -> END
