local projectName = "File"

target(projectName)
    set_kind("static")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")

    add_includedirs("include", { public = true }) 
    add_headerfiles("include/**.hpp")

    add_files("src/File.cpp")

    if is_plat("windows") then
        add_files("src/FileType/WinFile.cpp")
    elseif is_plat("linux") then
        add_files("src/FileType/StdFile.cpp")
    end
    
    add_deps("Helpers")

    on_load(function (target)
        import("target_helpers", { rootdir = get_config("scriptsRoot") })
        
        print("Project: " .. projectName .. " (STATIC)")

        target:add("defines", target_helpers.project_name_to_exports_define(projectName))
        target:add("defines", target_helpers.project_name_to_build_static_define(projectName))
    end)
