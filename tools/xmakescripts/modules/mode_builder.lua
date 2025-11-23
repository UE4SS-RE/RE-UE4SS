--- Applies options to a target.
---@param target any xmake target
---@param opts table xmake options ex. ["defines"] = { "UE_GAME" }
function apply_mode_options(target, opts)
    for option, values in pairs(opts) do
        target:add(option, values, { public = true })
    end
end

--- Applies build options to a target.
---@param target any xmake target
---@param opts table<string, string[]> Map of xmake flag types to flags. Ex. ["ldflags"] = {"/DEBUG:FULL"} 
---@param tools string[] xmake compiler/linker tools. Ex {"gcc", "ld" }
function apply_compiler_options(target, opts, tools)
    for option, values in pairs(opts) do
        target:add(option, values, { tools = tools })
    end
end