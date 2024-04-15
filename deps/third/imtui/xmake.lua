package("ImGuiTextEdit")
    add_urls("git@github.com:UE4SS-RE/ImGuiColorTextEdit.git")
    add_urls("https://github.com/UE4SS-RE/ImGuiColorTextEdit.git")

    add_versions("v1.0", "master")

    add_deps("cmake", "imtui v1.0.5")

    add_includedirs("include", { public = true })

    on_install(function (package)
        local xmake_lua = ([[
            add_rules("mode.debug", "mode.release")

            add_requires("imtui v1.0.5")

            target("ImGuiTextEdit")
                set_kind("static")
                set_languages("cxx20")

                add_includedirs(".", { public = true })
                add_headerfiles("*.h")

                add_files("*.cpp")

                add_packages("imgui")
        ]])
        io.writefile("xmake.lua", xmake_lua)

        import("package.tools.xmake").install(package)
    end)
package_end()

package("imtui")
    add_urls("https://github.com/ggerganov/imtui")

    add_versions("v1.0.5", "9f39c3e090c9b1e15557eac38a2f4389462f59df")
    add_patches("v1.0.5", "fix-size-cjk-and-mouse.patch")

    add_deps("cmake")

    -- imtui, imtui-ncurses and imgui-for-imtui
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DIMTUI_STANDALONE=ON")
        table.insert(configs, "-DIMTUI_SUPPORT_NCURSES=ON")
        table.insert(configs, "-DIMTUI_BUILD_EXAMPLES=OFF")

        import("package.tools.cmake").install(package, configs)
    end)
package_end()
