local projectName = "Helpers"

target(projectName)
    set_kind("headeronly")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")

    add_deps("String")
    
    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")