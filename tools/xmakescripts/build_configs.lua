local gameDefines = { "UE_GAME" }

-- The target/config/platform tables map unreal modes (Game__Shipping__Win64, etc.) to build settings.
-- The keys within each type should be a setting that xmake understands.
-- Example: ["defines"] = { "UE_GAME" } is equivalent to add_defines("UE_GAME") or target:add("defines", "UE_GAME")
-- All possible modes are generated from these target/config/platform tables.

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
        }
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
                        -- Inherit our base rule that should run regardless of the configured mode.
                        add_deps("ue4ss.mode.base")

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

-- This rule is used to modify settings for ALL modes regardless of which mode is configured.
-- All modes (Game__Shipping__Win64, etc) inherit this mode.
rule("ue4ss.mode.base")
    on_config(function(target)
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

-- This rule applies defines to a target and all upstream targets.
-- target ScopedTimer adds the flag -DRC_SCOPED_TIMER_BUILD_STATIC
rule("ue4ss.defines.static")
    after_load(function(target)
        import("target_helpers")
        target:add("defines", target_helpers.project_name_to_build_static_define(target:name()), {public = true})
    end)

-- This rule applies defines to a target and all upstream targets.
-- target ScopedTimer adds the flag -DRC_SCOPED_TIMER_EXPORTS
rule("ue4ss.defines.exports")
    after_load(function(target)
        import("target_helpers")
        target:add("defines", target_helpers.project_name_to_exports_define(target:name()),{public = true})
    end)

-- This rule aggregates both the ue4ss.defines .static and .exports rules.
-- It also adds any target with this rule to the `deps` group which is used for
-- organization purposes within the generated VS solution.
rule("ue4ss.dependency")
    add_deps("ue4ss.defines.static", "ue4ss.defines.exports")

    on_config(function(target)
        target:set("groups", "deps")
    end)

