local projectName = "proxy_generator"

target(projectName)
    set_kind("binary")
    set_languages("cxx20")

    add_files("main.cpp")

    add_deps("File", "Constructs")
    add_links("imagehlp")

includes("proxy")