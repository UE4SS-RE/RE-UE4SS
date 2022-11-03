if (auto val = parser.get_int64(STR("UObjectBase"), STR("ClassPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("InternalIndex"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("InternalIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("NamePrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("ObjectFlags"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ObjectFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("OuterPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("OuterPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("Alignment"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Alignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("Size"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Size"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("ArrayDim"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ArrayDim"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("DestructorLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("DestructorLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("ElementSize"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ElementSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("NextRef"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("NextRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("Offset_Internal"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("Offset_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PostConstructLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PostConstructLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PropertyFlags"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PropertyLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("RepIndex"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("RepNotifyFunc"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepNotifyFunc"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSoftClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FSoftClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("bAutoEmitLineTerminator"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bAutoEmitLineTerminator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("bSuppressEventTag"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bSuppressEventTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("EventGraphCallOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphCallOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("EventGraphFunction"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("FirstPropertyToInit"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FirstPropertyToInit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("Func"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("Func"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("FunctionFlags"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FunctionFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("NumParms"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("NumParms"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("ParmsSize"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ParmsSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RPCId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RPCResponseId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCResponseId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RepOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RepOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("ReturnValueOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ReturnValueOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UField"), STR("Next"), -1); val != -1)
    Unreal::UField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("Enum"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("UnderlyingProp"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("UnderlyingProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ChildProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ChildProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("Children"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Children"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("DestructorLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("DestructorLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("MinAlignment"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("MinAlignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PostConstructLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PostConstructLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PropertiesSize"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertiesSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PropertyLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertyLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("RefLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("RefLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("Script"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Script"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ScriptAndPropertyObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptAndPropertyObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ScriptObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("SuperStruct"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("SuperStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("UnresolvedScriptProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("UnresolvedScriptProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMulticastDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FMulticastDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FObjectPropertyBase"), STR("PropertyClass"), -1); val != -1)
    Unreal::FObjectPropertyBase::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("ClassPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("FlagsPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("FlagsPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("NamePrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Next"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Owner"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("ByteMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("ByteOffset"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("FieldMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("FieldSize"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("CppStructOps"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("CppStructOps"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("StructFlags"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("StructFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("bCppStructOpsFromBaseClass"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bCppStructOpsFromBaseClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("bPrepareCppStructOpsCompleted"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bPrepareCppStructOpsCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ActiveLevelCollectionIndex"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ActiveLevelCollectionIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AudioTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("BlockTillLevelStreamingCompletedEpoch"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BlockTillLevelStreamingCompletedEpoch"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("BuildStreamingDataTimer"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BuildStreamingDataTimer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CleanupWorldTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CleanupWorldTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CommittedPersistentLevelName"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CommittedPersistentLevelName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DebugDrawTraceTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DebugDrawTraceTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeltaRealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaRealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeltaTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ExtraReferencedObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ExtraReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("FullPurgeTriggered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("FullPurgeTriggered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("IsInBlockTillLevelStreamingCompleted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("IsInBlockTillLevelStreamingCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LWILastAssignedUID"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LWILastAssignedUID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LastTimeUnbuiltLightingWasEncountered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastTimeUnbuiltLightingWasEncountered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LevelSequenceActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LevelSequenceActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextSwitchCountdown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextSwitchCountdown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextURL"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumInvalidReflectionCaptureComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumInvalidReflectionCaptureComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumLightingUnbuiltObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumLightingUnbuiltObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumStreamingLevelsBeingLoaded"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumStreamingLevelsBeingLoaded"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumTextureStreamingDirtyResources"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingDirtyResources"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumTextureStreamingUnbuiltComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingUnbuiltComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumUnbuiltReflectionCaptures"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumUnbuiltReflectionCaptures"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("OriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PauseDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PauseDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PerModuleDataObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PerModuleDataObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PlayerNum"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PlayerNum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PreparingLevelNames"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PreparingLevelNames"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("RealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("StreamingLevelsPrefix"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingLevelsPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("StreamingVolumeUpdateDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingVolumeUpdateDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("TimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("TimeSinceLastPendingKillPurge"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSinceLastPendingKillPurge"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("UnpausedTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("UnpausedTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ViewLocationsRenderedLastFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ViewLocationsRenderedLastFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bActorsInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bActorsInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAggressiveLOD"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAggressiveLOD"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAllowAudioPlayback"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowAudioPlayback"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAllowDeferredPhysicsStateCreation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowDeferredPhysicsStateCreation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAreConstraintsDirty"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAreConstraintsDirty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bBegunPlay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bBegunPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bCleanedUpWorld"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCleanedUpWorld"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bCreateRenderStateForHiddenComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCreateRenderStateForHiddenComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugDrawAllTraceTags"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugDrawAllTraceTags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugFrameStepExecution"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugFrameStepExecution"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugPauseExecution"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugPauseExecution"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDoDelayedUpdateCullDistanceVolumes"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDoDelayedUpdateCullDistanceVolumes"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDropDetail"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDropDetail"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bHack_Force_UsesGameHiddenFlags_True"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHack_Force_UsesGameHiddenFlags_True"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bInTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bInTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bInitializedAndNeedsCleanup"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bInitializedAndNeedsCleanup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsBuilt"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsBuilt"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsCameraMoveableWhenPaused"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsCameraMoveableWhenPaused"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsDefaultLevel"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsDefaultLevel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsLevelStreamingFrozen"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsLevelStreamingFrozen"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsRunningConstructionScript"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsRunningConstructionScript"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsTearingDown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsTearingDown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsWorldInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsWorldInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bKismetScriptError"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bKismetScriptError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bMarkedObjectsPendingKill"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMarkedObjectsPendingKill"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bMatchStarted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMatchStarted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bOriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bOriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bPlayersOnly"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPlayersOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bPlayersOnlyPending"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPlayersOnlyPending"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bPostTickComponentUpdate"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPostTickComponentUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bRequestedBlockOnAsyncLoading"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bRequestedBlockOnAsyncLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bRequiresHitProxies"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bRequiresHitProxies"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldDelayGarbageCollect"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldDelayGarbageCollect"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldForceUnloadStreamingLevels"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldForceUnloadStreamingLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldForceVisibleStreamingLevels"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldForceVisibleStreamingLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldSimulatePhysics"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldSimulatePhysics"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bStartup"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bStartup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bStreamingDataDirty"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bStreamingDataDirty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bTickNewlySpawned"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bTickNewlySpawned"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bTriggerPostLoadMap"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bTriggerPostLoadMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bWorldWasLoadedThisTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bWorldWasLoadedThisTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSetProperty"), STR("ElementProp"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("ElementProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassAddReferencedObjects"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassAddReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassCastFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassCastFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassConfigName"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConfigName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassConstructor"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConstructor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassDefaultObject"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassDefaultObject"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassGeneratedBy"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassGeneratedBy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassUnique"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassUnique"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassVTableHelperCtorCaller"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassVTableHelperCtorCaller"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassWithin"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassWithin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FirstOwnedClassRep"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FirstOwnedClassRep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("Interfaces"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("Interfaces"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("NetFields"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("NetFields"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SparseClassData"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SparseClassDataStruct"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassDataStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("UberGraphFramePointerProperty"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("UberGraphFramePointerProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bCooked"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bCooked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bLayoutChanging"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bLayoutChanging"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppForm"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("CppForm"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppType"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("CppType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumDisplayNameFn"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumDisplayNameFn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumFlags_Internal"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("Names"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("Names"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("KeyProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("KeyProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("ValueProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("ValueProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FStructProperty"), STR("Struct"), -1); val != -1)
    Unreal::FStructProperty::MemberOffsets.emplace(STR("Struct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("Inner"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("Inner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FByteProperty"), STR("Enum"), -1); val != -1)
    Unreal::FByteProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FInterfaceProperty"), STR("InterfaceClass"), -1); val != -1)
    Unreal::FInterfaceProperty::MemberOffsets.emplace(STR("InterfaceClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldPathProperty"), STR("PropertyClass"), -1); val != -1)
    Unreal::FFieldPathProperty::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
