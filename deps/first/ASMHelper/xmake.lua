local projectName = "ASMHelper"

target(projectName)
    set_kind("shared")
    set_languages("cxx20")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    add_files("src/**.cpp")
    
    add_deps("File", "DynamicOutput", "Constructs")
    add_packages("zydis")

    on_load(function (target)
        import("target_helpers", { rootdir = os.projectdir() })
        
        print("Project: " .. projectName .. " (STATIC)")

        target:add("defines", target_helpers.project_name_to_exports_define(projectName))
        target:add("defines", target_helpers.project_name_to_build_static_define(projectName), { public = true })
    end)