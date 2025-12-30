#pragma once

#include <Common.hpp>

namespace RC
{
    /**
     * UE4SS Runtime Information
     *
     * This struct provides static functions to query UE4SS runtime state.
     * These can be used by both C++ mods and Lua scripts to gracefully
     * handle missing functionality or adapt behavior based on user configuration.
     *
     * Categories:
     * - Hook availability: Whether AOB scans succeeded for various hooks
     * - User preferences: Configuration choices that affect behavior
     * - Feature availability: Whether certain engine features were found
     */
    struct RC_UE4SS_API UE4SSRuntime
    {
        // ========== Hook Availability ==========
        // These check if the underlying functions were found during AOB scan.
        // The actual detours are created lazily when callbacks are registered.

        /**
         * Check if the EngineTick function is available for hooking.
         * This checks if the AOB scan found UEngine::Tick.
         * Required for:
         * - Frame-based delays (ExecuteInGameThreadAfterFrames, LoopInGameThreadAfterFrames)
         * - ExecuteInGameThread with EGameThreadMethod.EngineTick
         * @return true if EngineTick function was found and can be hooked
         */
        static auto IsEngineTickAvailable() -> bool;

        /**
         * Check if the ProcessEvent function is available for hooking.
         * This checks if the AOB scan found UObject::ProcessEvent.
         * Required for:
         * - ExecuteInGameThread with EGameThreadMethod.ProcessEvent
         * - RegisterHook functionality
         * @return true if ProcessEvent function was found and can be hooked
         */
        static auto IsProcessEventAvailable() -> bool;

        // ========== Feature Availability ==========

        /**
         * Check if the FUObjectHashTables singleton is available.
         * This checks if the AOB scan found FUObjectHashTables::Get().
         * Required for:
         * - Fast class-based object iteration via ClassToObjectListMap
         * - ForEachObjectOfClass and ForEachObjectOfClassIncludingDerived
         * @return true if FUObjectHashTables::Get() was found and can be used
         */
        static auto IsFUObjectHashTablesAvailable() -> bool;

        /**
         * Check if hash table iteration should be used instead of GUObjectArray.
         * Returns false if:
         * - FUObjectHashTables is not available
         * - User has configured to force GUObjectArray iteration
         * @return true if hash table iteration is available and enabled
         */
        static auto ShouldUseHashTableIteration() -> bool;

        // Add more runtime checks here as needed, e.g.,:
        // - User preferences (FavorVirtualWrappers vs FavorExplicitImplementation, etc.)
        // - Feature availability (FText constructor found, specific UE version features, etc.)
    };
} // namespace RC
