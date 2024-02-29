includes("build_configs/build_configs.lua", { rootdir = get_config("ue4ssRoot") })

includes("zycore")
includes("zydis")
includes("Polyhook_2_0")
includes("glaze")
includes("imgui")
includes("glad")
includes("raw_pdb")

add_requires("zycore", { debug = is_mode_debug(), config = { runtimes = get_mode_runtimes() } })
add_requires("zydis", { debug = is_mode_debug(), config = { runtimes = get_mode_runtimes() } })
add_requires("polyhook_2", { debug = is_mode_debug(), config = { runtimes = get_mode_runtimes() } })