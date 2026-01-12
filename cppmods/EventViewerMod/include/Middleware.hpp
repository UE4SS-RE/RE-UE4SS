#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <Enums.h>
#include <Structs.h>

#include <Unreal/Hooks/Hooks.hpp>
#include <concurrentqueue.h>

namespace RC::EventViewerMod
{
    // Non-virtual, single-scheme middleware (ConcurrentQueue).
    // Lives for the lifetime of the mod.
    class Middleware
    {
    public:
        // By-reference singleton.
        static auto GetInstance() -> Middleware&;

        // [Thread-ImGui] Sets the EMiddlewareHookTarget to use. Does not automatically start the stream.
        auto set_hook_target(EMiddlewareHookTarget target) -> void;

        // [Thread-ImGui]
        [[nodiscard]] auto get_hook_target() const -> EMiddlewareHookTarget;

        // [Thread-Any] Enqueues info on a call. The hook_target is captured at enqueue-time.
        auto enqueue(EMiddlewareHookTarget hook_target,
                     RC::Unreal::UObject* context,
                     RC::Unreal::UFunction* function,
                     uint32_t depth,
                     std::thread::id thread_id,
                     bool is_tick) -> void;

        // [Thread-ImGui] Dequeues call info.
        //  max_ms - maximum wall time (ms) to spend dequeuing.
        //  max_count_per_iteration - max items pulled per bulk-dequeue.
        //  on_dequeue - receives the dequeued entry by rvalue-ref (move).
        auto dequeue(uint16_t max_ms,
                     uint16_t max_count_per_iteration,
                     const std::function<void(CallStackEntry&&)>& on_dequeue) -> void;

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

        [[nodiscard]] auto get_average_enqueue_time() const -> double;
        [[nodiscard]] auto get_average_dequeue_time() const -> double;

    private:
        Middleware();
        ~Middleware();
        auto assert_on_imgui_thread() const -> void;
        [[nodiscard]] auto is_tick_fn(const RC::Unreal::UFunction* fn) const -> bool;
        auto stop_impl(bool do_assert) -> bool;

    private:
        // Hook state
        RC::Unreal::Hook::GlobalCallbackId m_prehook_id = RC::Unreal::Hook::ERROR_ID;
        RC::Unreal::Hook::GlobalCallbackId m_posthook_id = RC::Unreal::Hook::ERROR_ID;

        EMiddlewareHookTarget m_hook_target = EMiddlewareHookTarget::ProcessEvent;
        bool m_paused = true;

        std::thread::id m_imgui_id{};

        RC::Unreal::Hook::FCallbackOptions m_options{false, true, STR("EventViewer"), STR("CallStackMonitor")};

        // Hook callbacks (initialized in ctor)
        RC::Unreal::Hook::ProcessEventCallbackWithData m_pe_pre{};
        RC::Unreal::Hook::ProcessEventCallbackWithData m_pe_post{};
        RC::Unreal::Hook::ProcessInternalCallbackWithData m_pi_pre{};
        RC::Unreal::Hook::ProcessInternalCallbackWithData m_pi_post{};

        // Queue
        moodycamel::ConcurrentQueue<CallStackEntry> m_queue{};
        moodycamel::ConsumerToken m_imgui_consumer_token{m_queue};
        std::vector<CallStackEntry> m_buffer{};

        // Detected tick functions
        inline static std::unordered_set<RC::Unreal::UObject*> m_tick_fns{};

        // Depth tracking
        inline static thread_local uint32_t m_pe_depth = 0;
        inline static thread_local uint32_t m_pi_depth = 0;
        inline static std::atomic_uint64_t m_depth_reset_counter = 0;
    };
} // namespace RC::EventViewerMod
