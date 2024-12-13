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
if get_config("ue4ssCross") ~= "msvc-wine" then
    includes("patternsleuth_bind")
end
includes("Profiler")
includes("ScopedTimer")
includes("SinglePassSigScanner")
includes("Unreal")
includes("String")

task("manuallyBuildLocalPatternsleuth")
    on_run(function()
        os.execv("cargo rustc --release --target x86_64-pc-windows-msvc --crate-type=staticlib", {}, {curdir = get_config("ue4ssRoot") .. "/deps/first/patternsleuth_bind"})
    end)

if get_config("ue4ssCross") ~= "msvc-wine" then
    if is_config("patternsleuth", "local") then
        -- The patternsleuth target is managed by the cargo.build rule.
        target("patternsleuth")
            set_kind("static")
            add_rules("cargo.build", {project_name = "patternsleuth", is_debug = is_mode_debug(), features = { "process-internal", "image-pe" }})
            add_files("patternsleuth/Cargo.toml")
            -- Exposes the rust *.rs files to the Visual Studio project filters.
            add_extrafiles("patternsleuth/**.rs")
    end
else
    target("patternsleuth_bind")
        set_kind("static")
        add_linkdirs(os.scriptdir() .. "/patternsleuth_bind/target/x86_64-pc-windows-msvc/release", {public = true})
        add_links("patternsleuth_bind", "ws2_32", "userenv", {public = true})
        add_links("ntdll", "Ole32", "OleAut32", {public = true})

        before_build(function()
            import("core.project.task")
            task.run("manuallyBuildLocalPatternsleuth")
        end)
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
