local projectName = "Function"

target(projectName)
    set_kind("headeronly")
    set_languages("cxx20")
    set_exceptions("cxx")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    

    on_load(function (target)
        print("Project: " .. projectName .. " (HEADER-ONLY)")
    end)