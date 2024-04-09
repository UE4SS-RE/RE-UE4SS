-- define module: build_configs
local build_configs = build_configs or {}

local gameDefines = { "UE_GAME" }

TARGET_TYPES = {
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

CONFIG_TYPES = {
    ["Dev"] = {
        ["symbols"] = {"debug"},
        ["defines"] = {
            "UE_BUILD_DEVELOPMENT",
            "STATS"
        },
        ["optimize"] = {"none"}
    },
    ["Debug"] = {
        ["symbols"] = {"debug"},
        ["defines"] = {
            "UE_BUILD_DEBUG"
        },
        ["optimize"] = {"none"}
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

PLATFORM_TYPES = {
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

CLANG_COMPILE_OPTIONS = {
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

GNU_COMPILE_OPTIONS = {
    ["cxflags"] = {
        "-fms-extensions"
    }
}

MSVC_COMPILE_OPTIONS = {
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

-- Get target types
function get_target_types()
    return TARGET_TYPES
end

-- Get config types
function get_config_types()
    return CONFIG_TYPES
end

-- Get platform types
function get_platform_types()
    return PLATFORM_TYPES
end

-- Get clang compile options
function get_clang_compile_options()
    return CLANG_COMPILE_OPTIONS
end

-- Get gnu compile options
function get_gnu_compile_options()
    return GNU_COMPILE_OPTIONS
end

-- Get msvc compile options
function get_msvc_compile_options()
    return MSVC_COMPILE_OPTIONS
end

-- Apply targe options
function apply_target_options(self, target, options)
    for option, values in pairs(options) do
        target:add(option, values, { public = true })
    end
end

-- Returns a list of supported compilation modes
-- CasePreserving__Shipping_Win64, Game_Debug_Win64, etc.
function get_compilation_modes()
    local comp_modes = {}

    for target_type, _ in pairs(TARGET_TYPES) do
        for config_type, _ in pairs(CONFIG_TYPES) do
            for platform_type, _ in pairs(PLATFORM_TYPES) do
                local config_name = target_type .. "__" .. config_type .. "__" .. platform_type
                table.insert(comp_modes, config_name)
            end
        end
    end

    return comp_modes
end

-- Get unreal rules
function get_unreal_rules()
    local unreal_rules = {}

    for _, config_name in ipairs(get_compilation_modes()) do
        local rule_name = "mode." .. config_name
        table.insert(unreal_rules, rule_name)

        rule(rule_name)
        rule_end()
    end

    return unreal_rules
end

-- Parse mode string into modes
function mode_string_to_modes(self, str)
    local modes = {}
    for t in string.gmatch(str, "(%w+)") do
        table.insert(modes, t)
    end

    return {
        ["target"] = modes[1],
        ["config"] = modes[2],
        ["platform"] = modes[3]
    }
end

-- Apply compiler options
function apply_compiler_options(self, target)
    for option, values in pairs(self:get_gnu_compile_options()) do
        target:add(option, values, { tools = { "gcc", "ld" } })
    end

    for option, values in pairs(self:get_clang_compile_options()) do
        target:add(option, values, { tools = { "clang", "lld" } })
    end

    for option, values in pairs(self:get_msvc_compile_options()) do
        target:add(option, values, { tools = { "clang_cl", "cl", "link" } })
    end
end

-- Run on configure step for each target that wants unreal rules
function config(self, target)
    import("target_helpers", { rootdir = get_config("scriptsRoot") })

    local mode = get_config("mode")
    local modes = self:mode_string_to_modes(mode)

    local target_options = self:get_target_types()[modes.target]
    local config_options = self:get_config_types()[modes.config]
    local platform_options = self:get_platform_types()[modes.platform]

    self:apply_target_options(target, target_options)
    self:apply_target_options(target, config_options)
    self:apply_target_options(target, platform_options)

    self:apply_compiler_options(target)

    target:set("runtimes", get_mode_runtimes())
end

-- Construct output dir for target
function construct_output_dir(self, target)
    local mode = get_config("mode")
    local output_dir = path.join("Binaries", mode, target:name())
    return output_dir
end

-- Run after load step for each target that wants custom output dir
function set_output_dir(self, target)
    local output_dir = self:construct_output_dir(target)
    target:set("targetdir", output_dir)
end

-- Run after load step for each target that has dependencies with `ue4ssDep`: true
function export_deps(self, target)
    import("target_helpers", { rootdir = get_config("scriptsRoot") })

    local additional_defines = {}
    for _, dep in pairs(target:deps()) do
        if dep:values("ue4ssDep") == true then
            table.insert(additional_defines, target_helpers.project_name_to_build_static_define(dep:name()))
        end
    end
    table.sort(additional_defines)

    for _, define in pairs(additional_defines) do
        target:add("defines", define)
    end
end


-- If a target is a "ue4ssDep", add the target to the "deps" group for VS organization. 
function set_project_groups(self, target)
    for _, dep in pairs(target:deps()) do
        if dep:values("ue4ssDep") == true then
            dep:set("group", "deps")
        end
    end
end

-- Run after clean step for each target that has custom output dir
function clean_output_dir(self, target)
    local output_dir = self:construct_output_dir(target)
    os.rm(output_dir)
end

-- Get if current mode is debug
function is_mode_debug()
    local mode = get_config("mode")
    if mode == nil then
        return false
    end

    local modes = mode_string_to_modes(nil, mode)
    return modes.config == "Debug" or modes.config == "Dev"
end

-- Get runtime for current mode
function get_mode_runtimes()
    local is_debug = is_mode_debug()
    return is_debug and "MDd" or "MD"
end

-- return module: build_configs
return build_configs
