-- Convert to SCREAMING_SNAKE_CASE
function to_screaming_snake_case(s)
    if s == "ASMHelper" then
        return "ASM_HELPER"
    end

    if s:find("_") == nil and s:upper() == s then
        return s
    end
    return string.upper(string.gsub(s, "(.)([A-Z])", "%1_%2"))
end

-- Convert project name to exports define
function project_name_to_exports_define(name)
    return "RC_" .. to_screaming_snake_case(name) .. "_EXPORTS"
end

-- Convert project name to build static define
function project_name_to_build_static_define(name)
    return "RC_" .. to_screaming_snake_case(name) .. "_BUILD_STATIC"
end

-- Get file from path
function get_path_file(path)
    return string.match(string.gsub(path, "\\", "/"), "^.+/(.+)$")
end

-- Get file name from path
function get_path_filename(path)
    return string.gsub(get_path_file(path), "%..*", "")
end
