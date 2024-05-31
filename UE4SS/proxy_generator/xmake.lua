local projectName = "proxy_generator"

target(projectName)
    set_kind("binary")
    set_languages("cxx20")
    set_exceptions("cxx")

    add_files("main.cpp")

    add_deps("File", "Constructs")
    add_links("imagehlp")

    after_build(function(target)
        local proxy_exe =  target:targetfile()
        local output_path = target:autogendir()
        import("core.base.task")
        -- The second arg of task.run is used if we plan on calling `xmake proxy_files`.
        -- The proxy_files task is not exposed to the CLI, so we pass an empty second arg.
        task.run("proxy_files", {}, proxy_exe, output_path)
    end)

-- Define a task that executes the proxy_generator exe with the provided arguments
task("proxy_files")
    on_run(function (proxy_gen_exe, output_path)
        os.mkdir(output_path)
        os.execv(proxy_gen_exe, {get_config("ue4ssProxyPath"), output_path})
    end)
includes("proxy")