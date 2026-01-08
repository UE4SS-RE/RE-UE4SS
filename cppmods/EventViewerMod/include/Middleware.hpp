#pragma once
#include <memory>
#include <functional>
#include <chrono>

#include <Structs.h>
#include <Enums.h>

#include <Unreal/CoreUObject/UObject/Class.hpp>

namespace RC::EventViewerMod
{
    class IMiddleware
    {
    public:
        // [Thread-ImGui] Sets the EMiddlewareHookTarget to use. Does not automatically start the stream (call start())
        virtual auto set_hook_target(EMiddlewareHookTarget target) -> void = 0;

        // [Thread-ImGui] Gets the current EMiddlewareHookTarget.
        [[nodiscard]] virtual auto get_hook_target() const -> EMiddlewareHookTarget = 0;

        // [Thread-Any] Enqueues info on a call
        virtual auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id) -> void = 0;

        // [Thread-ImGui] Dequeues the info on the call stack
        //      max_ms - the maximum amount of time, in ms, this function will run. Use to avoid holding the ImGui thread for too long
        //      max_count_per_iteration -   for EMiddlewareThreadScheme::ConcurrentQueue, the amount of CallStackEntries pulled at a time
        //                                  for EMiddlewareThreadScheme::Mutex, the amount of CallStackEntries pulled between Mutex locking/unlocking
        //                                      (to avoid locking the game thread for too long)
        //      on_dequeue - takes ownership the dequeued CallStackEntry*; it's responsible for cleaning it up/wrapping it in unique_ptr
        virtual auto dequeue(const std::chrono::milliseconds& max_ms, size_t max_count_per_iteration, const std::function<void(CallStackEntry*)>&
                             on_dequeue) -> void = 0;

        // [Thread-ImGui] Pauses the call stack stream and removes hooks and clears the queue
        virtual auto stop() -> bool = 0;

        // [Thread-ImGui] Returns true if paused
        [[nodiscard]] virtual auto is_paused() const -> bool = 0;

        // [Thread-ImGui] Resumes the call stack stream by reinstalling hooks
        virtual auto start() -> bool = 0;

        // [Thread-ImGui] Sets the ImGui thread id, used for precondition assertions
        virtual auto set_imgui_thread_id(std::thread::id id) -> void = 0;

        virtual ~IMiddleware() = default;
    };

    // Gets a new instance of IMiddleware depending on the EMiddlewareThreadScheme
    auto GetNewMiddleware(EMiddlewareThreadScheme type) -> std::unique_ptr<IMiddleware>;
}