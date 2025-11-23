local projectName = "File"

target(projectName)
    set_kind("static")
    set_languages("cxx23")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")
    
    add_deps("String", "Helpers")

    add_includedirs("include", { public = true }) 
    add_headerfiles("include/**.hpp")

    add_files("src/**.cpp")
    add_packages("fmt", { public = true })