package("glaze")
    set_urls("https://github.com/stephenberry/glaze.git")

    add_versions("v2.9.0", "53b69c447f98b89f656c0e6bda19833d682bb44b")

    add_deps("cmake")

    on_install(function (package)
        os.cp("include", package:installdir())
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <glaze/glaze.hpp>
            struct obj_t
            {
                double x{};
                float y{};
            };
            template <>
            struct glz::meta<obj_t>
            {
                using T = obj_t;
                static constexpr auto value = object("x", &T::x);
            };
            void test() {
                std::string buffer{};
                obj_t obj{};
                glz::write_json(obj, buffer);
            }
        ]]}, {configs = {languages = "c++20"}}))
    end)
package_end()