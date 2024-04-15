-- UI configurations

option("UI")
    set_default(true)
    set_showmenu(true)
    add_defines("HAS_UI")
    set_description("Enable the user iterface.")

option("GUI")
    -- Windows has only GUI
    set_default(is_plat("windows"))
    set_showmenu(true)
    add_defines("HAS_GUI")
    add_deps("UI")
    set_description("Enable the graphical user interface.")
    if is_plat("windows") then
        add_defines("HAS_D3D11")
        add_defines("HAS_GLFW")
    elseif is_plat("linux") then
        add_defines("HAS_GLFW")
    end
    
option("TUI")
    -- use TUI by default on Linux
    set_default(is_plat("linux"))
    set_showmenu(true)
    add_defines("HAS_TUI")
    add_deps("UI")
    set_description("Enable the text user interface. (Not supported on Windows)")
    after_check(function (option)
        -- mutually exclusive with GUI
        if has_config("GUI") then
            option:enable(false)
        end
        if is_plat("windows") then
            option:enable(false)
        end
    end)

-- Input configurations

option("Input")
    set_default(has_config("UI") or is_plat("windows"))
    set_showmenu(true)
    add_defines("HAS_INPUT")
    set_description("Enable the input system.")
    after_check(function (option)
        if not has_config("UI") and not is_plat("windows") then
            option:enable(false)
        end
    end)