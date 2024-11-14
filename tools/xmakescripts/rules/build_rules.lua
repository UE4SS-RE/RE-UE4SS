-- The target/config/platform tables map unreal modes (Game__Shipping__Win64, etc.) to build settings.
-- The keys within each type should be a setting that xmake understands.
-- Example: ["defines"] = { "UE_GAME" } is equivalent to add_defines("UE_GAME") or target:add("defines", "UE_GAME")
-- All possible modes are generated from these target/config/platform tables.
local gameDefines = { "UE_GAME" }

local TARGET_TYPES = {
    ["Game"] = {
        ["defines"] = {
            table.unpack(gameDefines)
        }
    },
    ["CasePreserving"] = {
        ["defines"] = {
            "WITH_CASE_PRESERVING_NAME",
            table.unpack(gameDefines)
        }
    },
    ["LessEqual421"] = {
        ["defines"] = {
            "FNAME_ALIGN8",
            table.unpack(gameDefines)
        }
    }
}

local CONFIG_TYPES = {
    ["Dev"] = {
        ["symbols"] = {"debug"},
        ["defines"] = {
            "UE_BUILD_DEVELOPMENT",
            "STATS"
        },
        ["optimize"] = {"none"},
    },
    ["Debug"] = {
        ["symbols"] = {"debug"},
        ["defines"] = {
            "UE_BUILD_DEBUG"
        },
        ["optimize"] = {"none"},
    },
    ["Shipping"] = {
        ["symbols"] = {"debug"},
        ["defines"] = {
            "UE_BUILD_SHIPPING"
        },
        ["optimize"] = {"fastest"}
    },
    ["Test"] = {
        ["symbols"] = {"debug"},
        ["defines"] = {
            "UE_BUILD_TEST",
            "STATS"
        },
        ["optimize"] = {"none"}
    }
}

local PLATFORM_TYPES = {
    ["Win64"] = {
        ["defines"] = {
            "PLATFORM_WINDOWS",
            "PLATFORM_MICROSOFT",
            "OVERRIDE_PLATFORM_HEADER_NAME=Windows",
            "UBT_COMPILED_PLATFORM=Win64",
            "UNICODE",
            "_UNICODE"
        },

    }
}

-- The compile option tables map define what flags should be passed to each compiler.
-- The keys within each type should be a setting that xmake understands.
-- Example: ["cxflags"] = { "-g" } is equivalent to add_cxflags("-g") or target:add("cxflags", "-g")

local CLANG_COMPILE_OPTIONS = {
    ["cxflags"] = {
        "-g",
        "-gcodeview",
        "-fcolor-diagnostics",
        "-Wno-unknown-pragmas",
        "-Wno-unused-parameter",
        "-fms-extensions",
        "-Wignored-attributes"
    },
    ["ldflags"] = {
        "-g"
    },
    ["shflags"] = {
        "-g"
    }
}

local GNU_COMPILE_OPTIONS = {
    ["cxflags"] = {
        "-fms-extensions"
    }
}

local MSVC_COMPILE_OPTIONS = {
    ["cxflags"] = {
        "/MP",
        "/W3",
        "/wd4005",
        "/wd4251",
        "/wd4068",
        "/Zc:inline",
        "/Zc:strictStrings",
        "/Zc:preprocessor"  
    },
    ["ldflags"] = {
        "/DEBUG:FULL"
    },
    ["shflags"] = {
        "/DEBUG:FULL"
    },
    ["defines"] = {
        "_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR=1"
    }
}

--- Generate xmake modes for each of the target__config__platform permutations.
---@return table modes Table containing all target__config__platform permutations.
function generate_compilation_modes()
    local config_modes = {}
    for target_type, _ in pairs(TARGET_TYPES) do
        for config_type, _ in pairs(CONFIG_TYPES) do
            for platform_type, _ in pairs(PLATFORM_TYPES) do
                local config_name = target_type .. "__" .. config_type .. "__" .. platform_type
                table.append(config_modes, config_name)

                -- Modes are defined as rules with the `mode.` prefix. Ex: mode.Game__Shipping__Win64
                rule("mode."..config_name)
                    -- Only trigger the mode-specific logic if we are configured for this mode with `xmake f -m "Game__Shipping__Win64".
                    if is_mode(config_name) then
                        -- Apply the config options for this specific mode.
                        on_config(function(target)
                            import("mode_builder")
                            mode_builder.apply_mode_options(target, TARGET_TYPES[target_type])
                            mode_builder.apply_mode_options(target, CONFIG_TYPES[config_type])
                            mode_builder.apply_mode_options(target, PLATFORM_TYPES[platform_type])
                        end)
                    end
                rule_end()
            end
        end
    end

    return config_modes
end

--- Splits a mode string (Game__Shipping__Win64) into its component values.
---@param mode string
---@return string target_type
---@return string config_type
---@return string platform_type
local function mode_string_to_modes(mode)
    local modes = {}
    for t in string.gmatch(mode, "(%w+)") do
        table.insert(modes, t)
    end

    return modes[1], modes[2], modes[3]
end

--- This function determines if an unreal style mode (Game__Shipping__Win64) should be considered a debug mode.
--- This is useful when selecting the modes of dependencies that only have debug/release as valid modes.
---@return boolean debug Returns if the unreal mode is a debug mode.
function is_mode_debug()
    local mode = get_config("mode")
    if mode == nil then
        return false
    end

    local target, config, platform = mode_string_to_modes(mode)
    return config == "Debug" or config == "Dev"
end

--- This function returns any runtimes that need to be passed to targets or packages.
---@return string|string[] runtimes 
function get_mode_runtimes()
    if is_plat("windows") then
        return is_mode_debug() and "MDd" or "MD"
    end

    return {}
end

--- This local function wraps the global get_mode_runtimes() function. We have to locally wrap the function 
--- because we can only call locally scoped functions within the script scope.
---@see get_mode_runtimes
---@return string|string[] runtimes
local function _get_mode_runtimes()
    return get_mode_runtimes()
end

-- This rule is used to modify settings for ALL modes regardless of which mode is configured.
-- This mode inherits All modes (Game__Shipping__Win64, etc).
rule("ue4ss.base")
    on_load(function(target)
        -- Set the runtimes using the locally scoped runtime wrapper.
        target:set("runtimes", _get_mode_runtimes())

        import("mode_builder")
        -- Compiler flags are set in this rule since unreal modes currently do not change any compiler flags.
        mode_builder.apply_compiler_options(target, GNU_COMPILE_OPTIONS, {"gcc", "ld"})
        mode_builder.apply_compiler_options(target, CLANG_COMPILE_OPTIONS, {"clang", "lld"})
        mode_builder.apply_compiler_options(target, MSVC_COMPILE_OPTIONS, { "clang_cl", "cl", "link" })
    end)

    after_load(function(target)
        -- Binary outputs are written to the `Binaries` dir.
        target:set("targetdir", path.join(os.projectdir(), "Binaries", get_config("mode"), target:name()))
    end)

-- This rule is meant to be used by mod targets.
-- Logic that should ONLY apply to mods should be defined in this rule.
-- All other common logic is inherited from the ue4ss.base rule.
-- Example:
-- target("ExampleMod")
--   add_rules("ue4ss.mod")
--   add_includeirs(".")
--   add_files("dllmain.cpp")
rule("ue4ss.mod")
    add_deps("ue4ss.base", {order = true})
    after_load(function(target)
        target:set("kind", "shared")
        target:set("languages", "cxx23")
        target:set("exceptions", "cxx")
        target:add("deps", "UE4SS")
        target:set("group", "mods")
    end)

    on_install(function(target)
        import("mods.install").install(target)
    end)

-- This rule is meant to be used by all internal targets within UE4SS.
-- Logic that should ONLY apply to UE4SS targets (not mods) should be defined in this rule.
-- All other common logic is inherited from the ue4ss.base rule.
rule("ue4ss.core")
    add_deps("ue4ss.base", {order = true})
    after_load(function(target)
        local defines = {}
        import("target_helpers")
        for _, dep in pairs(target:deps()) do
            if dep:rule("ue4ss.defines.exports") then
                table.append(defines, target_helpers.project_name_to_exports_define(dep:name()))
            end
        end
        -- Incremental builds sometimes incorrectly report the need to rebuild if we do not sort the defines.
        table.sort(defines)
        target:add("defines", defines)
    end)

-- This rule applies defines to a target.
-- target ScopedTimer adds the flag -DRC_SCOPED_TIMER_BUILD_STATIC
rule("ue4ss.defines.static")
    after_load(function(target)
        import("target_helpers")
        target:add("defines", target_helpers.project_name_to_build_static_define(target:name()))
    end)

-- This rule applies defines to a target.
-- target ScopedTimer adds the flag -DRC_SCOPED_TIMER_EXPORTS
rule("ue4ss.defines.exports")
    after_load(function(target)
        import("target_helpers")
        target:add("defines", target_helpers.project_name_to_exports_define(target:name()))
    end)

-- This rule aggregates both the ue4ss.defines .static and .exports rules.
-- It also adds any target with this rule to the `deps` group which is used for
-- organization purposes within the generated VS
rule("ue4ss.dependency")
    add_deps("ue4ss.defines.static", "ue4ss.defines.exports")

    on_config(function(target)
        target:set("group", "deps")
    end)