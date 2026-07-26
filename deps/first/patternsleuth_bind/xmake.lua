target("patternsleuth_bind")
    set_kind("static")
    add_rules("rust.cargo_deps")
    add_files("src/lib.rs")
    add_deps("patternsleuth")
    set_values("rust.cratetype", "staticlib")

    if is_plat("windows") then
        add_links("ws2_32", "advapi32", "userenv", "ntdll", "oleaut32", "bcrypt", "ole32", { public = true })
    end
