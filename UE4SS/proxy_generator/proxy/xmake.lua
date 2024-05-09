option("ue4ssProxyPath")
    set_default("C:\\Windows\\System32\\dwmapi.dll")
    set_description("Set path for a dll to generate the proxy for")

target("proxy_files")
    set_kind("binary")
    add_deps("proxy_generator")

    set_policy("build.across_targets_in_parallel", false)

    -- Provide our own build logic by overriding xmake on_build()
    on_build(function (target)
        local proxy_generator = path.join(target:dep("proxy_generator"):targetfile())
        local output_path = path.join(get_config("ue4ssRoot"), target:autogendir())
        os.mkdir(output_path)
        os.execv(proxy_generator, {get_config("ue4ssProxyPath"), output_path})
    end)

    -- We override on_link to stop xmake from attempting to link this target.
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

    -- Set the name of the target file to be the base file name of the proxy dll ("dwmapi").
    set_basename(path.basename(get_config("ue4ssProxyPath")))

    after_load(function (target)
        local output_path = path.join(get_config("ue4ssRoot"), target:dep("proxy_files"):autogendir())
        local proxy_name = path.basename(target:targetfile())

        target:add("files", path.join(output_path, "dllmain.cpp"), { always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".asm"), { rule = "asm", always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".def"), { always_added = true })
    end)
