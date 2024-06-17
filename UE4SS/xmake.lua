includes("proxy_generator")

add_requires("imgui v1.89", { debug = is_mode_debug(), configs = { win32 = true, dx11 = true, opengl3 = true, glfw_opengl3 = true , runtimes = get_mode_runtimes()} } )
add_requires("ImGuiTextEdit v1.0", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })
add_requires("IconFontCppHeaders v1.0", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()}})
add_requires("glfw 3.3.9", { debug = is_mode_debug() , configs = {runtimes = get_mode_runtimes()}})
add_requires("opengl", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })
add_requires("glaze", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })
add_requires("fmt", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })

option("ue4ssBetaIsStarted")
    set_default(true)
    set_showmenu(true)
    -- Sets the possible options to only be true or false.
    set_values(true, false)

    set_description("Have beta releases started for the current major version")

option("ue4ssIsBeta")
    set_default(true)
    set_showmenu(true)
    -- Sets the possible options to only be true or false.
    set_values(true, false)

    set_description("Is this a beta release")

local projectName = "UE4SS"

local function parse_version_string(str)
    local matches = {}
    for match in str:gmatch("(%d+)") do
        table.insert(matches, match)
    end

    return {
        ["major"] = matches[1],
        ["minor"] = matches[2],
        ["hotfix"] = matches[3],
        ["prerelease"] = matches[4],
        ["beta"] = matches[5]
    }
end

target(projectName)
    set_kind("shared")
    set_languages("cxx23")
    set_exceptions("cxx")
    set_default(true)
    add_rules("ue4ss.defines.exports")
    add_options("ue4ssBetaIsStarted", "ue4ssIsBeta")
    add_includedirs("include", { public = true })
    add_includedirs("generated_include", { public = true })
    add_headerfiles("include/**.hpp")
    add_headerfiles("generated_include/*.hpp")

    add_files("src/**.cpp")

    add_deps(
        "File", "DynamicOutput", "Unreal",
        "SinglePassSigScanner", "LuaMadeSimple", "Function",
        "IniParser", "JSON", "Input",
        "Constructs", "Helpers", "MProgram",
        "ScopedTimer", "Profiler", "patternsleuth_bind",
        "glad", { public = true }
    )
    
    add_packages("fmt", { public = true })

    add_packages("imgui", "ImGuiTextEdit", "IconFontCppHeaders", "glfw", "opengl", { public = true })

    add_packages("glaze", "polyhook_2", { public = true })

    add_links("dbghelp", "psapi", "d3d11", { public = true })

    after_load(function (target)
        local projectRoot = get_config("ue4ssRoot")
        local version_string = io.readfile(path.join(target:scriptdir(), "generated_src/version.cache"))
        local version = parse_version_string(version_string)

        target:add("defines", "UE4SS_LIB_VERSION_MAJOR=" .. version.major, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_MINOR=" .. version.minor, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_HOTFIX=" .. version.hotfix, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_PRERELEASE=" .. version.prerelease, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_BETA=" .. version.beta, { public = true })
        
        target:add("defines", "UE4SS_LIB_BETA_STARTED=" .. (get_config("ue4ssBetaIsStarted") and "1" or "0"), { public = true })
        target:add("defines", "UE4SS_LIB_IS_BETA=" .. (get_config("ue4ssIsBeta") and "1" or "0"), { public = true })

        -- Attempt to get the latest git commit from the UE4SS root directory.
        -- We have to be explicit about running it in the root UE4SS directory
        -- in the case where RE-UE4SS is submoduled in another git repo.

        import("lib.detect.find_tool")
        local git = assert(find_tool("git"), "git not found!")

        -- init arguments
        local argv = {"rev-parse", "--short", "HEAD"}
        local lastcommit = os.iorunv(git.program, argv, {curdir = get_config("ue4ssRoot")})
        if lastcommit then
            lastcommit = lastcommit:trim()
        end

        target:add("defines", "UE4SS_LIB_BUILD_GITSHA=\"" .. lastcommit .. "\"", { public = true })
        target:add("defines", "UE4SS_CONFIGURATION=\"" .. get_config("mode") .. "\"", { public = true })
    end)

    on_install(function(target)
        os.mkdir(target:installdir())
        os.cp(target:targetfile(), target:installdir())
        if target:symbolfile() then
            os.cp(target:symbolfile(), target:installdir())
        end
    end)