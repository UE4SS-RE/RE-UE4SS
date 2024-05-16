--- Install the mod and dependencies to the installdir.
--- The installldir can be specified when running `xmake install --installdir="x/y/z/"`
---@param target any
function install(target)
    local mod_dir = path.join(target:installdir(), "Mods", target:name())
    os.cp(target:targetfile(), path.join(mod_dir, "dlls", "main.dll"))
    os.trycp(target:symbolfile(), path.join(mod_dir, "dlls", "main.pdb"))
    if not os.exists(path.join(mod_dir, "enabled.txt")) then
        io.writefile(path.join(mod_dir, "enabled.txt"))
    end

    -- Make sure we install any extra files. We can add these to the mod target by using
    -- add_extrafiles("xxx.yyy")
    for _, extrafile in ipairs(target:extrafiles()) do
        os.cp(extrafile, mod_dir)
    end
end