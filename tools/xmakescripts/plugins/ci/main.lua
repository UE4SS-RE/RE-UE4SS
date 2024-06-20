import("core.base.option")

function main()
    if option.get("dump") then
        local module_name = string.format("dump.%s", string.lower(option.get("dump")))
        assert(import(module_name, {try = true, anonymous = true}))()
    else
        raise("No options provided to the xmake ci command.")
    end
end