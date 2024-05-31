-- First party dependencies
includes("first")
-- Third party dependencies
includes("third")
-- Third party dependencies repository
-- Everything that is an xmake package should be inside of this repository
add_repositories("third-party deps/third-repo", { rootdir = get_config("ue4ssRoot") })

add_requires("zycore v1.5.0", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })
add_requires("zydis v4.1.0", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })
add_requires("polyhook_2", { debug = is_mode_debug(), configs = {runtimes = get_mode_runtimes()} })