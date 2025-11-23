-- TODO: This file should be deleted before the next release?
-- It helps people with old mod templates upgrade to the new template.
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
    print("SUGGESTED TEMPLATE:")
    print(format('target("%s")', target:name()))
    print('    add_rules("ue4ss.mod")')
    print('    add_includedirs(".")')
    print('    add_files("*.cpp")')
    raise("Your mod's xmake.lua file needs updating.")
end

function _warn_unreal_submod_outdated()
    raise("Unreal submodule needs updating.")
end

return build_configs