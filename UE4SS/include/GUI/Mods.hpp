#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <functional>

namespace RC::GUI
{
    enum class ModType
    {
        Invalid,
        LuaMod,
        CppMod,
        BPMod,
    };

    // Mod management
    struct BPModInfo
    {
        class ModActor* actor{};
        std::string author{};
        std::string description{};
        std::string version{};
        std::vector<std::string> buttons{};
    };
    struct ModInfo
    {
        std::string name{};
        std::filesystem::path path{};
        bool enabled_via_txt{false};
        bool enabled_via_mods_txt{false};
        bool is_running{false};
        ModType mod_type{ModType::Invalid};
        // Invalid unless mod_type == ModType::BPMod.
        BPModInfo bp_info{};

        bool is_enabled() const { return enabled_via_txt || enabled_via_mods_txt || mod_type == ModType::BPMod; }
    };

    class ModsWidget
    {
    public:
        std::atomic_bool m_mods_list_dirty{true};
        std::vector<ModInfo> m_discovered_mods{};
        bool m_show_create_mod_popup{false};
        std::string m_new_mod_name{};
        bool m_show_create_file_popup{false};
        std::string m_new_file_name{};
        bool m_add_require_to_main{true};
        std::filesystem::path m_create_file_mod_path{};
        size_t m_expanded_mod{s_invalid_mod_id};

        // Handlers.
        void* m_handler_context{};
        std::function<bool(void* context, const std::string& name)> m_create_new_mod_button_handler{};
        std::function<bool(void* context, const std::string& mod_path, const std::string& filename, bool add_require_to_main)> m_create_file_button_handler{};
        std::function<void(void* context, ModInfo& mod)> m_open_button_handler{};

    public:
        static constexpr auto s_invalid_mod_id = std::numeric_limits<decltype(m_expanded_mod)>::max();

    public:
        auto render() -> void;

    private:
        auto refresh_mods_list() -> void;
        auto set_mod_enabled(const std::filesystem::path& mod_path, bool enabled) -> void;
        auto uninstall_mod_by_name(const std::string& mod_name, ModType mod_type) -> void;
        auto restart_mod_by_name(const std::string& mod_name, ModType mod_type) -> void;
        auto start_mod_by_path(const std::filesystem::path& mod_path, ModType mod_type) -> void;
    };
}
