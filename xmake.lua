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

-- Load the build_configs file into the global scope.
includes("tools/xmakescripts/build_configs.lua")

-- Generate the modes and add them to all targets.
local modes = generate_compilation_modes()

for _, mode in ipairs(modes) do
    -- add_rules() expects the format `mode.Game__Shipping__Win64`
    add_rules("mode."..mode)
end

if is_plat("windows") then
    -- Globally set the runtimes for all targets.
    set_runtimes(is_mode_debug() and "MDd" or "MD")
end

-- Restrict the compilation modes/configs.
-- These restrictions are inherited upstream and downstream.
-- Any project that `includes("UE4SS")` will inherit these global restrictions.
set_allowedplats("windows")
set_allowedarchs("x64")
set_allowedmodes(modes)

if is_plat("windows") then
    set_defaultmode("Game__Shipping__Win64")
end

-- Override the `xmake install` behavior for all targets.
-- Targets can re-override the on_install() function to implement custom installation behavior.
on_install(function(target) end)

includes("deps")
includes("UE4SS")
includes("UVTD")