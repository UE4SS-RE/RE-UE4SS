set_xmakever("2.9.2")
-- We should use `get_config("ue4ssRoot")` instead of `os.projectdir()` or `$(projectdir)`.
-- This is because os.projectdir() will return a higher parent dir
-- when UE4SS is sub-moduled/`include("UE4SS")` in another xmake project.
set_config("ue4ssRoot", os.scriptdir())

-- All non-binary outputs are written to the Intermediates dir.
set_config("buildir", "Intermediates")

-- Any lua modules in this directory can be imported in the script scope by using
-- /modules/my_module.lua           import("my_module")
-- /modules/rules/my_module.lua     import("rules.my_module")
add_moduledirs("tools/xmakescripts/modules")

-- Add the plugins dir to support custom xmake <command>.
add_plugindirs("tools/xmakescripts/plugins")

-- Load our rule files into the global scope.
includes("tools/xmakescripts/rules/**.lua")

-- Generate the mode rules.
local modes = generate_compilation_modes()

-- Enter the existing ue4ss.base rule scope in order to add all xxx__xxx__xxx modes
-- to the ue4ss.base rule.
rule("ue4ss.base")
    for _, mode in ipairs(modes) do
        -- add_rules() expects the format `mode.Game__Shipping__Win64`
        add_deps("mode."..mode)
    end
rule_end()

-- Add the ue4ss.core rule to all targets within the UE4SS repository.
add_rules("ue4ss.core")

-- Restrict the compilation modes/configs.
-- These restrictions are inherited upstream and downstream.
-- Any project that `includes("UE4SS")` will inherit these global restrictions.
set_allowedplats("windows")
set_allowedarchs("x64")
set_allowedmodes(modes)

if is_host("windows") then
    set_defaultmode("Game__Shipping__Win64")
end

-- Override the `xmake install` behavior for all targets.
-- Targets can re-override the on_install() function to implement custom installation behavior.
on_install(function(target) end)

includes("deps")
includes("UE4SS")
includes("UVTD")
includes("cppmods")

-- TODO: Remove this before the next release. It only exists to maintain backwards compat
-- warnings for older mod templates.
set_config("scriptsRoot", path.join(os.scriptdir(), "tools/xmakescripts"))