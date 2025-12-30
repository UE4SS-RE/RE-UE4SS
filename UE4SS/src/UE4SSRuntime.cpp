#include <UE4SSRuntime.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectHashTables.hpp>
#include <Unreal/UnrealVersion.hpp>

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

    auto UE4SSRuntime::IsFUObjectHashTablesAvailable() -> bool
    {
        return Unreal::FUObjectHashTables::IsAvailable();
    }

    auto UE4SSRuntime::ShouldUseHashTableIteration() -> bool
    {
        // Check if FUObjectHashTables is available
        if (!IsFUObjectHashTablesAvailable())
        {
            return false;
        }

        // Check if user has forced GUObjectArray iteration
        if (UE4SSProgram::get_program().settings_manager.General.ForceGUObjectArrayForIteration)
        {
            return false;
        }

        return true;
    }
} // namespace RC
