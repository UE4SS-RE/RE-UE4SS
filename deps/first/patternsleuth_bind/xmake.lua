-- If patternsleuth is configured to install as a package.
add_requires("cargo::patternsleuth_bind", { optional = not is_config("patternsleuth", "package"), debug = is_mode_debug(), configs = { cargo_toml = path.join(os.scriptdir(), "Cargo.toml") } })

target("patternsleuth_bind")
    set_kind("static")
    set_values("rust.cratetype", "staticlib")
    add_files("src/lib.rs")
    -- Exposes the src/lib.rs files to the Visual Studio project filters.
    add_extrafiles("src/lib.rs")

    -- If patternsleuth is configured to install as a package.
    if is_config("patternsleuth", "package") then
        add_packages("cargo::patternsleuth_bind")
        add_links("ws2_32", "advapi32", "userenv", "ntdll", "oleaut32", "bcrypt", "ole32", { public = true })
    end

    -- If patternsleuth should be built as part of compilation.
    if is_config("patternsleuth", "local") then 
        add_deps("patternsleuth")
        add_rules("rust.link")
    end

    -- We need to clean up the non-selected package/local version of patternsleuth when we reconfigure.
    -- This just deletes the .lib and keeps cargo deps on disk for faster recompilation.
    on_config(function(target)
        os.tryrm(target:targetfile())
    end)