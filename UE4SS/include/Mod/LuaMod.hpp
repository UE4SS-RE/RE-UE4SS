#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <Common.hpp>
#include <File/File.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <Mod/Mod.hpp>

namespace RC
{
    class UE4SSProgram;

    namespace Unreal
    {
        class UClass;
    }

    RC_UE4SS_API auto get_mod_ref(const LuaMadeSimple::Lua& lua) -> class LuaMod*;

    class LuaMod : public Mod
    {
      private:
        std::wstring m_scripts_path;
        LuaMadeSimple::Lua& m_lua;

      public:
        std::vector<LuaMadeSimple::Lua*> m_hook_lua{};
        LuaMadeSimple::Lua* m_main_lua{};
        LuaMadeSimple::Lua* m_async_lua{};

      public:
        enum class ActionType
        {
            Immediate,
            Delayed,
            Loop
        };

        struct SimpleLuaAction
        {
            const LuaMadeSimple::Lua* lua;
            int32_t lua_action_function_ref{};
            int32_t lua_action_thread_ref{};
        };

        struct AsyncAction
        {
            // TODO: Use LuaMadeSimple instead of lua_State*
            // Not doing it now because the copy constructor gets implicitly deleted which is needed for erase & remove_if
            // lua_State* lua_state;
            int32_t lua_action_function_ref{};
            ActionType type{};
            std::chrono::time_point<std::chrono::steady_clock> created_at{};
            int64_t delay{};
        };
        std::vector<AsyncAction> m_pending_actions{};
        std::vector<AsyncAction> m_delayed_actions{};

        struct SharedLuaVariable
        {
            struct UserdataContainer
            {
                void* userdata;
            };
            int lua_type{LUA_TNIL};
            void* value{};
            bool is_integer{}; // Is true if lua_isinteger returned true when this variable was shared.
        };
        struct LuaCallbackData
        {
            struct RegistryIndex
            {
                int32_t lua_index{};
                int32_t identifier{};
            };
            const LuaMadeSimple::Lua& lua;
            Unreal::UClass* instance_of_class;
            std::vector<std::pair<const LuaMadeSimple::Lua*, RegistryIndex>> registry_indexes;
        };
        struct LuaCancellableCallbackData
        {
            const LuaMadeSimple::Lua* lua;
            Unreal::UClass* instance_of_class;
            int32_t lua_callback_function_ref{};
            int32_t lua_callback_thread_ref{};
        };
        static inline std::vector<LuaCancellableCallbackData> m_static_construct_object_lua_callbacks;
        static inline std::vector<LuaCallbackData> m_process_console_exec_pre_callbacks;
        static inline std::vector<LuaCallbackData> m_process_console_exec_post_callbacks;
        static inline std::vector<LuaCallbackData> m_call_function_by_name_with_arguments_pre_callbacks;
        static inline std::vector<LuaCallbackData> m_call_function_by_name_with_arguments_post_callbacks;
        static inline std::vector<LuaCallbackData> m_local_player_exec_pre_callbacks;
        static inline std::vector<LuaCallbackData> m_local_player_exec_post_callbacks;
        static inline std::unordered_map<File::StringType, LuaCallbackData> m_global_command_lua_callbacks;
        static inline std::unordered_map<File::StringType, LuaCallbackData> m_custom_command_lua_pre_callbacks;
        static inline std::vector<SimpleLuaAction> m_game_thread_actions{};
        // This is storage that persists through hot-reloads.
        static inline std::unordered_map<std::string, SharedLuaVariable> m_shared_lua_variables{};
        static inline std::unordered_map<StringType, LuaCallbackData> m_custom_event_callbacks{};
        static inline std::vector<LuaCallbackData> m_load_map_pre_callbacks{};
        static inline std::vector<LuaCallbackData> m_load_map_post_callbacks{};
        static inline std::vector<LuaCallbackData> m_init_game_state_pre_callbacks{};
        static inline std::vector<LuaCallbackData> m_init_game_state_post_callbacks{};
        static inline std::vector<LuaCallbackData> m_begin_play_pre_callbacks{};
        static inline std::vector<LuaCallbackData> m_begin_play_post_callbacks{};
        static inline std::unordered_map<StringType, LuaCallbackData> m_script_hook_callbacks{};
        static inline std::unordered_map<int32_t, int32_t> m_generic_hook_id_to_native_hook_id{};
        // Generic hook ids are generated incrementally so the first one is 0 and the next one is always +1 from the last id.
        static inline int32_t m_last_generic_hook_id{};
        static inline bool m_is_currently_executing_game_action{};
        static inline std::recursive_mutex m_thread_actions_mutex{};

      private:
        std::jthread m_async_thread;
        bool m_processing_events{};
        bool m_pause_events_processing{};
        bool m_is_process_event_hooked{};
        std::mutex m_actions_lock{};

      public:
        LuaMod(UE4SSProgram&, std::wstring&& mod_name, std::wstring&& mod_path);
        ~LuaMod() override = default;

      private:
        auto start_async_thread() -> void
        {
            m_async_thread = std::jthread{&Mod::update_async, this};
        }

      private:
        auto setup_lua_require_paths(const LuaMadeSimple::Lua& lua) const -> void;
        auto setup_lua_global_functions(const LuaMadeSimple::Lua& lua) const -> void;
        auto setup_lua_global_functions_main_state_only() const -> void;
        auto setup_lua_classes(const LuaMadeSimple::Lua& lua) const -> void;
        auto fire_on_lua_start_for_cpp_mods() -> void;
        auto fire_on_lua_stop_for_cpp_mods() -> void;

      public:
        auto start_mod() -> void override;
        auto uninstall() -> void override;

        auto prepare_mod(const LuaMadeSimple::Lua& lua) -> void;

        RC_UE4SS_API auto lua() const -> const LuaMadeSimple::Lua&;
        RC_UE4SS_API auto main_lua() const -> const LuaMadeSimple::Lua*;
        RC_UE4SS_API auto async_lua() const -> const LuaMadeSimple::Lua*;
        RC_UE4SS_API auto get_lua_state() const -> lua_State*;

        RC_UE4SS_API auto actions_lock() -> void
        {
            m_actions_lock.lock();
        }
        RC_UE4SS_API auto actions_unlock() -> void
        {
            m_actions_lock.unlock();
        }

      public:
        // Called once when the program is starting, after mods are setup but before any mods have been started
        auto static on_program_start() -> void;

        auto static global_uninstall() -> void;

        // Async update
        // Used when the main update function would block other mods from executing their scripts
        auto update_async() -> void override;

        auto process_delayed_actions() -> void;
        auto clear_delayed_actions() -> void;
    };

    struct LuaStatics
    {
        // Lua instance connected to the in-game console.
        static LuaMadeSimple::Lua* console_executor;
        static bool console_executor_enabled;
    };
} // namespace RC
