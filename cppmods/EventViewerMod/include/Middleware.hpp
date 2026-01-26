#pragma once

// EventViewerMod: Capture backend (UE hook installation + queueing).
//
// Hooks multiple Unreal call sites and enqueues CallStackEntry objects into a lock-free
// moodycamel::ConcurrentQueue. The queue is drained on the ImGui thread by Client.
//
// Design constraints:
// - Hooks are extremely hot (ProcessEvent/ProcessInternal/ProcessLocalScriptFunction), so this code
//   avoids allocations where possible and uses thread_local producer tokens.
// - Depth is unified across the hooked functions so nested / recursive call flows keep consistent
//   indentation regardless of which hook produced a given entry.

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <Enums.hpp>
#include <Structs.hpp>

#include <Unreal/Hooks/Hooks.hpp>
#include <concurrentqueue.h>

namespace RC::EventViewerMod
{
    class Middleware
    {
      public:
        // By-reference singleton.
        static auto GetInstance() -> Middleware&;

        // [Thread-Any] Enqueues info on a call. The hook_target is captured at enqueue-time.
        auto enqueue(EMiddlewareHookTarget hook_target, RC::Unreal::UObject* context, RC::Unreal::UFunction* function) -> void;

        // [Thread-ImGui] Dequeues call info.
        //  max_ms - maximum wall time (ms) to spend dequeuing.
        //  max_count_per_iteration - max items pulled per bulk-dequeue.
        //  on_dequeue - receives the dequeued entry by rvalue-ref (move).
        auto dequeue(uint16_t max_ms, uint16_t max_count_per_iteration, const std::function<void(CallStackEntry&&)>& on_dequeue) -> void;

        // [Thread-ImGui] Pauses stream, removes hooks. Also drains queue.
        auto stop() -> bool;

        // [Thread-ImGui]
        [[nodiscard]] auto is_paused() const -> bool;

        // [Thread-ImGui] Resumes stream by installing hooks.
        auto start() -> bool;

        // [Thread-ImGui]
        auto set_imgui_thread_id(std::thread::id id) -> void;

        // [Thread-ImGui]
        [[nodiscard]] auto get_imgui_thread_id() const -> std::thread::id;

        // [Thread-ImGui]
        [[nodiscard]] auto get_average_enqueue_time() const -> double;

        // [Thread-ImGui]
        [[nodiscard]] auto get_average_dequeue_time() const -> double;

        ~Middleware();

      private:
        Middleware();

        auto assert_on_imgui_thread() const -> void;
        [[nodiscard]] auto is_tick_fn(const RC::Unreal::UFunction* fn) const -> bool;
        auto stop_impl(bool do_assert) -> bool;

        template <typename CallbackType>
        struct HookController
        {
            Unreal::Hook::GlobalCallbackId (*register_prehook_fn)(CallbackType, Unreal::Hook::FCallbackOptions){};
            Unreal::Hook::GlobalCallbackId (*register_posthook_fn)(CallbackType, Unreal::Hook::FCallbackOptions){};
            CallbackType m_pre_callback{};
            CallbackType m_post_callback{};
            Unreal::Hook::GlobalCallbackId m_prehook_id = Unreal::Hook::ERROR_ID;
            Unreal::Hook::GlobalCallbackId m_posthook_id = Unreal::Hook::ERROR_ID;

            auto install_prehook() -> bool;
            auto install_posthook() -> bool;
            auto unhook() -> void;
            auto is_hooked() const -> bool;

            inline static Unreal::Hook::FCallbackOptions m_cb_options{false, true, STR("EventViewer"), STR("CallStackMonitor")};
        };

      private:
        HookController<Unreal::Hook::ProcessEventCallbackWithData> m_pe_controller{};
        HookController<Unreal::Hook::ProcessInternalCallbackWithData> m_pi_controller{};
        HookController<Unreal::Hook::ProcessLocalScriptFunctionCallbackWithData> m_plsf_controller{};

        bool m_paused = true;

        std::thread::id m_imgui_id{};

        // Queue
        moodycamel::ConcurrentQueue<CallStackEntry> m_queue{};
        moodycamel::ConsumerToken m_imgui_consumer_token{m_queue};
        std::vector<CallStackEntry> m_buffer{};

        // Detected tick functions
        std::unordered_set<RC::Unreal::UObject*> m_tick_fns{};

        inline static thread_local uint32_t m_depth = 0;
        std::atomic_uint64_t m_depth_reset_counter = 0;

        std::atomic_flag m_allow_queue;
    };

    template <typename CallbackType>
    auto Middleware::HookController<CallbackType>::install_prehook() -> bool
    {
        if (m_prehook_id != RC::Unreal::Hook::ERROR_ID)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to install prehook because it's already installed!"));
            return false;
        }

        if (!register_prehook_fn)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to install prehook because register function is unknown! Is FName.toString known?"));
            return false;
        }

        m_prehook_id = register_prehook_fn(m_pre_callback, m_cb_options);

        if (m_prehook_id == RC::Unreal::Hook::ERROR_ID)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to install prehook!"));
            return false;
        }

        return true;
    }

    template <typename CallbackType>
    auto Middleware::HookController<CallbackType>::install_posthook() -> bool
    {
        if (m_posthook_id != RC::Unreal::Hook::ERROR_ID)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to install posthook because it's already installed!"));
            return false;
        }

        if (!register_posthook_fn)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to install posthook because register function is unknown! Is FName.toString known?"));
            return false;
        }

        m_posthook_id = register_posthook_fn(m_post_callback, m_cb_options);

        if (m_posthook_id == RC::Unreal::Hook::ERROR_ID)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to install posthook!"));
            return false;
        }

        return true;
    }

    template <typename CallbackType>
    auto Middleware::HookController<CallbackType>::unhook() -> void
    {
        if (m_prehook_id == RC::Unreal::Hook::ERROR_ID && m_posthook_id == RC::Unreal::Hook::ERROR_ID)
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to remove hooks because it's not active!"));
            return;
        }

        if (!RC::Unreal::Hook::UnregisterCallback(m_prehook_id))
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to unregister prehook!"));
        }

        if (!RC::Unreal::Hook::UnregisterCallback(m_posthook_id))
        {
            Output::send<LogLevel::Error>(STR("[EventViewerMod] Failed to unregister posthook!"));
        }

        m_prehook_id = m_posthook_id = RC::Unreal::Hook::ERROR_ID;
    }

    template <typename CallbackType>
    auto Middleware::HookController<CallbackType>::is_hooked() const -> bool
    {
        return (m_prehook_id != RC::Unreal::Hook::ERROR_ID && m_posthook_id != RC::Unreal::Hook::ERROR_ID);
    }
} // namespace RC::EventViewerMod
