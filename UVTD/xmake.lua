local projectName = "UVTD"

add_requires("raw_pdb", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })

target(projectName)
    set_kind("binary")
    set_languages("cxx20")
    set_exceptions("cxx")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    add_files("src/**.cpp")

    add_deps("File", "Input", "DynamicOutput", "Helpers")

    add_packages("raw_pdb")
