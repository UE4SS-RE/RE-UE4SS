local projectName = "ArgsParser"

target(projectName)
    set_kind("binary")
    set_languages("cxx23")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")
    
    add_files("src/**.cpp")
    

    add_deps("Helpers")