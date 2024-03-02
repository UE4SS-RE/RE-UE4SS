set_config("ue4ssRoot", os.curdir())

includes("build_configs/build_configs.lua")

add_rules(get_unreal_rules())

set_runtimes(get_mode_runtimes())

-- Tell WinAPI macros to map to unicode functions instead of ansi
add_defines("_UNICODE", "UNICODE")

after_load(function (target)
    import("build_configs.build_configs", { rootdir = get_config("ue4ssRoot") })
    import("target_helpers", { rootdir = get_config("ue4ssRoot") })
    build_configs:set_output_dir(target)
    build_configs:export_deps(target)
end)

on_config(function (target)
    import("build_configs.build_configs", { rootdir = get_config("ue4ssRoot") })
    build_configs:config(target)
end)

after_clean(function (target)
    import("build_configs.build_configs", { rootdir = get_config("ue4ssRoot") })
    build_configs:clean_output_dir(target)
end)

includes("deps")
includes("UE4SS")
includes("UVTD")