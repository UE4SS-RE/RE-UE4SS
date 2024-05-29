---@alias rust_mode "dev" | "release"

--- Takes a target and returns context variables to be used in on_xxx overrides.
---@param target any xmake target
---@param is_debug boolean Should we get the debug or release context?
---@return string cargo_dir The root cargo dir for the project. Ex. Intermediates/Cargo/target/
---@return string cargo_mode_dir The cargo dir for the debug/release config. Ex. Intermediates/Cargo/target/debug/
---@return rust_mode rust_mode The rust flavor of mode derrived from is_debug param.
function get_cargo_context(target, is_debug)
    local rust_mode = is_debug and "dev" or "release"
    local rust_mode_dir = is_debug and "debug" or "release"
    local cargo_dir = path.join(get_config("buildir"), "cargo", target:name())
    return cargo_dir, path.join(cargo_dir, rust_mode_dir), rust_mode
end

--- Parses ".d" files created by Cargo.
---@param file string Path to a ".d" file created by Cargo
---@return string[] cargo_deps A list of any src files that should trigger a rebuild.
function get_dependencies(file)
    if not os.exists(file) then
        return {}
    end

    local data = io.readfile(file)
    data = data:trim()
    local start = data:find(": ", 1, false)
    local deps = data:sub(start + 2):split(" ", {strict = false, plain = true})

    local parsed_deps = {}
    local dep = ""
    local dep_idx = 1
    while dep do
        dep = deps[dep_idx]
        if dep then 
            while dep:endswith("\\") do
                dep_idx = dep_idx + 1
                dep = dep:sub(1, -2) .. " " .. deps[dep_idx]
            end
            table.insert(parsed_deps, dep)
            dep_idx = dep_idx + 1
        end
    end

    return parsed_deps
end

--[[ 
The previous function is based on the cargo impl of internal .d parsing.

let mut deps = line[pos + 2..].split_whitespace();

while let Some(s) = deps.next() {
    let mut file = s.to_string();
    while file.ends_with('\\') {
        file.pop();
        file.push(' ');
        file.push_str(deps.next().ok_or_else(|| {
            internal("malformed dep-info format, trailing \\".to_string())
        })?);
    }
    ret.files.push(file.into());
}
]]--