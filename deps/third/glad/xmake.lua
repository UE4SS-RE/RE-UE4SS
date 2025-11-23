target("glad")
    set_kind("static")
    set_languages("cxx23")
    set_exceptions("cxx")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.h")

    add_files("src/glad.c")