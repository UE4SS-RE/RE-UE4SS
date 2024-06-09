local hasWindows = is_plat("windows")
local uiMode = get_config("ue4ssUI")

if uiMode ~= nil and uiMode ~= "None" then
    local isTUI = uiMode == "TUI"
    add_requires("ImGuiTextEdit v1.0", { debug = is_mode_debug(), configs = { tui = isTUI, runtimes = get_mode_runtimes() } })

    if uiMode == "GUI" then
        local imguiConfig = { win32 = hasWindows, dx11 = hasWindows, opengl3 = true, glfw = true , runtimes = get_mode_runtimes()}
        add_requires("imgui v1.89", { alias = "ue4ssImGui", debug = is_mode_debug(), configs = imguiConfig } )
        add_requireconfs("ImGuiTextEdit.imgui", { configs = imguiConfig })

        add_requires("IconFontCppHeaders v1.0", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()}})
        add_requires("glfw 3.3.9", { debug = is_mode_debug() , configs = {runtimes = get_mode_runtimes()}})
        add_requires("opengl", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })
    elseif uiMode == "TUI" then
        add_requires("imtui v1.0.5", { debug = is_mode_debug(), configs = { runtimes = get_mode_runtimes() } })
    end
end

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

option("ue4ssUI")
    set_default("GUI")
    set_showmenu(true)

    set_values("None", "GUI", "TUI")

    set_description("UE4SS GUI Modes. TUI is not available on windows")

    after_check(function (option)
        local value = option:value()

        if value == "TUI" and is_plat("windows") then
            raise("TUI is not available on windows")
        end

        if value ~= "None" then
            option:add("defines", "HAS_UI")
        end

        if value == "GUI" then
            option:add("defines", "HAS_GUI")
        elseif value == "TUI" then
            option:add("defines", "HAS_TUI")
        end
    end)

option("ue4ssInput")
    set_default(true)
    set_showmenu(true)

    add_deps("ue4ssUI")
    add_defines("HAS_INPUT")

    set_description("Enable the input system.")

    after_check(function (option)
        local noUI = option:dep("ue4ssUI"):value() == "None"
        if noUI and not is_plat("windows") then
            option:enable(false)
        end
    end)

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
    add_options("ue4ssBetaIsStarted", "ue4ssIsBeta", "ue4ssUI", "ue4ssInput")
    add_includedirs("include", { public = true })
    add_includedirs("generated_include", { public = true })
    add_headerfiles("include/**.hpp")
    add_headerfiles("generated_include/*.hpp")

    add_files("src/**.cpp|Platform/**.cpp|GUI/**.cpp")
    
    if is_plat("windows") then
        add_files("src/Platform/Win32/CrashDumper.cpp", "src/Platform/Win32/EntryWin32.cpp")
        add_links("dbghelp", "psapi", { public = true })
    elseif is_plat("linux") then
        add_files("src/Platform/Linux/EntryLinux.cpp")
        add_shflags("-Wl,-soname,libUE4SS.so")
    end

    if uiMode ~= "None" then
        add_files("src/GUI/*.cpp")

        add_packages("ImGuiTextEdit", { public = true })

        if uiMode == "GUI" then
            add_files("src/GUI/Platform/GLFW/**.cpp")

            add_defines("HAS_GLFW", { public = true })

            add_deps("glad", { public = true })

            add_packages("ue4ssImGui", "IconFontCppHeaders", "glfw", "opengl", { public = true })

            if is_plat("windows") then
                add_files("src/GUI/Platform/D3D11/**.cpp")
                add_files("src/GUI/Platform/Windows/**.cpp")

                add_defines("HAS_D3D11", { public = true })

                add_links("d3d11", { public = true })
            end
        elseif uiMode == "TUI" then
            add_files("src/GUI/Platform/TUI/**.cpp")

            add_packages("imtui", { public = true })
        end
    end

    add_deps(
        "File", "DynamicOutput", "Unreal",
        "SinglePassSigScanner", "LuaMadeSimple", "Function",
        "IniParser", "JSON", 
        "Constructs", "Helpers", "MProgram",
        "ScopedTimer", "Profiler", "patternsleuth_bind",
        { public = true }
    )

    add_deps("Input", { public = true })

    add_packages("glaze", "polyhook_2", { public = true })

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