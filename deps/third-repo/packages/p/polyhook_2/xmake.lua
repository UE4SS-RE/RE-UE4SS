package("polyhook_2")
    set_urls("https://github.com/stevemk14ebr/PolyHook_2_0.git")
    add_versions("v2.0.0", "fd2a88f09c8ae89440858fc52573656141013c7f")
    set_sourcedir(os.scriptdir())
    
    add_deps("cmake", "zydis", "zycore")

    on_install(function (package)
        local configs = {}

        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        if get_config("ue4ssCross") == "msvc-wine" then
            table.insert(configs, "-Due4ssCross=msvc-wine")
        else
            table.insert(configs, "-Due4ssCross=None")
        end

        import("package.tools.cmake").install(package, configs, { packagedeps = { "zycore", "zydis" } })
        
    end)
package_end()