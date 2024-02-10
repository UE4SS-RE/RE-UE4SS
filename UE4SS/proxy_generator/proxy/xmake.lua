option("ue4ssProxyPath")
    set_default("C:\\Windows\\System32\\dwmapi.dll")
    set_description("Set path for a dll to generate the proxy for")

target("proxy_files")
    set_kind("phony")
    add_deps("proxy_generator")

    on_build(function (target)
        import("target_helpers", { rootdir = os.projectdir() })
        local proxy_path = get_config("ue4ssProxyPath")
        local proxy_name = target_helpers.get_path_filename(proxy_path)

        local proxy_generator = path.join(os.projectdir(), target:dep("proxy_generator"):targetfile())
        os.execv(proxy_generator, {proxy_path, target:autogendir()})
    end)

target("proxy")
    set_kind("shared")
    add_options("ue4ssProxyPath")
    set_languages("cxx20")
    add_rules("c++", "asm")

    add_deps("File", "proxy_files")

    add_files("proxy.rc")

    on_load(function (target)
        import("target_helpers", { rootdir = os.projectdir() })
        local filename = target_helpers.get_path_filename(get_config("ue4ssProxyPath"))
        target:set("basename", filename)
    end)

    on_config(function (target)
        import("target_helpers", { rootdir = os.projectdir() })
        import("build_configs.build_configs", { rootdir = os.projectdir() })

        build_configs:config(target)

        local output_path = target:dep("proxy_files"):autogendir()
        local proxy_name = target_helpers.get_path_filename(get_config("ue4ssProxyPath"))

        target:add("files", path.join(output_path, "dllmain.cpp"), { always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".asm"), { always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".def"), { always_added = true })
    end)
