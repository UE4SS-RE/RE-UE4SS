package("polyhook_2")
    set_urls("https://github.com/stevemk14ebr/PolyHook_2_0.git")
    add_versions("v2.0.0", "298d56210b9d9e66cde8f96481d6053925c6ae15")
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
        if package:is_plat("linux") then
            local zydis = package:dep("zydis")
            local zycore = package:dep("zycore")
            table.insert(configs, "-DZYDIS_LIBRARY=" .. path.join(zydis:installdir("lib"), "libZydis.a"))
            table.insert(configs, "-DZYCORE_LIBRARY=" .. path.join(zycore:installdir("lib"), "libZycore.a"))
            table.insert(configs, "-DZYDIS_INCLUDE_DIR=" .. zydis:installdir("include"))
            table.insert(configs, "-DZYCORE_INCLUDE_DIR=" .. zycore:installdir("include"))
        end

        import("package.tools.cmake").install(package, configs, { packagedeps = { "zycore", "zydis" } })
        
    end)
package_end()
