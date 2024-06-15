package("polyhook_2")
    set_homepage("https://github.com/stevemk14ebr/PolyHook_2_0")
    set_description("C++17, x86/x64 Hooking Libary v2.0")
    set_license("MIT")

    add_urls("https://github.com/stevemk14ebr/PolyHook_2_0.git")
    
    add_versions("2024.2.07", "fd2a88f09c8ae89440858fc52573656141013c7f")

    add_configs("shared_deps", {description = "Use shared library for dependency", default = false, type = "boolean"})
    add_configs("external_zydis", {description = "Use external zydis", default = false, type = "boolean"})

    add_deps("cmake")

    on_load(function (package)
        if package:config("external_zydis") then
            -- Zydis version is derrived from add_requires("zydis <version>") in this project.
            package:add("deps", "zydis")
        end
    end)

    on_install("windows|x86", "windows|x64", "linux|i386", "linux|x86_64", function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DPOLYHOOK_BUILD_SHARED_LIB=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DPOLYHOOK_USE_EXTERNAL_ZYDIS="..(package:config("external_zydis") and "ON" or "OFF"))

        if package:is_plat("windows") then
            local static_runtime = package:runtimes():startswith("MT")
            table.insert(configs, "-DPOLYHOOK_BUILD_STATIC_RUNTIME=" .. (static_runtime and "ON" or "OFF"))
            if not static_runtime then
                table.insert(configs, "-DPOLYHOOK_BUILD_SHARED_ASMTK=ON")
                table.insert(configs, "-DPOLYHOOK_BUILD_SHARED_ASMJIT=ON")
            end
        end
        if package:config("shared_deps") then
            if not package:is_plat("windows") then
                table.insert(configs, "-DPOLYHOOK_BUILD_SHARED_ASMTK=ON")
                table.insert(configs, "-DPOLYHOOK_BUILD_SHARED_ASMJIT=ON")
            end
            table.insert(configs, "-DPOLYHOOK_BUILD_SHARED_ZYDIS=ON")
        end
        
        local opts = {buildir = "build"}
        local links = {"PolyHook_2", "asmtk", "asmjit"}

        if package:config("external_zydis") then
            -- Allow xmake to link with the existing zydis package instead of its own.
            opts.packagedeps = "zydis"
        else
            table.insert(links, "Zydis")
            table.insert(links, "Zycore")
        end

        import("package.tools.cmake").install(package, configs, opts)
        package:add("links", links)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <iostream>
            static void test() {
                PLH::NatDetour detour(0, 0, nullptr);
            }
        ]]}, {includes = "polyhook2/Detour/NatDetour.hpp", configs={languages = "c++17"}}))
    end)
package_end()
