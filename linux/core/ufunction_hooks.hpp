#pragma once

#include "ue4ss/inline_hook.hpp"
#include "unreal_readonly.hpp"

#include <atomic>
#include <array>
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
    struct UFunctionHookInvocation
    {
        ReadOnlyUObjectHandle context;
        ReadOnlyUObjectHandle function;
        std::uint64_t stack_address{};
        std::uint64_t result_address{};
    };

    // Hooks individual UFunction::Func pointer slots. Installation is an
    // aligned pointer replacement and never rewrites executable Unreal code.
    class UFunctionHookManager
    {
      public:
        using Callback = std::function<void(const UFunctionHookInvocation&)>;

        UFunctionHookManager() = default;
        ~UFunctionHookManager();

        UFunctionHookManager(const UFunctionHookManager&) = delete;
        UFunctionHookManager& operator=(const UFunctionHookManager&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept;
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> register_hook(
                const ReadOnlyUObjectHandle& function,
                Callback pre,
                Callback post,
                std::string& error) noexcept;
        [[nodiscard]] bool unregister_hook(const ReadOnlyUObjectHandle& function,
                                           std::uint64_t pre_id,
                                           std::uint64_t post_id,
                                           std::string& error) noexcept;
        [[nodiscard]] std::size_t hooked_function_count() const noexcept;

      private:
        using NativeFunction = void (*)(void*, void*, void*);
        static constexpr std::size_t k_max_hooked_functions = 256u;

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
            NativeFunction original{};
            std::size_t slot_index{};
            PointerHook hook;
            std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        };

        struct DispatchSnapshot
        {
            ReadOnlyUObjectHandle function;
            NativeFunction original{};
            std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        };

        template <std::size_t SlotIndex>
        static void detour_slot(void* context, void* stack, void* result) noexcept;
        [[nodiscard]] static const std::array<NativeFunction, k_max_hooked_functions>& detour_table() noexcept;
        void dispatch(std::size_t slot_index, void* context, void* stack, void* result) noexcept;

        static std::atomic<UFunctionHookManager*> s_active;
        static std::mutex s_dispatch_mutex;
        static std::array<std::atomic<NativeFunction>, k_max_hooked_functions> s_originals;

        ReadOnlyUnrealRuntime* m_runtime{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        mutable std::mutex m_records_mutex;
        std::vector<std::unique_ptr<FunctionRecord>> m_records;
        mutable std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };
}
