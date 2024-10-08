package("safetyhook")
    add_urls("git@github.com:cursey/safetyhook.git")
    add_urls("https://github.com/cursey/safetyhook.git")

    add_versions("v0.4.1", "629558c64009a7291ba6ed5cfb49187086a27a47")

    add_deps("cmake", "zydis")
    
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        import("package.tools.cmake").install(package, configs, { packagedeps = {"zydis", "zycore"} })

        os.cp("include/*.hpp", package:installdir("include"))
        os.cp("include/safetyhook/*.hpp", package:installdir("include/safetyhook"))
    end)
package_end()