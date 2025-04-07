task("iwyu")
    set_category("plugin")
    set_menu {
        usage = "xmake iwyu",
        description = "Runs include-what-you-use on the project.",
        options = {
            {nil, "params", "vs", nil, "Parameters to pass to iwyu."}
        }
    }

    on_run("iwyu")
task_end()