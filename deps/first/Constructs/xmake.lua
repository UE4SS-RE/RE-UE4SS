local projectName = "Constructs"

target(projectName)
    set_kind("headeronly")
    set_languages("cxx20")
    set_exceptions("cxx")
    set_values("ue4ssDep", true)

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    on_load(function ()
        print("Project: " .. projectName .. " (HEADER-ONLY)")
    end)