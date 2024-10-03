add_requires("luajit2", { debug = is_mode_debug(), configs = { gc64 = true, static = true , runtimes = get_mode_runtimes()} } )

target("LuaRaw")
    set_kind("static")
    add_packages("luajit2", { public = true })
    add_includedirs("src", { public = true })

    add_headerfiles("src/compat.h", "src/lua.hpp")
    add_files("src/compat.c")
