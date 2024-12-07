package("polyhook_2")
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