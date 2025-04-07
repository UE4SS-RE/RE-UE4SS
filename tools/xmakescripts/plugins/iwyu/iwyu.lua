import("core.base.task")
import("core.base.option")
import("core.project.project")
import("lib.detect.find_file")
import("detect.tools.find_python3")

function main()
    local params = option.get("params")

    local python = find_python3()
    if not python then
        raise("python not found")
    end

    local iwyu = find_file("iwyu_tool.py", { "$(env PATH)" })
    if not iwyu then
        raise("include-what-you-use not found")
    end

    local outputdir = os.tmpfile() .. ".dir"
    local filename = "compile_commands.json"
    filepath = outputdir and path.join(outputdir, filename) or filename
    task.run("project", {quiet = true, kind = "compile_commands", lsp = "clangd", outputdir = outputdir})

    local runArgs = {iwyu, "-p", filepath}
    local args = table.join(runArgs, params)

    print("Starting iwyu with args:", args)
    os.execv(python, args)
end