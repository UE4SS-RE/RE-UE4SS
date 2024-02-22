package("ImGuiTextEdit")
    add_urls("git@github.com:UE4SS-RE/ImGuiColorTextEdit.git")
    add_urls("https://github.com/UE4SS-RE/ImGuiColorTextEdit.git")

    add_versions("v1.0", "master")

    add_deps("cmake", "imgui v1.89")

    add_includedirs("include", { public = true })

    on_install(function (package)
        local imgui = package:dep("imgui")
        local configs = imgui:requireinfo().configs

        if configs then 
            configs = string.serialize(configs, { strip = true, indent = false })
        end

        local xmake_lua = ([[
            add_rules("mode.debug", "mode.release")

            add_requires("imgui %s", { configs = %s })

            target("ImGuiTextEdit")
                set_kind("static")
                set_languages("cxx20")

                add_includedirs(".", { public = true })
                add_headerfiles("*.h")

                add_files("*.cpp")

                add_packages("imgui")
        ]]):format(imgui:version_str(), configs)
        io.writefile("xmake.lua", xmake_lua)

        import("package.tools.xmake").install(package)
    end)
package_end()

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