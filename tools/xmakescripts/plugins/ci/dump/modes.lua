import("core.project.project")
import("core.base.json")

function main()
    local modes = {}
    for _, mode in pairs(project.modes()) do
        table.append(modes, mode)
    end
    local jsonString = json.encode(modes)
    io.write(jsonString)
end