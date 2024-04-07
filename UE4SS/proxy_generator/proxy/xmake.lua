option("ue4ssProxyPath")
    set_default("C:\\Windows\\System32\\dwmapi.dll")
    set_description("Set path for a dll to generate the proxy for")

target("proxy_files")
    set_kind("binary")
    add_deps("proxy_generator")

    set_policy("build.across_targets_in_parallel", false)

    on_build(function (target)
        local proxy_generator = path.join(os.projectdir(), target:dep("proxy_generator"):targetfile())
        local output_path = path.join(os.projectdir(), target:autogendir())
        local file = get_config("ue4ssProxyPath")

        os.mkdir(output_path)

        os.execv(proxy_generator, {file, output_path})
    end)

    on_link(function () end)

    after_clean(function (target)
        os.rm(target:autogendir())
    end)

target("proxy")
    set_kind("shared")
    add_options("ue4ssProxyPath")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("c++", "asm")

    set_policy("build.across_targets_in_parallel", false)

    add_deps("File", "proxy_files")

    add_files("proxy.rc")

    on_load(function (target)
        import("target_helpers", { rootdir = get_config("scriptsRoot") })
        local filename = target_helpers.get_path_filename(get_config("ue4ssProxyPath"))
        target:set("basename", filename)
    end)

    after_load(function (target)
        local projectRoot = get_config("ue4ssRoot")

        local scriptsRoot = get_config("scriptsRoot")
        import("target_helpers", { rootdir = scriptsRoot })
        import("build_configs", { rootdir = scriptsRoot })

        build_configs:config(target)
        build_configs:set_output_dir(target)

        local output_path = path.join(os.projectdir(), target:dep("proxy_files"):autogendir())
        local proxy_name = target_helpers.get_path_filename(get_config("ue4ssProxyPath"))

        target:add("files", path.join(output_path, "dllmain.cpp"), { always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".asm"), { rule = "asm", always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".def"), { always_added = true })
    end)
