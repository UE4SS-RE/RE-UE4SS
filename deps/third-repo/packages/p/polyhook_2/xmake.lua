package("polyhook_2")
    set_sourcedir(os.scriptdir())
    
    add_deps("cmake", "zydis", "zycore")

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        if is_plat("linux") then
            table.insert(configs, "-DZYDIS_INCLUDE_DIR=" .. package:dep("zydis"):installdir("include"))
            table.insert(configs, "-DZYCORE_INCLUDE_DIR=" .. package:dep("zycore"):installdir("include"))
            table.insert(configs, "-DZYCORE_LIBRARY=" .. package:dep("zycore"):installdir("libzycore.a"))
            table.insert(configs, "-DZYDIS_LIBRARY=" .. package:dep("zydis"):installdir("libzydis.a"))
        end
        import("package.tools.cmake").install(package, configs, { packagedeps = { "zycore", "zydis" } })
        print(package:get("links"))
        package:add("links", "PolyHook_2")
        package:add("links", "asmtk")
        package:add("links", "asmjit")
        print(package:get("links"))
    end)
package_end()