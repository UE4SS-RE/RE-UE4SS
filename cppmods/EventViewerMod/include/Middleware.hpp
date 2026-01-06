#pragma once
#include <memory>
#include <functional>
#include <chrono>

#include <Structs.h>

#include <Unreal/CoreUObject/UObject/Class.hpp>

namespace RC::EventViewerMod
{
    enum class EMiddlewareThreadScheme
    {
        cqueue,
        mutex
    };

    enum class EMiddlewareHookTarget
    {
        process_event,
        process_internal
    };

    class IMiddleware
    {
    public:
        virtual auto set_hook_target(EMiddlewareHookTarget target) -> void = 0;
        virtual auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id) -> void = 0;
        virtual auto dequeue_for_max_time(const std::chrono::duration<std::chrono::milliseconds>& max_ms, std::function<void(CallStackEntry*)> on_dequeue) -> void = 0;
        virtual auto clear() -> void = 0;
        virtual ~IMiddleware() = default;
    };

    auto GetNewMiddleware(EMiddlewareThreadScheme type) -> std::unique_ptr<IMiddleware>;
}