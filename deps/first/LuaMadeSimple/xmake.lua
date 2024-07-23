local projectName = "LuaMadeSimple"

target(projectName)
    set_kind("static")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")
    
    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    add_files("src/**.cpp")
    
    add_deps("LuaRaw", { public = true })
    add_deps("Helpers")
    add_packages("fmt")