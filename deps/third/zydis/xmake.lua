package("zydis")
    set_urls("git@github.com:zyantific/zydis.git")

    add_versions("v3.1.0", "bfee99f49274a0eec3ffea16ede3a5bda9cda88f")

    add_deps("zycore", "cmake")

    on_install(function (package)
        local configs = {}
        
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")

        table.insert(configs, "-DZYDIS_BUILD_SHARED_LIB=OFF")
        
        table.insert(configs, "-DZYDIS_BUILD_TOOLS=OFF")
        table.insert(configs, "-DZYDIS_BUILD_EXAMPLES=OFF")
        
        table.insert(configs, "-DASMTK_NO_CUSTOM_FLAGS=ON")
        table.insert(configs, "-DASMJIT_NO_CUSTOM_FLAGS=ON")

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