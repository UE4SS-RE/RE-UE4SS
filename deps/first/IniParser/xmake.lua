local projectName = "IniParser"

target(projectName)
    set_kind("static")
    set_languages("cxx23")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    add_files(
        "src/Ini.cpp", "src/Value.cpp", "src/TokenParser.cpp"
    )
    
    add_deps("File", "Helpers", "ParserBase")