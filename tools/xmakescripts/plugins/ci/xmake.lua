task("ci")
    set_category("plugin")
    on_run("main")

    set_menu {
            usage = "xmake ci [options]",
            description = "Pass build information to external tools.",
            options =
            {
                {'d', "dump", "kv", nil, "Dump the specified information in JSON format.",
                    values =  {"modes", "targets"} }
            }
        }