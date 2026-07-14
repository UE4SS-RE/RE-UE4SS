package("glaze")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/stephenberry/glaze")
    set_description("Extremely fast, in memory, JSON and interface library for modern C++")
    set_license("MIT")

    add_urls("https://github.com/stephenberry/glaze/archive/refs/tags/$(version).tar.gz",
             "https://github.com/stephenberry/glaze.git")
    add_versions("v6.4.0", "20bdfcc6a97f632ba9f962565710a5dd37e9833f5629b898b3fc1ad301edce60")

    add_deps("cmake")

    on_install(function (package)
        import("package.tools.cmake").install(package, {
            "-Dglaze_DEVELOPER_MODE=OFF",
            "-DCMAKE_CXX_STANDARD=23",
            "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"),
            "-Dglaze_ENABLE_SSL=OFF"
        })
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test() {
                glz::generic value{};
            }
        ]]}, {configs = {languages = "c++23"}, includes = "glaze/glaze.hpp"}))
    end)
package_end()
