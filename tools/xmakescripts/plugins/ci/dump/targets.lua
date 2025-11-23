import("core.project.config")
import("core.project.project")
import("core.base.json")
import("core.base.task")

function main()
    project.lock()
    -- Check to ensure that config has been run. This will not trample the existing config if it exists.
    task.run("config", {yes=true}, {disable_dump = true})
    project.load_targets()
    project.unlock()

    local targets = {}
    for targetname, target in pairs(project.targets()) do
        targets[targetname] = {target = target:targetfile(), symbol = target:symbolfile()}
    end

    local jsonString = json.encode(targets)
    io.write(jsonString)
end