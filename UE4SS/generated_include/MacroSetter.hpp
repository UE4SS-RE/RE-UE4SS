if (auto val = parser.get_int64(STR("UObjectBase"), STR("Class"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("Class"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("ClassPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("InternalIndex"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("InternalIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Name"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("Name"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("NamePrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("ObjectFlags"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ObjectFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Outer"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("Outer"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameSession"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameSessionClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSessionClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("HUDClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("OptionsString"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("ServerStatReplicator"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("ServerStatReplicatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("SpectatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("bPauseable"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bPauseable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("bStartPlayersAsSpectators"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bStartPlayersAsSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("bUseSeamlessTravel"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bUseSeamlessTravel"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UFunction"), STR("Func"), -1); val != -1) Unreal::UFunction::MemberOffsets.emplace(STR("Func"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UField"), STR("Next"), -1); val != -1) Unreal::UField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UStruct"), STR("Script"), -1); val != -1) Unreal::UStruct::MemberOffsets.emplace(STR("Script"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ActiveSplitscreenType"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ActiveSplitscreenType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentBufferVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentBufferVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentLumenVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentLumenVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentNaniteVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentNaniteVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentVirtualShadowMapVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentVirtualShadowMapVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("DebugProperties"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("DebugProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("EngineShowFlags"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("EngineShowFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("GameLayerManagerPtr"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("GameLayerManagerPtr"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HighResScreenshotDialog"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HighResScreenshotDialog"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MaxSplitscreenPlayers"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MaxSplitscreenPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MouseCaptureMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseCaptureMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MouseLockMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseLockMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("SplitscreenInfo"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("SplitscreenInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("StatHitchesData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatHitchesData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("StatUnitData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatUnitData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("TitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("TitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewModeIndex"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewModeIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("Viewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Viewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportConsole"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportConsole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportFrame"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportOverlayWidget"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportOverlayWidget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("Window"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Window"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("World"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("World"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bDisableSplitScreenOverride"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableSplitScreenOverride"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bDisableWorldRendering"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableWorldRendering"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bHasAudioFocus"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHasAudioFocus"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bHideCursorDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHideCursorDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIgnoreInput"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIgnoreInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIsMouseOverClient"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsMouseOverClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIsPlayInEditorViewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsPlayInEditorViewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bLockDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bLockDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bShowTitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bShowTitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bSuppressTransitionMessage"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bSuppressTransitionMessage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bUseSoftwareCursorWidgets"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bUseSoftwareCursorWidgets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArAllowLazyLoading"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArAllowLazyLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArContainsCode"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArContainsCode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArContainsMap"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArContainsMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArEngineVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArForceByteSwapping"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArForceByteSwapping"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArForceUnicode"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArForceUnicode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreArchetypeRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreArchetypeRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreClassGeneratedByRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreClassGeneratedByRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreClassRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreClassRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreOuterRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreOuterRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsCountingMemory"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsCountingMemory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsCriticalError"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsCriticalError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsError"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsFilterEditorOnly"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsFilterEditorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsLoading"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsModifyingWeakAndStrongReferences"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsModifyingWeakAndStrongReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsNetArchive"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsNetArchive"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsObjectReferenceCollector"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsObjectReferenceCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsPersistent"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsPersistent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsSaveGame"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsSaveGame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsSaving"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsSaving"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsTextFormat"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsTextFormat"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsTransacting"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsTransacting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNoDelta"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNoDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNoIntraPropertyDelta"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNoIntraPropertyDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArPortFlags"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArRequiresLocalizationGather"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArRequiresLocalizationGather"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArShouldSkipBulkData"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArShouldSkipBulkData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArUseCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUseCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArWantBinaryPropertySerialization"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArWantBinaryPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("SerializedProperty"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("CurrentID"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("CurrentID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("EngineMessageClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("EngineMessageClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("GameModeClassAliases"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameModeClassAliases"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("GameSession"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("HUDClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("InactivePlayerArray"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerArray"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("InactivePlayerStateLifeSpan"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerStateLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MatchState"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MatchState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MaxInactivePlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MaxInactivePlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MinRespawnDelay"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MinRespawnDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumBots"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumBots"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumTravellingPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumTravellingPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("OptionsString"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("SpectatorClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bDelayedStart"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bDelayedStart"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bHandleDedicatedServerReplays"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bHandleDedicatedServerReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bPauseable"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bPauseable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bStartPlayersAsSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bStartPlayersAsSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bUseSeamlessTravel"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bUseSeamlessTravel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("AttachmentReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AttachmentReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("AutoReceiveInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AutoReceiveInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CachedLastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CachedLastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ControllingMatineeActors"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ControllingMatineeActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CreationTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CreationTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CustomTimeDilation"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CustomTimeDilation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("DetachFence"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DetachFence"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("HiddenEditorViews"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("HiddenEditorViews"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InitialLifeSpan"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InitialLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputConsumeOption_DEPRECATED"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputConsumeOption_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("LastNetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastNetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("LastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Layers"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("Layers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("MinNetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("MinNetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetCullDistanceSquared"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetCullDistanceSquared"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetDormancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDormancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetDriverName"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDriverName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetTag"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("NetTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorBeginOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorBeginOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorEndOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorEndOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorHit"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorHit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnBeginCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnBeginCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnClicked"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnClicked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnEndCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnEndPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchBegin"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchBegin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchEnd"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnd"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchEnter"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnter"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchLeave"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchLeave"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnReleased"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnReleased"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakeAnyDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeAnyDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakePointDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakePointDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakeRadialDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeRadialDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("PrimaryActorTick"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("PrimaryActorTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RayTracingGroupId"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RayTracingGroupId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RemoteRole"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RemoteRole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedComponentsInfo"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedComponentsInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedSubObjects"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedSubObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Role"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("Role"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RootComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RootComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("SpawnCollisionHandlingMethod"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("SpawnCollisionHandlingMethod"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Tags"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("Tags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("TimerHandle_LifeSpanExpired"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("TimerHandle_LifeSpanExpired"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("UpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("UpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorBeginningPlayFromLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorBeginningPlayFromLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorEnableCollision"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorEnableCollision"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorInitialized"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorIsBeingConstructed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorIsBeingConstructed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorIsBeingDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorIsBeingDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorSeamlessTraveled"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorSeamlessTraveled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorWantsDestroyDuringBeginPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorWantsDestroyDuringBeginPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAllowReceiveTickEventOnDedicatedServer"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAllowReceiveTickEventOnDedicatedServer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAllowTickBeforeBeginPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAllowTickBeforeBeginPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAlwaysRelevant"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAlwaysRelevant"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAsyncPhysicsTickEnabled"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAsyncPhysicsTickEnabled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAutoDestroyWhenFinished"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAutoDestroyWhenFinished"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bBlockInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bBlockInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCallPreReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCallPreReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCallPreReplicationForReplay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCallPreReplicationForReplay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCanBeDamaged"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCanBeDamaged"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCanBeInCluster"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCanBeInCluster"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCollideWhenPlacing"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCollideWhenPlacing"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bEnableAutoLODGeneration"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bEnableAutoLODGeneration"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bExchangedRoles"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bExchangedRoles"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bFindCameraComponentWhenViewTarget"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bFindCameraComponentWhenViewTarget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bForceNetAddressable"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bForceNetAddressable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bGenerateOverlapEventsDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bGenerateOverlapEventsDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasDeferredComponentRegistration"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasDeferredComponentRegistration"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasFinishedSpawning"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasFinishedSpawning"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasRegisteredAllComponents"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasRegisteredAllComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHidden"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("bHidden"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bIgnoresOriginShifting"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bIgnoresOriginShifting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bIsEditorOnlyActor"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bIsEditorOnlyActor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetCheckedInitialPhysicsState"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetCheckedInitialPhysicsState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetLoadOnClient"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetLoadOnClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetStartup"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetStartup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetTemporary"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetTemporary"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetUseOwnerRelevancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetUseOwnerRelevancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bOnlyRelevantToOwner"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bOnlyRelevantToOwner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bPendingNetUpdate"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bPendingNetUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bRelevantForLevelBounds"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRelevantForLevelBounds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bRelevantForNetworkReplays"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRelevantForNetworkReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplayRewindable"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplayRewindable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplicateMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicateMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplicateUsingRegisteredSubObjectList"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicateUsingRegisteredSubObjectList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplicates"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicates"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bRunningUserConstructionScript"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRunningUserConstructionScript"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bTearOff"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bTearOff"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bTickFunctionsRegistered"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bTickFunctionsRegistered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("ConfiguredInternetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredInternetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("ConfiguredLanSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredLanSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("CurrentNetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("CurrentNetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FByteProperty"), STR("Enum"), -1); val != -1)
    Unreal::FByteProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("AspectRatioAxisConstraint"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("AspectRatioAxisConstraint"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ControllerId"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ControllerId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("LastViewLocation"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("LastViewLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ViewportClient"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ViewportClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("bSentSplitJoin"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("bSentSplitJoin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMulticastDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FMulticastDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FObjectPropertyBase"), STR("PropertyClass"), -1); val != -1)
    Unreal::FObjectPropertyBase::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArAllowLazyLoading"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArAllowLazyLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArContainsCode"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArContainsCode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArContainsMap"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArContainsMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArEngineVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArForceByteSwapping"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArForceByteSwapping"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArForceUnicode"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArForceUnicode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreArchetypeRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreArchetypeRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreClassGeneratedByRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreClassGeneratedByRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreClassRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreClassRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreOuterRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreOuterRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsCountingMemory"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsCountingMemory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsCriticalError"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsCriticalError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsError"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsFilterEditorOnly"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsFilterEditorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsLoading"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsLoadingFromCookedPackage"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsLoadingFromCookedPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsModifyingWeakAndStrongReferences"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsModifyingWeakAndStrongReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsNetArchive"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsNetArchive"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsObjectReferenceCollector"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsObjectReferenceCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsPersistent"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsPersistent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsSaveGame"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsSaveGame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsSaving"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsSaving"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsTextFormat"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsTextFormat"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsTransacting"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsTransacting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArLicenseeUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArNoDelta"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArNoDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArNoIntraPropertyDelta"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArNoIntraPropertyDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArPortFlags"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArRequiresLocalizationGather"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArRequiresLocalizationGather"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArShouldSkipBulkData"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipBulkData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArShouldSkipCompilingAssets"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipCompilingAssets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUseCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUseCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUseUnversionedPropertySerialization"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUseUnversionedPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArWantBinaryPropertySerialization"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArWantBinaryPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("SerializedProperty"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("ClassPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("FlagsPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("FlagsPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("NamePrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Next"), -1); val != -1) Unreal::FField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Owner"), -1); val != -1) Unreal::FField::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UWorld"), STR("CachedViewInfoRenderedLastFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CachedViewInfoRenderedLastFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CleanupWorldTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CleanupWorldTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CommittedPersistentLevelName"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CommittedPersistentLevelName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ContentBundleManager"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ContentBundleManager"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UWorld"), STR("LastRenderTime"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LastTimeUnbuiltLightingWasEncountered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastTimeUnbuiltLightingWasEncountered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LevelSequenceActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LevelSequenceActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextSwitchCountdown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextSwitchCountdown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextURL"), -1); val != -1) Unreal::UWorld::MemberOffsets.emplace(STR("NextURL"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UWorld"), STR("OnWorldPartitionInitializedEvent"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OnWorldPartitionInitializedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("OnWorldPartitionUninitializedEvent"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OnWorldPartitionUninitializedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("OriginLocation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OriginLocation"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UWorld"), STR("RequestedOriginLocation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RequestedOriginLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ServerStreamingLevelsVisibility"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ServerStreamingLevelsVisibility"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UWorld"), STR("bHasEverBeenInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHasEverBeenInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bInTick"), -1); val != -1) Unreal::UWorld::MemberOffsets.emplace(STR("bInTick"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UWorld"), STR("bSupportsMakingInvisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingInvisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bSupportsMakingVisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingVisibleTransactionRequests"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UClass"), STR("CppClassStaticFunctions"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("CppClassStaticFunctions"), static_cast<int32_t>(val));
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
if (auto val = parser.get_int64(STR("UClass"), STR("bCooked"), -1); val != -1) Unreal::UClass::MemberOffsets.emplace(STR("bCooked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bLayoutChanging"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bLayoutChanging"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppForm"), -1); val != -1) Unreal::UEnum::MemberOffsets.emplace(STR("CppForm"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppType"), -1); val != -1) Unreal::UEnum::MemberOffsets.emplace(STR("CppType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumDisplayNameFn"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumDisplayNameFn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumFlags_Internal"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumPackage"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("Names"), -1); val != -1) Unreal::UEnum::MemberOffsets.emplace(STR("Names"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("KeyProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("KeyProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("ValueProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("ValueProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FStructProperty"), STR("Struct"), -1); val != -1)
    Unreal::FStructProperty::MemberOffsets.emplace(STR("Struct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("Inner"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("Inner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FInterfaceProperty"), STR("InterfaceClass"), -1); val != -1)
    Unreal::FInterfaceProperty::MemberOffsets.emplace(STR("InterfaceClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldPathProperty"), STR("PropertyClass"), -1); val != -1)
    Unreal::FFieldPathProperty::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
