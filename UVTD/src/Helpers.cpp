#include <format>

#include <UVTD/Helpers.hpp>
#include <Helpers/String.hpp>

namespace RC::UVTD
{
    auto to_string_type(const char* c_str) -> File::StringType
    {
#if RC_IS_ANSI == 1
        return File::StringType(c_str);
#else
        size_t count = strlen(c_str) + 1;
        wchar_t* converted_method_name = new wchar_t[count];

        size_t num_of_char_converted = 0;
        mbstowcs_s(&num_of_char_converted, converted_method_name, count, c_str, count);

        auto converted = File::StringType(converted_method_name);

        delete[] converted_method_name;

        return converted;
#endif
    }

    auto is_valid_type_to_dump(File::StringType type_name) -> bool
    {
        if (type_name.find(STR("FUnversionedStructSchema")) != type_name.npos ||
            type_name.find(STR("ELifetimeCondition")) != type_name.npos ||
            type_name.find(STR("UAISystemBase")) != type_name.npos ||
            type_name.find(STR("FLevelCollection")) != type_name.npos ||
            type_name.find(STR("FThreadSafeCounter")) != type_name.npos ||
            type_name.find(STR("FWorldAsyncTraceState")) != type_name.npos ||
            type_name.find(STR("FDelegateHandle")) != type_name.npos ||
            type_name.find(STR("UAvoidanceManager")) != type_name.npos ||
            type_name.find(STR("FOnBeginTearingDownEvent")) != type_name.npos ||
            type_name.find(STR("UBlueprint")) != type_name.npos ||
            type_name.find(STR("UCanvas")) != type_name.npos ||
            type_name.find(STR("UActorComponent")) != type_name.npos ||
            type_name.find(STR("AController")) != type_name.npos ||
            type_name.find(STR("ULevel")) != type_name.npos ||
            type_name.find(STR("FPhysScene_Chaos")) != type_name.npos ||
            type_name.find(STR("APhysicsVolume")) != type_name.npos ||
            type_name.find(STR("UDemoNetDriver")) != type_name.npos ||
            type_name.find(STR("FEndPhysicsTickFunction")) != type_name.npos ||
            type_name.find(STR("FFXSystemInterface")) != type_name.npos ||
            type_name.find(STR("ERHIFeatureLevel")) != type_name.npos ||
            type_name.find(STR("EFlushLevelStreamingType")) != type_name.npos ||
            type_name.find(STR("ULineBatchComponent")) != type_name.npos ||
            type_name.find(STR("AGameState")) != type_name.npos ||
            type_name.find(STR("FOnGameStateSetEvent")) != type_name.npos ||
            type_name.find(STR("AAudioVolume")) != type_name.npos ||
            type_name.find(STR("FLatentActionManager")) != type_name.npos ||
            type_name.find(STR("FOnLevelsChangedEvent")) != type_name.npos ||
            type_name.find(STR("AParticleEventManager")) != type_name.npos ||
            type_name.find(STR("UNavigationSystem")) != type_name.npos ||
            type_name.find(STR("UNetDriver")) != type_name.npos ||
            type_name.find(STR("AGameNetworkManager")) != type_name.npos ||
            type_name.find(STR("ETravelType")) != type_name.npos ||
            type_name.find(STR("FDefaultDelegateUserPolicy")) != type_name.npos ||
            type_name.find(STR("TMulticastDelegate")) != type_name.npos ||
            type_name.find(STR("FActorsInitializedParams")) != type_name.npos ||
            type_name.find(STR("FOnBeginPostProcessSettings")) != type_name.npos ||
            type_name.find(STR("FIntVector")) != type_name.npos ||
            type_name.find(STR("UGameInstance")) != type_name.npos ||
            type_name.find(STR("FWorldPSCPool")) != type_name.npos ||
            type_name.find(STR("UMaterialParameterCollectionInstance")) != type_name.npos ||
            type_name.find(STR("FParticlePerfStats")) != type_name.npos ||
            type_name.find(STR("FWorldInGamePerformanceTrackers")) != type_name.npos ||
            type_name.find(STR("UPhysicsCollisionHandler")) != type_name.npos ||
            type_name.find(STR("UPhysicsFieldComponent")) != type_name.npos ||
            type_name.find(STR("FPhysScene")) != type_name.npos ||
            type_name.find(STR("APlayerController")) != type_name.npos ||
            type_name.find(STR("IInterface_PostProcessVolume")) != type_name.npos ||
            type_name.find(STR("FOnTickFlushEvent")) != type_name.npos ||
            type_name.find(STR("FSceneInterface")) != type_name.npos ||
            type_name.find(STR("FStartAsyncSimulationFunction")) != type_name.npos ||
            type_name.find(STR("FStartPhysicsTickFunction")) != type_name.npos ||
            type_name.find(STR("FOnNetTickEvent")) != type_name.npos ||
            type_name.find(STR("ETickingGroup")) != type_name.npos ||
            type_name.find(STR("FTickTaskLevel")) != type_name.npos ||
            type_name.find(STR("FTimerManager")) != type_name.npos ||
            type_name.find(STR("FURL")) != type_name.npos ||
            type_name.find(STR("UWorldComposition")) != type_name.npos ||
            type_name.find(STR("EWorldType")) != type_name.npos ||
            type_name.find(STR("FSubsystemCollection")) != type_name.npos ||
            type_name.find(STR("UWorldSubsystem")) != type_name.npos ||
            type_name.find(STR("FStreamingLevelsToConsider")) != type_name.npos ||
            type_name.find(STR("APawn")) != type_name.npos ||
            type_name.find(STR("ACameraActor")) != type_name.npos ||
            type_name.find(STR("EMapPropertyFlags")) != type_name.npos ||
            type_name.find(STR("FScriptMapLayout")) != type_name.npos ||
            type_name.find(STR("EArrayPropertyFlags")) != type_name.npos ||
            type_name.find(STR("ICppClassTypeInfo")) != type_name.npos ||
            type_name.find(STR("FDefaultSetAllocator")) != type_name.npos ||
            type_name.find(STR("TDefaultMapKeyFuncs")) != type_name.npos ||
            type_name.find(STR("FNativeFunctionLookup")) != type_name.npos ||
            type_name.find(STR("FGCReferenceTokenStream")) != type_name.npos ||
            type_name.find(STR("FWindowsCriticalSection")) != type_name.npos ||
            type_name.find(STR("TDefaultMapHashableKeyFuncs")) != type_name.npos ||
            type_name.find(STR("FWindowsRWLock")) != type_name.npos ||
            type_name.find(STR("FRepRecord")) != type_name.npos ||
            type_name.find(STR("EClassCastFlags")) != type_name.npos ||
            type_name.find(STR("FAudioDeviceHandle")) != type_name.npos ||
            type_name.find(STR("TVector")) != type_name.npos ||
            type_name.find(STR("FScriptSetLayout")) != type_name.npos
            )
        {

            // These types are not currently supported in RC::Unreal, so we must prevent code from being generated.
            return false;
        }
        else
        {
            return true;
        }
    }
}