local projectName = "ScopedTimer"

target(projectName)
    set_kind("headeronly")
    set_languages("cxx20")
    set_exceptions("cxx")
    set_values("ue4ssDep", true)

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    on_load(function (target)
        import("target_helpers", { rootdir = get_config("scriptsRoot") })
        
        print("Project: " .. projectName .. " (HEADER-ONLY)")

        target:add("defines", target_helpers.project_name_to_exports_define(projectName))
        target:add("defines", target_helpers.project_name_to_build_static_define(projectName))
    end)