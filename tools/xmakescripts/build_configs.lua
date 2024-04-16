-- TODO: This file should be deleted before the next release?
-- It helps people with old mod templates upgrade to the new template.
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
            "_UNICODE",
            "DLLEXT=.dll"
        },
        ["cxflags"] = {
            "clang_cl::-gcodeview"
        }
    },
    ["Linux"] = {
        ["defines"] = {
            "PLATFORM_LINUX",
            "PLATFORM_UNIX",
            "LINUX",
            "OVERRIDE_PLATFORM_HEADER_NAME=Linux",
            "UBT_COMPILED_PLATFORM=Linux",
            "printf_s=printf",
            "DLLEXT=.so"
        },
        ["cxflags"] = {
            "clang::-fno-delete-null-pointer-checks"
        }
    }
}

CLANG_COMPILE_OPTIONS = {
    ["cxflags"] = {
        "-g",
        "-fcolor-diagnostics",
        "-Wno-unknown-pragmas",
        "-Wno-unused-parameter",
        "-fms-extensions",
        "-Wignored-attributes",
        "-fPIC"
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
    if target:name() == "Unreal" then
        _warn_unreal_submod_outdated()
    else
        _warn_mod_template_outdated(target)
    end
end

function set_output_dir(self, target)
    if target:name() == "Unreal" then
        _warn_unreal_submod_outdated()
    else
        _warn_mod_template_outdated(target)
    end
end

function clean_output_dir(self, target)
    if target:name() == "Unreal" then
        _warn_unreal_submod_outdated()
    else
        _warn_mod_template_outdated(target)
    end
end

-- Get runtime for current mode
function get_mode_runtimes()
    local is_debug = is_mode_debug()
    if is_plat("windows") then
        return is_debug and "MDd" or "MD"
    end
    -- we don't care about runtime on linux
    return ""
end

function _warn_unreal_submod_outdated()
    raise("Unreal submodule needs updating.")
end

return build_configs