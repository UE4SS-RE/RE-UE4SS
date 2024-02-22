package("Tracy")
    add_urls("git@github.com:wolfpld/tracy.git")
    add_urls("https://github.com/wolfpld/tracy.git")

    add_versions("v0.10", "37aff70dfa50cf6307b3fee6074d627dc2929143")

    add_deps("cmake")

    on_install(function (package)
        local configs = {}

        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")

        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            static void test() {
                FrameMarkStart("Test start");
                FrameMarkEnd("Test end");
            }
        ]]}, {configs = {languages = "c++17"}, includes = {"tracy/Tracy.hpp"}}))
    end)