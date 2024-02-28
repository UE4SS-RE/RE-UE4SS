local projectName = "ASMHelper"

target(projectName)
    set_kind("static")
    set_languages("cxx20")
    set_exceptions("cxx")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    add_files("src/**.cpp")
    
    add_deps("File", "DynamicOutput", "Constructs")
    add_packages("zydis")

    on_load(function (target)
        import("target_helpers", { rootdir = get_config("ue4ssRoot") })
        
        print("Project: " .. projectName .. " (STATIC)")

        target:add("defines", "RC_ASM_HELPER_EXPORTS")
        target:add("defines", "RC_ASM_HELPER_BUILD_STATIC", { public = true })
    end)