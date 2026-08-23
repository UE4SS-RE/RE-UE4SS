#pragma once

#include "ue4ss/inline_hook.hpp"
#include "unreal_readonly.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ue4ss::linux::core
{
    [[nodiscard]] std::vector<std::string> tokenize_console_command(
            std::string_view command);

    // UE 5.1 Linux implements UObject::ProcessConsoleExec as a small SysV
    // tail thunk which clears the fifth argument (r8d) before jumping to
    // UObject::CallFunctionByNameWithArguments.  Resolve that target only
    // when the complete instruction shape and executable-segment gates pass.
    [[nodiscard]] std::optional<std::uint64_t>
    resolve_call_function_by_name_with_arguments_thunk(
            std::uint64_t process_console_exec,
            std::string& error) noexcept;

    struct ProcessConsoleExecInvocation
    {
        ReadOnlyUObjectHandle context;
        std::string command;
        std::vector<std::string> command_parts;
        std::uint64_t output_device{};
        ReadOnlyUObjectHandle executor;
    };

    // Hooks UObject::ProcessConsoleExec using the UE 5.1 Linux Itanium vtable.
    // FOutputDevice and TCHAR storage remain engine-owned and are only exposed
    // to synchronous callbacks; no pointer from an invocation is retained by
    // the manager after dispatch returns.
    class ProcessConsoleExecHookManager
    {
      public:
        using Callback =
                std::function<std::optional<bool>(const ProcessConsoleExecInvocation&)>;

        ProcessConsoleExecHookManager() = default;
        ~ProcessConsoleExecHookManager();

        ProcessConsoleExecHookManager(const ProcessConsoleExecHookManager&) = delete;
        ProcessConsoleExecHookManager& operator=(const ProcessConsoleExecHookManager&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept;
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] std::uint64_t target_address() const noexcept;
        [[nodiscard]] std::uint64_t call_function_by_name_target() const noexcept;
        [[nodiscard]] const std::vector<std::uint64_t>& direct_call_targets() const noexcept;

        [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> register_hook(
                Callback pre,
                Callback post,
                std::string& error) noexcept;
        [[nodiscard]] bool unregister_hook(std::uint64_t pre_id,
                                           std::uint64_t post_id,
                                           std::string& error) noexcept;

        [[nodiscard]] bool log(std::uint64_t output_device,
                               std::string_view message,
                               std::string& error) const noexcept;

      private:
        using ProcessConsoleExecFunction = bool (*)(void*, const char16_t*, void*, void*);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static bool detour(void* context,
                           const char16_t* command,
                           void* output_device,
                           void* executor) noexcept;
        bool dispatch(void* context,
                      const char16_t* command,
                      void* output_device,
                      void* executor) noexcept;
        [[nodiscard]] bool snapshot_invocation(void* context,
                                               const char16_t* command,
                                               void* output_device,
                                               void* executor,
                                               ProcessConsoleExecInvocation& output,
                                               std::string& error) const noexcept;

        static std::atomic<ProcessConsoleExecHookManager*> s_active;
        static std::atomic<ProcessConsoleExecFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<ProcessConsoleExecFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::uint64_t m_target{};
        std::uint64_t m_call_function_by_name_target{};
        std::vector<std::uint64_t> m_direct_call_targets;
        std::mutex m_callbacks_mutex;
        std::vector<std::shared_ptr<CallbackRecord>> m_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };

    struct CallFunctionByNameWithArgumentsInvocation
    {
        ReadOnlyUObjectHandle context;
        std::string command;
        std::uint64_t output_device{};
        ReadOnlyUObjectHandle executor;
        bool force_call_with_non_exec{};
    };

    struct LocalPlayerExecInspection
    {
        bool success{};
        ReadOnlyUObjectHandle class_default_object;
        std::uint64_t secondary_vtable{};
        std::int64_t offset_to_top{};
        std::uint64_t type_info{};
        std::uint64_t secondary_base_offset{};
        std::vector<std::uint64_t> executable_slots;
        std::uint64_t exec_adjustor_thunk{};
        std::uint64_t exec_target{};
        std::string detail;
    };

    [[nodiscard]] LocalPlayerExecInspection inspect_local_player_exec_abi(
            ReadOnlyUnrealRuntime& runtime) noexcept;

    // Hooks the native five-argument implementation reached through the
    // validated ProcessConsoleExec SysV tail thunk. FOutputDevice and TCHAR
    // arguments remain engine-owned and are valid only during dispatch.
    class CallFunctionByNameWithArgumentsHookManager
    {
      public:
        using Callback = std::function<std::optional<bool>(
                const CallFunctionByNameWithArgumentsInvocation&)>;

        CallFunctionByNameWithArgumentsHookManager() = default;
        ~CallFunctionByNameWithArgumentsHookManager();

        CallFunctionByNameWithArgumentsHookManager(
                const CallFunctionByNameWithArgumentsHookManager&) = delete;
        CallFunctionByNameWithArgumentsHookManager& operator=(
                const CallFunctionByNameWithArgumentsHookManager&) = delete;

        [[nodiscard]] bool start(
                ReadOnlyUnrealRuntime& runtime,
                const ProcessConsoleExecHookManager& process_console_exec,
                std::string& error) noexcept;
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] std::uint64_t target_address() const noexcept;

        [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> register_hook(
                Callback pre,
                Callback post,
                std::string& error) noexcept;
        [[nodiscard]] bool unregister_hook(std::uint64_t pre_id,
                                           std::uint64_t post_id,
                                           std::string& error) noexcept;

      private:
        using TargetFunction = bool (*)(void*, const char16_t*, void*, void*, bool);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static bool detour(void* context,
                           const char16_t* command,
                           void* output_device,
                           void* executor,
                           bool force_call_with_non_exec) noexcept;
        bool dispatch(void* context,
                      const char16_t* command,
                      void* output_device,
                      void* executor,
                      bool force_call_with_non_exec) noexcept;
        [[nodiscard]] bool snapshot_invocation(
                void* context,
                const char16_t* command,
                void* output_device,
                void* executor,
                bool force_call_with_non_exec,
                CallFunctionByNameWithArgumentsInvocation& output,
                std::string& error) const noexcept;

        static std::atomic<CallFunctionByNameWithArgumentsHookManager*> s_active;
        static std::atomic<TargetFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<TargetFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::uint64_t m_target{};
        std::mutex m_callbacks_mutex;
        std::vector<std::shared_ptr<CallbackRecord>> m_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };

    struct LocalPlayerExecInvocation
    {
        ReadOnlyUObjectHandle context;
        ReadOnlyUObjectHandle world;
        std::string command;
        std::uint64_t output_device{};
    };

    struct LocalPlayerExecCallbackResult
    {
        std::optional<bool> return_override;
        bool execute_original{true};
    };

    class LocalPlayerExecHookManager
    {
      public:
        using Callback = std::function<LocalPlayerExecCallbackResult(
                const LocalPlayerExecInvocation&)>;

        LocalPlayerExecHookManager() = default;
        ~LocalPlayerExecHookManager();

        LocalPlayerExecHookManager(const LocalPlayerExecHookManager&) = delete;
        LocalPlayerExecHookManager& operator=(const LocalPlayerExecHookManager&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime,
                                 std::string& error) noexcept;
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] std::uint64_t target_address() const noexcept;
        [[nodiscard]] std::uint64_t secondary_base_offset() const noexcept;

        [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> register_hook(
                Callback pre,
                Callback post,
                std::string& error) noexcept;
        [[nodiscard]] bool unregister_hook(std::uint64_t pre_id,
                                           std::uint64_t post_id,
                                           std::string& error) noexcept;

      private:
        using TargetFunction = bool (*)(void*, void*, const char16_t*, void*);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static bool detour(void* context,
                           void* world,
                           const char16_t* command,
                           void* output_device) noexcept;
        bool dispatch(void* context,
                      void* world,
                      const char16_t* command,
                      void* output_device) noexcept;
        [[nodiscard]] bool snapshot_invocation(
                void* context,
                void* world,
                const char16_t* command,
                void* output_device,
                LocalPlayerExecInvocation& output,
                std::string& error) const noexcept;

        static std::atomic<LocalPlayerExecHookManager*> s_active;
        static std::atomic<TargetFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<TargetFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::uint64_t m_target{};
        std::uint64_t m_secondary_base_offset{};
        std::mutex m_callbacks_mutex;
        std::vector<std::shared_ptr<CallbackRecord>> m_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };
} // namespace ue4ss::linux::core
