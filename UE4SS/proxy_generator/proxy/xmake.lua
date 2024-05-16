option("ue4ssProxyPath")
    set_default("C:\\Windows\\System32\\dwmapi.dll")
    set_description("Set path for a dll to generate the proxy for")

target("proxy")
    set_kind("shared")
    add_options("ue4ssProxyPath")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("c++", "asm")

    set_policy("build.across_targets_in_parallel", false)

    add_deps("File", "proxy_generator")

    add_files("proxy.rc")

    -- Set the name of the target file to be the base file name of the proxy dll ("dwmapi").
    set_basename(path.basename(get_config("ue4ssProxyPath")))

    after_load(function (target)
        local output_path = path.join(target:dep("proxy_generator"):autogendir())
        local proxy_name = path.basename(target:targetfile())
        target:add("files", path.join(output_path, "dllmain.cpp"), { always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".asm"), { rule = "asm", always_added = true })
        target:add("files", path.join(output_path, proxy_name .. ".def"), { always_added = true })
    end)

    on_install(function(target)
        os.mkdir(target:installdir())
        os.cp(target:targetfile(), target:installdir())
    end)
