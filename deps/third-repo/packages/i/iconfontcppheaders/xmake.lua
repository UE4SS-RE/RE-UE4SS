package("IconFontCppHeaders")
    add_urls("git@github.com:juliettef/IconFontCppHeaders.git")
    add_urls("https://github.com/juliettef/IconFontCppHeaders.git")
    set_kind("library", { headeronly = true })

    add_versions("v1.0", "main")

    on_install(function (package)
        os.cp("**.h", package:installdir("include"))
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({ test = [[
            void test() {
                ICON_FA_TERMINAL;
            }
        ]]}, { includes = { "IconsFontAwesome5.h" } }))
    end)
package_end()