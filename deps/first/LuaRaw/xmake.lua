local projectName = "LuaRaw"

target(projectName)
    set_kind("static")
    set_languages("cxx20")
    set_exceptions("cxx")
    set_values("ue4ssDep", true)

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    add_files(
        "src/lapi.c", "src/lauxlib.c", "src/lbaselib.c", "src/lcode.c",
        "src/lcorolib.c", "src/lctype.c", "src/ldblib.c", "src/ldebug.c",
        "src/ldo.c", "src/ldump.c", "src/lfunc.c", "src/lgc.c",
        "src/linit.c", "src/liolib.c", "src/llex.c", "src/lmathlib.c",
        "src/lmem.c", "src/loadlib.c", "src/lobject.c", "src/lopcodes.c",
        "src/loslib.c", "src/lparser.c", "src/lstate.c", "src/lstring.c",
        "src/lstrlib.c", "src/ltable.c", "src/ltablib.c", "src/ltm.c",
        "src/luauser.c", "src/lundump.c", "src/lutf8lib.c", "src/lvm.c",
        "src/lzio.c"
    )

    

    on_load(function (target)
        import("target_helpers", { rootdir = get_config("scriptsRoot") })
        
        print("Project: " .. projectName .. " (STATIC)")

        target:add("defines", target_helpers.project_name_to_exports_define(projectName))
        target:add("defines", target_helpers.project_name_to_build_static_define(projectName))
    end)