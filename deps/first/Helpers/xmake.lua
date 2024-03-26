local projectName = "Helpers"

target(projectName)
    set_kind("headeronly")
    set_languages("cxx20")
    set_exceptions("cxx")
    set_values("ue4ssDep", true)

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    on_load(function (target)
        print("Project: " .. projectName .. " (HEADER-ONLY)")
    end)