#pragma once
#include <chrono>
#include <functional>
#include <memory>

#include <Enums.h>
#include <Structs.h>

#include <Unreal/CoreUObject/UObject/Class.hpp>

namespace RC::EventViewerMod
{
    class IMiddleware
    {
    public:
        // [Thread-ImGui] Sets the EMiddlewareHookTarget to use. Does not automatically start the stream (call start()).
        virtual auto set_hook_target(EMiddlewareHookTarget target) -> void = 0;

        // [Thread-ImGui] Gets the current EMiddlewareHookTarget.
        [[nodiscard]] virtual auto get_hook_target() const -> EMiddlewareHookTarget = 0;

        // [Thread-Any] Enqueues info on a call. The hook_target is captured at enqueue-time to avoid misrouting
        // when targets are switched while the queue still has pending items.
        virtual auto enqueue(EMiddlewareHookTarget hook_target,
                             Unreal::UObject* context,
                             Unreal::UFunction* function,
                             uint32_t depth,
                             std::thread::id thread_id,
                             bool is_tick) -> void = 0;

        // [Thread-ImGui] Dequeues call info.
        //  max_ms - maximum wall time (ms) to spend dequeuing.
        //  max_count_per_iteration - for ConcurrentQueue: max items pulled per bulk-dequeue
        //                           for Mutex: max items pulled per lock hold (we release between batches).
        //  on_dequeue - receives the dequeued entry by rvalue-ref (move).
        virtual auto dequeue(uint16_t max_ms,
                             uint16_t max_count_per_iteration,
                             const std::function<void(CallStackEntry&&)>& on_dequeue) -> void = 0;

        // [Thread-ImGui] Pauses stream, removes hooks. Derived classes should also drain their queues.
        virtual auto stop() -> bool = 0;

        // [Thread-ImGui] Returns true if paused.
        [[nodiscard]] virtual auto is_paused() const -> bool = 0;

        // [Thread-ImGui] Resumes stream by reinstalling hooks.
        virtual auto start() -> bool = 0;

        // [Thread-ImGui] Sets the ImGui thread id, used for precondition assertions.
        virtual auto set_imgui_thread_id(std::thread::id id) -> void = 0;

        // [Thread-ImGui] Gets the ImGui thread id.
        [[nodiscard]] virtual auto get_imgui_thread_id() const -> std::thread::id = 0;

        [[nodiscard]] virtual auto get_type() const -> EMiddlewareThreadScheme = 0;

        [[nodiscard]] virtual auto get_average_enqueue_time() const -> double = 0;
        [[nodiscard]] virtual auto get_average_dequeue_time() const -> double = 0;

        virtual ~IMiddleware() = default;
    };

    // Gets a new instance of IMiddleware depending on the EMiddlewareThreadScheme.
    auto GetNewMiddleware(EMiddlewareThreadScheme type) -> std::unique_ptr<IMiddleware>;
} // namespace RC::EventViewerMod
