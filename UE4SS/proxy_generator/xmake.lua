local projectName = "proxy_generator"

rule("dllexports")
    set_extensions(".exports")

target(projectName)
    set_kind("binary")
    set_languages("cxx23")
    set_exceptions("cxx")

    add_files("main.cpp")

    add_deps("File", "Constructs")
    add_links("imagehlp")

    add_files("exports/*.exports", { rule = "dllexports" })

    after_build(function(target)
        local proxy_exe =  target:targetfile()
        local output_path = target:autogendir()
        import("core.base.task")

        local input = get_config("ue4ssProxyPath")

        local dll_name = path.basename(input):lower()
        for k, sourcebatch in pairs(target:sourcebatches()) do
            if k == "dllexports" then
                for _, expfile in ipairs(sourcebatch.sourcefiles) do
                    if path.basename(expfile):lower() == dll_name then
                        input = path.absolute(expfile)
                        goto found
                    end
                end
            end
        end
        ::found::

        -- The second arg of task.run is used if we plan on calling `xmake proxy_files`.
        -- The proxy_files task is not exposed to the CLI, so we pass an empty second arg.
        task.run("proxy_files", {}, proxy_exe, input, output_path)
    end)

-- Define a task that executes the proxy_generator exe with the provided arguments
task("proxy_files")
    on_run(function (proxy_gen_exe, input, output_path)
        os.mkdir(output_path)
        os.execv(proxy_gen_exe, {input, output_path})
    end)

includes("proxy")
