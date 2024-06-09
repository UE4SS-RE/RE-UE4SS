package("ImGuiTextEdit")
    add_urls("git@github.com:UE4SS-RE/ImGuiColorTextEdit.git")
    add_urls("https://github.com/UE4SS-RE/ImGuiColorTextEdit.git")

    add_versions("v1.0", "master")

    add_configs("tui", { description = "Enable TUI.", default = false, type = "boolean" })

    add_includedirs("include", { public = true })

    local function _get_imgui_name(isTUI)
        if isTUI then
            return { name = "imtui", version = "v1.0.5" }
        else
            return { name = "imgui", version = "v1.89" }
        end
    end

    on_load(function (package)
        local imgui = _get_imgui_name(package:config("tui"))
        package:add("deps", string.format("%s %s", imgui.name, imgui.version))
    end)
    
    on_install(function (package)
        local imguiName = _get_imgui_name(package:config("tui")).name
        local imgui = package:dep(imguiName)

        local configs = imgui:requireinfo().configs

        if configs then 
            configs = string.serialize(configs, { strip = true, indent = false })
        end

        local xmake_lua = ([[
            add_rules("mode.debug", "mode.release")

            add_requires("%s %s", { configs = %s })

            target("ImGuiTextEdit")
                set_kind("static")
                set_languages("cxx20")

                add_includedirs(".", { public = true })
                add_headerfiles("*.h")

                add_files("*.cpp")

                add_packages("%s")
        ]]):format(imguiName, imgui:version_str(), configs, imguiName)
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
