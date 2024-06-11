local projectName = "File"

target(projectName)
    set_kind("static")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("ue4ss.dependency")

    add_includedirs("include", { public = true }) 
    add_headerfiles("include/**.hpp")

    add_files("src/File.cpp")

    if is_plat("windows") then
        add_files("src/FileType/WinFile.cpp")
    end
    add_deps("Helpers")
