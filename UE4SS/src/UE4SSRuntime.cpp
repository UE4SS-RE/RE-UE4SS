#include <UE4SSRuntime.hpp>
#include <Unreal/Hooks.hpp>

namespace RC
{
    auto UE4SSRuntime::IsEngineTickAvailable() -> bool
    {
        return Unreal::Hook::StaticStorage::EngineTickDetour != nullptr;
    }

    auto UE4SSRuntime::IsProcessEventAvailable() -> bool
    {
        return Unreal::Hook::StaticStorage::ProcessEventDetour != nullptr;
    }
} // namespace RC
