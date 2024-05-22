package("imtui")
    add_urls("https://github.com/ggerganov/imtui.git")

    set_policy("package.install_always", true)

    add_versions("v1.0.5", "9f39c3e090c9b1e15557eac38a2f4389462f59df")
    add_patches("v1.0.5", "fix-size-cjk-and-mouse.patch")

    -- WARN: xmake cannot distinguish ncursesw and ncurses, this may cause problem on other systems
    add_deps("cmake", "ncurses")

    add_includedirs("include", "include/imgui", "include/imgui-for-imtui/", "include/imgui-for-imtui/imgui", "include/imtui", {public = true})

    -- imtui, imtui-ncurses and imgui-for-imtui
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DIMTUI_STANDALONE=ON")
        table.insert(configs, "-DIMTUI_SUPPORT_NCURSES=ON")
        table.insert(configs, "-DIMTUI_BUILD_EXAMPLES=OFF")

        import("package.tools.cmake").install(package, configs)
        print(package:cachedir() .. "/source/imtui/")
        print(package:installdir())
        os.cp("third-party/imgui/imgui/misc/cpp/imgui_stdlib.h", package:installdir("include/imgui-for-imtui/imgui/misc/cpp"))
        os.cp("third-party/imgui/imgui/imgui_internal.h", package:installdir("include/imgui-for-imtui/imgui"))
        os.cp("third-party/imgui/imgui/imstb_textedit.h", package:installdir("include/imgui-for-imtui/imgui"))
    end)
package_end()
