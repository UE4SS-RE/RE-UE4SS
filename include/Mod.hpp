#ifndef UE4SS_REWRITTEN_MOD_HPP
#define UE4SS_REWRITTEN_MOD_HPP

#include <string>
#include <chrono>
#include <thread>

#include <File/File.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>

namespace RC
{
    class UE4SSProgram;

    namespace Unreal
    {
        class UClass;
    }

    class Mod
    {
    public:
        UE4SSProgram& m_program;

    private:
        std::wstring m_mod_name;
        std::wstring m_mod_path;
        std::wstring m_scripts_path;
        LuaMadeSimple::Lua& m_lua;

    public:
        LuaMadeSimple::Lua* m_hook_lua{};
        LuaMadeSimple::Lua* m_main_lua{};

    private:
        // Whether the mod can be installed
        // This is true by default and is only false if the state of the mod won't allow for a successful installation
        bool m_installable{true};
        bool m_installed{false};
        mutable bool m_is_started{false};

    public:
        enum class IsTrueMod { Yes, No };
 
        enum class ActionType { Immediate, Delayed, Loop };

        struct AsyncAction
        {
            // TODO: Use LuaMadeSimple instead of lua_State*
            // Not doing it now because the copy constructor gets implicitly deleted which is needed for erase & remove_if
            lua_State* lua_state;
            int32_t lua_action_function_ref;
            ActionType type;
            std::chrono::time_point<std::chrono::steady_clock> created_at;
            int64_t delay;
        };
        struct DelayedAction : public AsyncAction
        {
            std::chrono::time_point<std::chrono::steady_clock> created_at;
            int64_t delay;
        };
        static inline std::vector<AsyncAction> m_pending_actions{};
        static inline std::vector<AsyncAction> m_async_actions{};
        static inline std::vector<DelayedAction> m_delayed_actions{};
        static inline std::vector<std::jthread> m_async_loop_threads{};

        struct LuaCallbackData
        {
            const LuaMadeSimple::Lua& lua;
            Unreal::UClass* instance_of_class;
            std::vector<int32_t> registry_indexes;
        };
        static inline std::vector<LuaCallbackData> m_static_construct_object_lua_callbacks;
        static inline std::unordered_map<File::StringType, LuaCallbackData> m_global_command_lua_callbacks;
        static inline std::unordered_map<File::StringType, LuaCallbackData> m_custom_command_lua_pre_callbacks;

    protected:
        static inline std::jthread m_async_thread;
        static inline bool m_processing_events{};
        static inline bool m_pause_events_processing{};

    public:
        Mod(UE4SSProgram&, std::wstring&& mod_name, std::wstring&& mod_path);
        ~Mod() = default;

    private:
        auto setup_lua_require_paths(const LuaMadeSimple::Lua& lua) const -> void;
        auto setup_lua_global_functions(const LuaMadeSimple::Lua& lua) const -> void;
        auto setup_lua_global_functions_main_state_only() const -> void;
        auto setup_lua_classes(const LuaMadeSimple::Lua& lua) const -> void;

    public:
        auto get_name()  const -> std::wstring_view;

        auto set_installable(bool) -> void;
        auto is_installable() const -> bool;
        auto set_installed(bool) -> void;
        auto is_installed() const -> bool;
        auto prepare_mod(const LuaMadeSimple::Lua& lua) -> void;
        auto start_mod() -> void;
        auto is_started() const -> bool;
        auto uninstall() const -> void;

        auto lua() const -> const LuaMadeSimple::Lua&;
        auto main_lua() const -> const LuaMadeSimple::Lua*;

    public:
        // Called once when the program is starting, after mods are setup but before any mods have been started
        auto static on_program_start() -> void;

        // Main update from the program
        auto static update() -> void;

        // Async update
        // Used when the main update function would block other mods from executing their scripts
        auto static update_async() -> void;

        auto static process_async_actions() -> void;
        auto static process_delayed_actions() -> void;
        auto static process_async_loops(std::stop_token, const LuaMadeSimple::Lua&, int32_t lua_function_ref, std::chrono::time_point<std::chrono::steady_clock> created_at, int64_t delay) -> void;
        auto static clear_delayed_actions() -> void;
        auto static clear_async_loop_threads() -> void;
    };

    struct LuaStatics
    {
        // Lua instance connected to the in-game console.
        static LuaMadeSimple::Lua* console_executor;
        static bool console_executor_enabled;
    };
}


#endif //UE4SS_REWRITTEN_MOD_HPP
