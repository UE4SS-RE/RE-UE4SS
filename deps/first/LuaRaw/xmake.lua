option("lua_compile_as_cpp")
    set_default(false)
    set_showmenu(true)
    set_description("Compile Lua source files as C++ instead of C")
    set_category("Lua Options")

local projectName = "LuaRaw"

target(projectName)
    set_kind("static")
    set_languages("cxx23")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")
    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")
    add_options("lua_compile_as_cpp")

    local lua_sources = {
        "src/lapi.c", "src/lauxlib.c", "src/lbaselib.c", "src/lcode.c",
        "src/lcorolib.c", "src/lctype.c", "src/ldblib.c", "src/ldebug.c",
        "src/ldo.c", "src/ldump.c", "src/lfunc.c", "src/lgc.c",
        "src/linit.c", "src/liolib.c", "src/llex.c", "src/lmathlib.c",
        "src/lmem.c", "src/loadlib.c", "src/lobject.c", "src/lopcodes.c",
        "src/loslib.c", "src/lparser.c", "src/lstate.c", "src/lstring.c",
        "src/lstrlib.c", "src/ltable.c", "src/ltablib.c", "src/ltm.c",
        "src/luauser.c", "src/lundump.c", "src/lutf8lib.c", "src/lvm.c",
        "src/lzio.c"
    }

    local compile_as_cpp = get_config("lua_compile_as_cpp")
    if compile_as_cpp == nil then compile_as_cpp = false end
    
    if compile_as_cpp then
        add_defines("LUA_COMPILED_AS_CPP", { public = true })
        -- Force compilation as C++
        add_files(lua_sources, {sourcekind = "cxx"})
    else
        -- Compile as C
        add_files(lua_sources)
    end
    

