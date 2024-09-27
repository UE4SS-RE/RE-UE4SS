package("luajit2")
    set_homepage("http://luajit.org")
    set_description("A Just-In-Time Compiler (JIT) for the Lua programming language. OpenResty's Branch of LuaJIT 2.")
    set_license("MIT")

    set_urls("https://github.com/openresty/luajit2.git")

    add_versions("v2.1-20240815", "33d6b04681d2f079a6d013988a426a841c52e29e")

    add_configs("nojit", { description = "Disable JIT.", default = false, type = "boolean"})
    add_configs("fpu",   { description = "Enable FPU.", default = true, type = "boolean"})
    add_configs("gc64",  { description = "Enable GC64.", default = false, type = "boolean"})

    add_includedirs("include/luajit", {public = true})
    if not is_plat("windows") then
        add_syslinks("dl")
    end

    if on_check then
        on_check(function (package)
            if package:is_arch("arm.*") then
                raise("package(luajit/arm64) unsupported arch")
            end
        end)
    end

    on_install("windows", "linux", "macosx", "bsd", "android", "iphoneos", function (package)
        local configs = {}
        configs.fpu     = package:config("fpu")
        configs.nojit   = package:config("nojit")
        configs.gc64    = package:config("gc64")
        if package:config("shared") then
            configs.kind = "shared"
        elseif package:config("static") then
            configs.kind = "static"
        elseif package:config("binary") then
            configs.kind = "binary"
        end
        os.cp(path.join(package:scriptdir(), "port", "xmake.lua"), "xmake.lua")
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:has_cfuncs("lua_pcall", {includes = "luajit.h"}))
    end)
