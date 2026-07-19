#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <Mod/CppMod.hpp>
#include <UE4SSProgram.hpp>

namespace
{
    auto read_lines(const std::filesystem::path& path) -> std::vector<std::string>
    {
        std::ifstream input{path};
        std::vector<std::string> lines{};
        for (std::string line{}; std::getline(input, line);)
        {
            lines.emplace_back(std::move(line));
        }
        return lines;
    }
} // namespace

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        return 2;
    }

    const auto unique_suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / ("ue4ss-cpp-mod-loader-" + std::to_string(unique_suffix));
    const auto mod_root = root / "CppModProbe";
    const auto dlls_root = mod_root / "dlls";
    const auto log_path = root / "lifecycle.log";

    std::filesystem::create_directories(dlls_root);
    std::filesystem::copy_file(argv[1], dlls_root / "libCppModProbe.so", std::filesystem::copy_options::overwrite_existing);
    if (setenv("CPP_MOD_PROBE_LOG", log_path.c_str(), 1) != 0)
    {
        std::filesystem::remove_all(root);
        return 3;
    }

    int result{};
    {
        RC::UE4SSProgram program{std::filesystem::path{argv[2]}, {}};
        RC::CppMod mod{program, STR("CppModProbe"), mod_root};
        if (!mod.is_installable())
        {
            result = 4;
        }
        else
        {
            mod.start_mod();
            if (!mod.is_started())
            {
                result = 5;
            }
            else
            {
                mod.fire_program_start();
                mod.fire_unreal_init();
                mod.fire_update();
                mod.fire_on_cpp_mods_loaded();
                mod.fire_dll_load(STR("fixture.so"));
                mod.uninstall();
            }
        }
    }

    const std::vector<std::string> expected{
            "start_mod", "program_start", "unreal_init", "update", "cpp_mods_loaded", "dll_load", "uninstall_mod"};
    if (result == 0 && read_lines(log_path) != expected)
    {
        result = 6;
    }

    std::filesystem::remove_all(root);
    return result;
}
