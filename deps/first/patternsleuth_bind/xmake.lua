target("patternsleuth_bind")
    set_kind("static")
    add_files("src/lib.rs")
    add_deps("patternsleuth")
    set_values("rust.cratetype", "staticlib")

    add_links("ws2_32", "advapi32", "userenv", "ntdll", "oleaut32", "bcrypt", "ole32", { public = true })