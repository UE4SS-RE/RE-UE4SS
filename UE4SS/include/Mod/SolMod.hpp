#pragma once

#include <memory>
#include <mutex>
#include <thread>

#include <Mod/SolHelpers.hpp>
#include <Mod/Mod.hpp>

namespace sol
{
    class state;
}

namespace RC
{
    struct LuaCallbackData
    {
        struct RegistryIndex
        {
            sol::function lua_callback{};
            int32_t identifier{};
        };
        lua_State* lua;
        Unreal::UClass* instance_of_class;
        std::vector<RegistryIndex> registry_indexes;
    };

    class SolMod : public Mod
    {
    private:
        StringType m_scripts_path{};
        sol::state m_sol_state_ue4ss{};
        std::vector<sol::thread> m_sol_gameplay_states{};
        sol::thread m_sol_state_async{};
        sol::reference m_sol_register_hook_callback_ref{};
        std::mutex m_actions_lock{};
        std::jthread m_async_thread{};

    public:
        enum class ActionType { Immediate, Delayed, Loop };
        struct AsyncAction
        {
            // TODO: Use LuaMadeSimple instead of lua_State*
            // Not doing it now because the copy constructor gets implicitly deleted which is needed for erase & remove_if
            // lua_State* lua_state;
            sol::function lua_action_function{};
            ActionType type{};
            std::chrono::time_point<std::chrono::steady_clock> created_at{};
            int64_t delay{};
        };
        std::vector<AsyncAction> m_pending_actions{};
        std::vector<AsyncAction> m_delayed_actions{};
        
    public:
        static std::recursive_mutex m_thread_actions_mutex;
        static std::vector<std::unique_ptr<LuaUnrealScriptFunctionData>> m_hooked_script_function_data;
        static std::vector<LuaCallbackData> m_static_construct_object_lua_callbacks;
        static std::unordered_map<StringType, LuaCallbackData> m_custom_event_callbacks;
        static std::vector<LuaCallbackData> m_init_game_state_pre_callbacks;
        static std::vector<LuaCallbackData> m_init_game_state_post_callbacks;
        static std::unordered_map<StringType, LuaCallbackData> m_script_hook_callbacks;
        static std::unordered_map<int32_t, int32_t> m_generic_hook_id_to_native_hook_id;
        static int32_t m_last_generic_hook_id;

    public:
        SolMod(UE4SSProgram&, StringType&& mod_name, StringType&& mod_path);
        ~SolMod() override;

    private:
        auto state_init(sol::state_view sol) -> void;
        auto setup_lua_global_functions(sol::state_view) -> void;
        auto start_async_thread() -> void;
        auto clear_delayed_actions() -> void;

    public:
        enum class State { UE4SS, Async };
        auto sol(State = State::UE4SS) -> sol::state_view;
        auto process_delayed_actions() -> void;

    public:
        auto start_mod() -> void override;
        auto uninstall() -> void override;
        auto update_async() -> void override;

    public:
        static auto on_program_start() -> void;
        static auto global_uninstall() -> void;
    };
}
