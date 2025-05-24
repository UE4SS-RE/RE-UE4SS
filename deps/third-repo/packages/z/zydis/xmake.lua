package("zydis")
    add_urls("git@github.com:zyantific/zydis.git")
    add_urls("https://github.com/zyantific/zydis.git")

    add_versions("v4.1.1", "a2278f1d254e492f6a6b39f6cb5d1f5d515659dc")

    add_deps("zycore", "cmake")

    add_defines("ZYDIS_STATIC_BUILD", { public = true })

    on_install(function (package)
        local configs = {}
        
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DZYDIS_BUILD_SHARED_LIB=OFF")

        table.insert(configs, "-DZYDIS_BUILD_TOOLS=OFF")
        table.insert(configs, "-DZYDIS_BUILD_EXAMPLES=OFF")

        import("package.tools.cmake").install(package, configs, { packagedeps = "zycore" })
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <Zydis/Zydis.h>
            #include <Zycore/LibC.h>
            void test() {
                ZyanU8 encoded_instruction[ZYDIS_MAX_INSTRUCTION_LENGTH];
                ZyanUSize encoded_length = sizeof(encoded_instruction);
            }
        ]]}, {configs = {languages = "c++11"}}))
    end)
package_end()