#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <UE4SSProgram.hpp>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / ("ue4ss-lua-mod-discovery-" + std::to_string(suffix));
    const auto scripts_directory = root / "Mods" / "LuaProbeMod" / "Scripts";

    std::filesystem::create_directories(scripts_directory);
    std::filesystem::copy_file(argv[1], root / "UE4SS-settings.ini", std::filesystem::copy_options::overwrite_existing);
    std::ofstream{scripts_directory / "main.lua"} << "print('probe')\n";

    int result{};
    {
        RC::UE4SSProgram program{root / "libUE4SS.so", {}};
        if (!RC::UE4SSProgram::find_lua_mod_by_name(STR("LuaProbeMod")))
        {
            result = 3;
        }
    }

    std::filesystem::remove_all(root);
    return result;
}
