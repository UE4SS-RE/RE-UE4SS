#include <UE4SSRuntime.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObject.hpp>

namespace RC
{
    auto UE4SSRuntime::IsEngineTickAvailable() -> bool
    {
        // Check if the EngineTick function was found during AOB scan
        // The detour is created lazily when a callback is registered,
        // so we check if the underlying function is ready instead
        return Unreal::UEngine::TickInternal.is_ready();
    }

    auto UE4SSRuntime::IsProcessEventAvailable() -> bool
    {
        // Check if the ProcessEvent function was found during AOB scan
        // The detour is created lazily when a callback is registered,
        // so we check if the underlying function is ready instead
        return Unreal::UObject::ProcessEventInternal.is_ready();
    }
} // namespace RC
