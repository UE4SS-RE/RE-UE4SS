package("imtui")
    add_urls("https://github.com/ggerganov/imtui.git")

    add_versions("v1.0.5", "9f39c3e090c9b1e15557eac38a2f4389462f59df")
    local root = get_config("ue4ssRoot")
    add_patches("v1.0.5", path.join(root, "deps", "third", "imtui", "fix-size-cjk-and-mouse.patch"))

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
