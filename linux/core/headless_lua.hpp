#pragma once

#include "unreal_readonly.hpp"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

struct lua_State;

namespace ue4ss::linux::core
{
    enum class LuaModLifecycleAction
    {
        restart,
        uninstall,
    };

    using LuaModLifecycleHandler = std::function<bool(
            LuaModLifecycleAction,
            std::string_view,
            std::string&)>;

    class GameThreadScheduler;
    class UFunctionHookManager;
    class ObjectNotificationManager;
    class BlueprintHookManager;
    class BeginPlayHookManager;
    class EndPlayHookManager;
    class InitGameStateHookManager;
    class LoadMapHookManager;
    class ProcessConsoleExecHookManager;
    class CallFunctionByNameWithArgumentsHookManager;
    class LocalPlayerExecHookManager;
    struct LoadMapHookInvocation;
    struct ProcessConsoleExecInvocation;
    struct CallFunctionByNameWithArgumentsInvocation;
    struct LocalPlayerExecInvocation;
    struct LocalPlayerExecCallbackResult;
    struct UFunctionHookInvocation;
}

namespace ue4ss::linux::core
{
    struct LuaExecutionResult
    {
        bool success{};
        std::string detail;
    };

    int schedule_lua_callback(lua_State* state,
                              int callback_index,
                              std::chrono::milliseconds delay,
                              std::chrono::milliseconds repeat) noexcept;
    int schedule_lua_async_callback(lua_State* state,
                                    int callback_index,
                                    std::chrono::milliseconds delay,
                                    bool repeating) noexcept;
    std::uint64_t schedule_lua_action(lua_State* state,
                                      int callback_index,
                                      std::chrono::milliseconds delay,
                                      std::optional<std::int64_t> frames,
                                      bool repeating,
                                      std::uint64_t requested_handle = 0u) noexcept;
    int register_lua_ufunction_hook(lua_State* state) noexcept;
    int unregister_lua_ufunction_hook(lua_State* state) noexcept;
    int register_lua_begin_play_hook(lua_State* state, bool post) noexcept;
    int register_lua_end_play_hook(lua_State* state, bool post) noexcept;
    int register_lua_init_game_state_hook(lua_State* state, bool post) noexcept;
    int register_lua_load_map_hook(lua_State* state, bool post) noexcept;
    int register_lua_process_console_exec_hook(lua_State* state, bool post) noexcept;
    int register_lua_call_function_by_name_hook(lua_State* state, bool post) noexcept;
    int register_lua_local_player_exec_hook(lua_State* state, bool post) noexcept;
    int register_lua_console_command(lua_State* state, bool global) noexcept;
    int register_lua_custom_event(lua_State* state) noexcept;
    int unregister_lua_custom_event(lua_State* state) noexcept;
    int notify_on_new_object(lua_State* state) noexcept;

    class HeadlessLuaState
    {
      public:
        HeadlessLuaState() noexcept;
        ~HeadlessLuaState();

        HeadlessLuaState(const HeadlessLuaState&) = delete;
        HeadlessLuaState& operator=(const HeadlessLuaState&) = delete;
        HeadlessLuaState(HeadlessLuaState&&) = delete;
        HeadlessLuaState& operator=(HeadlessLuaState&&) = delete;

        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] std::string version() const;
        [[nodiscard]] LuaExecutionResult install_readonly_unreal_bindings(ReadOnlyUnrealRuntime& runtime,
                                                                          GameThreadScheduler* scheduler = nullptr,
                                                                          UFunctionHookManager* hooks = nullptr,
                                                                          ObjectNotificationManager* notifications = nullptr,
                                                                          BlueprintHookManager* blueprint_hooks = nullptr,
                                                                          BeginPlayHookManager* begin_play_hooks = nullptr,
                                                                          EndPlayHookManager* end_play_hooks = nullptr,
                                                                          InitGameStateHookManager* init_game_state_hooks = nullptr,
                                                                          LoadMapHookManager* load_map_hooks = nullptr,
                                                                          ProcessConsoleExecHookManager* process_console_exec_hooks = nullptr,
                                                                          CallFunctionByNameWithArgumentsHookManager* call_function_by_name_hooks = nullptr,
                                                                          LocalPlayerExecHookManager* local_player_exec_hooks = nullptr,
                                                                          std::filesystem::path game_executable_directory = {},
                                                                          std::filesystem::path ue4ss_root_directory = {}) noexcept;
        void invoke_ufunction_hook_callback(int reference,
                                            const UFunctionHookInvocation& invocation,
                                            bool post_callback) noexcept;
        void invoke_begin_play_hook_callback(int reference,
                                             const ReadOnlyUObjectHandle& actor) noexcept;
        void invoke_end_play_hook_callback(int reference,
                                           const ReadOnlyUObjectHandle& actor,
                                           std::int32_t& reason) noexcept;
        void invoke_init_game_state_hook_callback(
                int reference,
                const ReadOnlyUObjectHandle& game_mode) noexcept;
        [[nodiscard]] std::optional<bool> invoke_load_map_hook_callback(
                int reference,
                const LoadMapHookInvocation& invocation) noexcept;
        [[nodiscard]] std::optional<bool> invoke_process_console_exec_hook_callback(
                int reference,
                const ProcessConsoleExecInvocation& invocation) noexcept;
        [[nodiscard]] std::optional<bool> invoke_call_function_by_name_hook_callback(
                int reference,
                const CallFunctionByNameWithArgumentsInvocation& invocation) noexcept;
        [[nodiscard]] LocalPlayerExecCallbackResult invoke_local_player_exec_hook_callback(
                int reference,
                const LocalPlayerExecInvocation& invocation) noexcept;
        [[nodiscard]] std::optional<bool> invoke_console_command_callback(
                int reference,
                const ProcessConsoleExecInvocation& invocation,
                bool require_game_viewport) noexcept;
        [[nodiscard]] bool log_output_device(std::uint64_t epoch,
                                             std::uint64_t output_device,
                                             std::string_view message,
                                             std::string& error) noexcept;
        [[nodiscard]] bool is_hook_callback_scope_active(std::uint64_t epoch) const noexcept;
        [[nodiscard]] bool invoke_new_object_callback(int reference,
                                                      const ReadOnlyUObjectHandle& object) noexcept;
        void invoke_custom_event_callback(
                int reference,
                const UFunctionHookInvocation& invocation) noexcept;
        [[nodiscard]] LuaExecutionResult execute_string(std::string_view source) noexcept;
        [[nodiscard]] LuaExecutionResult execute_file(const std::filesystem::path& path) noexcept;
        [[nodiscard]] std::thread::id main_thread_id() const noexcept;
        [[nodiscard]] std::thread::id async_thread_id() const noexcept;
        [[nodiscard]] bool is_async_thread() const noexcept;
        void configure_mod_lifecycle(
                std::string mod_name,
                LuaModLifecycleHandler handler) noexcept;
        [[nodiscard]] bool request_mod_lifecycle(
                LuaModLifecycleAction action,
                std::optional<std::string_view> target_mod,
                std::string& error) noexcept;

      private:
        friend int schedule_lua_callback(lua_State*, int, std::chrono::milliseconds, std::chrono::milliseconds) noexcept;
        friend int schedule_lua_async_callback(lua_State*, int, std::chrono::milliseconds, bool) noexcept;
        friend std::uint64_t schedule_lua_action(lua_State*,
                                                 int,
                                                 std::chrono::milliseconds,
                                                 std::optional<std::int64_t>,
                                                 bool,
                                                 std::uint64_t) noexcept;
        friend int register_lua_ufunction_hook(lua_State*) noexcept;
        friend int unregister_lua_ufunction_hook(lua_State*) noexcept;
        friend int register_lua_begin_play_hook(lua_State*, bool) noexcept;
        friend int register_lua_end_play_hook(lua_State*, bool) noexcept;
        friend int register_lua_init_game_state_hook(lua_State*, bool) noexcept;
        friend int register_lua_load_map_hook(lua_State*, bool) noexcept;
        friend int register_lua_process_console_exec_hook(lua_State*, bool) noexcept;
        friend int register_lua_call_function_by_name_hook(lua_State*, bool) noexcept;
        friend int register_lua_local_player_exec_hook(lua_State*, bool) noexcept;
        friend int register_lua_console_command(lua_State*, bool) noexcept;
        friend int register_lua_custom_event(lua_State*) noexcept;
        friend int unregister_lua_custom_event(lua_State*) noexcept;
        friend int notify_on_new_object(lua_State*) noexcept;
        [[nodiscard]] std::uint64_t enqueue_lua_callback(int reference,
                                                         GameThreadScheduler& scheduler,
                                                         std::chrono::milliseconds delay,
                                                         std::optional<std::int64_t> frames,
                                                         bool repeating,
                                                         std::uint64_t requested_handle = 0u) noexcept;
        [[nodiscard]] bool invoke_lua_callback(int reference, bool repeating) noexcept;
        void release_lua_callback(int reference) noexcept;
        void drain_deferred_lua_callbacks_locked() noexcept;
        [[nodiscard]] bool start_async_worker(std::string& error) noexcept;
        void stop_async_worker() noexcept;
        void run_async_worker() noexcept;
        [[nodiscard]] bool enqueue_async_callback(int reference,
                                                  std::chrono::milliseconds delay,
                                                  bool repeating) noexcept;
        [[nodiscard]] bool invoke_async_callback(int reference, bool repeating) noexcept;

        lua_State* m_state{};
        lua_State* m_async_state{};
        int m_async_state_reference{-1};
        mutable std::recursive_mutex m_mutex;
        std::mutex m_deferred_callback_mutex;
        std::vector<int> m_deferred_callback_references;
        std::thread::id m_main_thread_id;
        struct AsyncAction
        {
            int callback_reference{-1};
            std::chrono::steady_clock::time_point due;
            std::chrono::milliseconds delay{};
            bool repeating{};
        };
        mutable std::mutex m_async_mutex;
        std::condition_variable m_async_wakeup;
        std::vector<AsyncAction> m_async_actions;
        std::thread m_async_thread;
        std::thread::id m_async_thread_id;
        bool m_async_stop_requested{};
        struct HookReferences
        {
            ReadOnlyUObjectHandle function;
            bool blueprint{};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            std::uint64_t blueprint_pre_id{};
            std::uint64_t blueprint_post_id{};
            int pre_reference{-1};
            int post_reference{-1};
        };
        std::vector<HookReferences> m_hook_references;
        std::uint64_t m_next_hook_callback_epoch{1u};
        std::vector<std::uint64_t> m_active_hook_callback_epochs;
        UFunctionHookManager* m_hook_manager{};
        BlueprintHookManager* m_blueprint_hook_manager{};
        struct CustomEventReferences
        {
            std::string event_name;
            std::uint64_t callback_id{};
            int callback_reference{-1};
        };
        std::vector<CustomEventReferences> m_custom_event_references;
        struct BeginPlayReferences
        {
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            int callback_reference{-1};
        };
        std::vector<BeginPlayReferences> m_begin_play_references;
        BeginPlayHookManager* m_begin_play_hook_manager{};
        struct NativeLifecycleReferences
        {
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            int callback_reference{-1};
        };
        std::vector<NativeLifecycleReferences> m_end_play_references;
        EndPlayHookManager* m_end_play_hook_manager{};
        std::vector<NativeLifecycleReferences> m_init_game_state_references;
        InitGameStateHookManager* m_init_game_state_hook_manager{};
        std::vector<NativeLifecycleReferences> m_load_map_references;
        LoadMapHookManager* m_load_map_hook_manager{};
        std::vector<NativeLifecycleReferences> m_process_console_exec_references;
        ProcessConsoleExecHookManager* m_process_console_exec_hook_manager{};
        std::vector<NativeLifecycleReferences> m_call_function_by_name_references;
        CallFunctionByNameWithArgumentsHookManager*
                m_call_function_by_name_hook_manager{};
        std::vector<NativeLifecycleReferences> m_local_player_exec_references;
        LocalPlayerExecHookManager* m_local_player_exec_hook_manager{};
        struct NotificationReferences
        {
            std::uint64_t id{};
            int callback_reference{-1};
        };
        std::vector<NotificationReferences> m_notification_references;
        ObjectNotificationManager* m_notification_manager{};
        GameThreadScheduler* m_scheduler{};
        std::string m_mod_name;
        LuaModLifecycleHandler m_mod_lifecycle_handler;
    };

    // Creates a throw-away VM, opens the standard libraries, and executes a
    // protected chunk.  No Lua longjmp or C++ exception crosses this boundary.
    [[nodiscard]] LuaExecutionResult probe_lua_runtime() noexcept;

    // Creates a throw-away VM and installs the read-only Unreal bridge. The
    // native object API gate has already exercised the live registry; this
    // probe deliberately avoids repeating long engine calls on the startup
    // monitor thread.
    [[nodiscard]] LuaExecutionResult probe_readonly_unreal_lua(ReadOnlyUnrealRuntime& runtime,
                                                               GameThreadScheduler* scheduler = nullptr,
                                                               UFunctionHookManager* hooks = nullptr,
                                                               ObjectNotificationManager* notifications = nullptr,
                                                               BlueprintHookManager* blueprint_hooks = nullptr,
                                                               BeginPlayHookManager* begin_play_hooks = nullptr,
                                                               EndPlayHookManager* end_play_hooks = nullptr,
                                                               InitGameStateHookManager* init_game_state_hooks = nullptr,
                                                               LoadMapHookManager* load_map_hooks = nullptr,
                                                               ProcessConsoleExecHookManager* process_console_exec_hooks = nullptr,
                                                               CallFunctionByNameWithArgumentsHookManager* call_function_by_name_hooks = nullptr,
                                                               LocalPlayerExecHookManager* local_player_exec_hooks = nullptr) noexcept;
}
