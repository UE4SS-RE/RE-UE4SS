local projectName = "Profiler"

includes("deps/Tracy")

add_requires("Tracy", { optional = true })
add_requires("Superluminal", { system = true, optional = true })

option("profilerFlavor")
    set_default("Tracy")
    set_showmenu(true)
    set_values("Tracy", "Superluminal", "None")

target(projectName)
    set_kind("headeronly")
    add_options("profilerFlavor")
    set_languages("cxx20")

    add_includedirs("include", { public = true })
    add_headerfiles("include/**.hpp")

    on_load(function (target)
        local flavor = get_config("profilerFlavor")

        if flavor == "Tracy" then
            target:add("packages", "Tracy")
            target:add("defines", "IS_TRACY=1", "IS_SUPERLUMINAL=0")
        elseif flavor == "Superluminal" then
            target:add("packages", "Superluminal")
            target:add("defines", "IS_TRACY=0", "IS_SUPERLUMINAL=1")
        elseif flavor == "None" then
            target:add("defines", "IS_TRACY=0", "IS_SUPERLUMINAL=0", "DISABLE_PROFILER")
        end
    end)
    