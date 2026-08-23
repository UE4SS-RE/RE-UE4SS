#pragma once

#include "ue4ss/inline_hook.hpp"
#include "ufunction_hooks.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace ue4ss::linux::core
{
    struct BlueprintExecutionAddresses
    {
        std::uint64_t process_internal{};
        std::uint64_t process_local_script_function{};
        std::string detail;
    };

    // Hooks UE5.1 Blueprint-to-Blueprint calls which bypass an individual
    // UFunction::Func slot and enter ProcessLocalScriptFunction directly.
    class BlueprintHookManager
    {
      public:
        using Callback = UFunctionHookManager::Callback;

        BlueprintHookManager() = default;
        ~BlueprintHookManager();

        BlueprintHookManager(const BlueprintHookManager&) = delete;
        BlueprintHookManager& operator=(const BlueprintHookManager&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept;
#if defined(UE4SS_LINUX_TESTING)
        [[nodiscard]] bool start_for_testing(ReadOnlyUnrealRuntime& runtime,
                                             BlueprintExecutionAddresses addresses,
                                             std::string& error) noexcept;
#endif
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] const BlueprintExecutionAddresses& addresses() const noexcept;
        [[nodiscard]] bool is_script_function(const ReadOnlyUObjectHandle& function) const noexcept;
        [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> register_hook(
                const ReadOnlyUObjectHandle& function,
                Callback pre,
                Callback post,
                std::string& error) noexcept;
        [[nodiscard]] bool unregister_hook(const ReadOnlyUObjectHandle& function,
                                           std::uint64_t pre_id,
                                           std::uint64_t post_id,
                                           std::string& error) noexcept;
        [[nodiscard]] std::uint64_t register_named_hook(std::string event_name,
                                                        Callback callback,
                                                        std::string& error) noexcept;
        [[nodiscard]] bool unregister_named_hook(std::uint64_t callback_id,
                                                 std::string& error) noexcept;

      private:
        using ScriptFunction = void (*)(void*, void*, void*);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        struct FunctionRecord
        {
            ReadOnlyUObjectHandle function;
            std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        };

        struct NamedCallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t id{};
            std::string event_name;
            Callback callback;
        };

        static void detour(void* context, void* stack, void* result) noexcept;
        void dispatch(void* context, void* stack, void* result) noexcept;
        [[nodiscard]] bool ensure_hook_installed(std::string& error) noexcept;

        static std::atomic<BlueprintHookManager*> s_active;
        static std::atomic<ScriptFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        BlueprintExecutionAddresses m_addresses;
        ue4ss::linux::InlineHook m_hook;
        std::atomic<ScriptFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::mutex m_records_mutex;
        std::vector<std::unique_ptr<FunctionRecord>> m_records;
        std::vector<std::shared_ptr<NamedCallbackRecord>> m_named_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };
} // namespace ue4ss::linux::core
