#pragma once

#include <vector>
#include <string>
#include <filesystem>

namespace RC::GUI
{
    enum class ModType
    {
        Invalid,
        LuaMod,
        CppMod,
    };

    // Mod management
    struct ModInfo
    {
        std::string name{};
        std::filesystem::path path{};
        bool enabled_via_txt{false};
        bool enabled_via_mods_txt{false};
        bool is_running{false};
        ModType mod_type{ModType::Invalid};

        bool is_enabled() const { return enabled_via_txt || enabled_via_mods_txt; }
    };

    class ModsWidget
    {
    public:
        bool m_mods_list_dirty{true};
        std::vector<ModInfo> m_discovered_mods{};
        bool m_show_create_mod_popup{false};
        std::string m_new_mod_name{};
        bool m_show_create_file_popup{false};
        std::string m_new_file_name{};
        bool m_add_require_to_main{true};
        std::filesystem::path m_create_file_mod_path{};

    public:
        auto render() -> void;

    private:
        auto refresh_mods_list() -> void;
        auto set_mod_enabled(const std::filesystem::path& mod_path, bool enabled) -> void;
        auto uninstall_mod_by_name(const std::string& mod_name, ModType mod_type) -> void;
        auto restart_mod_by_name(const std::string& mod_name, ModType mod_type) -> void;
        auto start_mod_by_path(const std::filesystem::path& mod_path, ModType mod_type) -> void;
        auto create_new_mod(const std::string& name) -> bool;
        //auto create_new_file(const std::string& mod_path, const std::string& filename, bool add_require_to_main) -> bool;
    };
}
