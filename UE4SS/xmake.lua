includes("proxy_generator")

add_requires("imgui v1.89", { configs = { win32 = true, dx11 = true, opengl3 = true, glfw_opengl3 = true } })
add_requires("ImGuiTextEdit v1.0")
add_requires("IconFontCppHeaders v1.0")
add_requires("glfw 3.3.9")
add_requires("opengl")

option("ue4ssBetaIsStarted")
    set_default(true)
    set_showmenu(true)
    set_values(true, false)

    set_description("Have beta releases started for the current major version")

option("ue4ssIsBeta")
    set_default(true)
    set_showmenu(true)
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
    set_languages("cxx20")

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
        "glad"
    )
    add_packages("imgui", "ImGuiTextEdit", "IconFontCppHeaders", "glfw", "opengl")

    add_packages("glaze", "polyhook_2")

    add_links("dbghelp", "psapi", "d3d11")

    after_load(function (target)
        import("build_configs.build_configs", { rootdir = os.projectdir() })
        import("target_helpers", { rootdir = os.projectdir() })
        
        print("Project: " .. projectName .. " (SHARED)")

        build_configs:set_output_dir(target)

        target:add("defines", target_helpers.project_name_to_exports_define(projectName))
        
        local version_string = io.readfile(path.join(target:scriptdir(), "generated_src/version.cache"))
        local version = parse_version_string(version_string)

        target:add("defines", "UE4SS_LIB_VERSION_MAJOR=" .. version.major, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_MINOR=" .. version.minor, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_HOTFIX=" .. version.hotfix, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_PRERELEASE=" .. version.prerelease, { public = true })
        target:add("defines", "UE4SS_LIB_VERSION_BETA=" .. version.beta, { public = true })
        
        target:add("defines", "UE4SS_LIB_BETA_STARTED=" .. (get_config("ue4ssBetaIsStarted") and "1" or "0"), { public = true })
        target:add("defines", "UE4SS_LIB_IS_BETA=" .. (get_config("ue4ssIsBeta") and "1" or "0"), { public = true })

        local outdata, _ = os.iorunv("git", {"rev-parse", "--short", "HEAD"})
        local commit_hash = outdata:gsub("%s+$", "")
        target:add("defines", "UE4SS_LIB_BUILD_GITSHA=\"" .. commit_hash .. "\"", { public = true })
        
        target:add("defines", "UE4SS_CONFIGURATION=\"" .. get_config("mode") .. "\"", { public = true })
    end)

