local projectName = "ArgsParser"

target(projectName)
    set_kind("binary")
    set_languages("cxx20")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")
    
    add_files("src/**.cpp")
    

    add_deps("Helpers")