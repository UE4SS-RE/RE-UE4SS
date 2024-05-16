local build_configs = build_configs or {}

function config(self, target)
    if target:name() == "Unreal" then
        _warn_unreal_submod_outdated()
    else
        _warn_mod_template_outdated(target)
    end
end

function set_output_dir(self, target)
    if target:name() == "Unreal" then
        _warn_unreal_submod_outdated()
    else
        _warn_mod_template_outdated(target)
    end
end

function clean_output_dir(self, target)
    if target:name() == "Unreal" then
        _warn_unreal_submod_outdated()
    else
        _warn_mod_template_outdated(target)
    end
end

function _warn_mod_template_outdated(target)
    raise("Your mod's xmake.lua file needs updating.")
    print(format('target("%s")', target:name()))
    print('    add_rules("ue4ss.mod")')
    print('    add_includedirs(".")')
    print('    add_files("*.cpp")')
end

function _warn_unreal_submod_outdated()
    raise("Unreal submodule needs updating.")
end

return build_configs