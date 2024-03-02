local projectName = "ArgsParser"

target(projectName)
    set_kind("binary")
    set_languages("cxx20")
    set_exceptions("cxx")
    set_values("ue4ssDep", true)

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")
    
    add_files("src/**.cpp")
    

    add_deps("Helpers")