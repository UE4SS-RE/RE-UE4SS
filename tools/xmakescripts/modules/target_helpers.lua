--- Convert to SCREAMING_SNAKE_CASE
---@param s string
---@return string
function _to_screaming_snake_case(s)
    if s == "ASMHelper" then
        return "ASM_HELPER"
    end

    if s:find("_") == nil and s:upper() == s then
        return s
    end
    return string.upper(string.gsub(s, "(.)([A-Z])", "%1_%2"))
end

--- Convert project name to exports define
---@param name string
---@return string
function project_name_to_exports_define(name)
    return "RC_" .. _to_screaming_snake_case(name) .. "_EXPORTS"
end

--- Convert project name to build static define
---@param name string
---@return string
function project_name_to_build_static_define(name)
    return "RC_" .. _to_screaming_snake_case(name) .. "_BUILD_STATIC"
end