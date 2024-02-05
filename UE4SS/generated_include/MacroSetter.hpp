if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("Class"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("Class"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("ClassPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("InternalIndex"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("InternalIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("Name"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("Name"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("NamePrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("ObjectFlags"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ObjectFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("Outer"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("Outer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UObjectBase"), SYSSTR("OuterPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("OuterPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UScriptStruct::ICppStructOps"), SYSSTR("Alignment"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Alignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UScriptStruct::ICppStructOps"), SYSSTR("Size"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Size"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("ArrayDim"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ArrayDim"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("DestructorLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("DestructorLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("ElementSize"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ElementSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("NextRef"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("NextRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("Offset_Internal"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("Offset_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("PostConstructLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PostConstructLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("PropertyFlags"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("PropertyLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("RepIndex"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FProperty"), SYSSTR("RepNotifyFunc"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepNotifyFunc"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FSoftClassProperty"), SYSSTR("MetaClass"), -1); val != -1)
    Unreal::FSoftClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("GameSession"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("GameSessionClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSessionClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("HUDClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("OptionsString"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("ServerStatReplicator"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("ServerStatReplicatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("SpectatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("bPauseable"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bPauseable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("bStartPlayersAsSpectators"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bStartPlayersAsSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameModeBase"), SYSSTR("bUseSeamlessTravel"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bUseSeamlessTravel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FOutputDevice"), SYSSTR("bAutoEmitLineTerminator"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bAutoEmitLineTerminator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FOutputDevice"), SYSSTR("bSuppressEventTag"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bSuppressEventTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("EventGraphCallOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphCallOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("EventGraphFunction"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("FirstPropertyToInit"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FirstPropertyToInit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("Func"), -1); val != -1) Unreal::UFunction::MemberOffsets.emplace(STR("Func"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("FunctionFlags"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FunctionFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("NumParms"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("NumParms"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("ParmsSize"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ParmsSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("RPCId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("RPCResponseId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCResponseId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("RepOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RepOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UFunction"), SYSSTR("ReturnValueOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ReturnValueOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UField"), SYSSTR("Next"), -1); val != -1) Unreal::UField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FEnumProperty"), SYSSTR("Enum"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FEnumProperty"), SYSSTR("UnderlyingProp"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("UnderlyingProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("ChildProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ChildProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("Children"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Children"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("DestructorLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("DestructorLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("MinAlignment"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("MinAlignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("PostConstructLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PostConstructLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("PropertiesSize"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertiesSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("PropertyLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertyLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("RefLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("RefLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("Script"), -1); val != -1) Unreal::UStruct::MemberOffsets.emplace(STR("Script"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("ScriptAndPropertyObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptAndPropertyObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("ScriptObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("SuperStruct"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("SuperStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UStruct"), SYSSTR("UnresolvedScriptProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("UnresolvedScriptProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FDelegateProperty"), SYSSTR("SignatureFunction"), -1); val != -1)
    Unreal::FDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("ActiveSplitscreenType"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ActiveSplitscreenType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("CurrentBufferVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentBufferVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("CurrentLumenVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentLumenVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("CurrentNaniteVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentNaniteVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("CurrentVirtualShadowMapVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentVirtualShadowMapVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("DebugProperties"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("DebugProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("EngineShowFlags"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("EngineShowFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("GameLayerManagerPtr"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("GameLayerManagerPtr"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("HighResScreenshotDialog"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HighResScreenshotDialog"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("MaxSplitscreenPlayers"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MaxSplitscreenPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("MouseCaptureMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseCaptureMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("MouseLockMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseLockMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("SplitscreenInfo"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("SplitscreenInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("StatHitchesData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatHitchesData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("StatUnitData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatUnitData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("TitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("TitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("ViewModeIndex"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewModeIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("Viewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Viewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("ViewportConsole"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportConsole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("ViewportFrame"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("ViewportOverlayWidget"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportOverlayWidget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("Window"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Window"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("World"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("World"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bDisableSplitScreenOverride"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableSplitScreenOverride"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bDisableWorldRendering"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableWorldRendering"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bHasAudioFocus"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHasAudioFocus"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bHideCursorDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHideCursorDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bIgnoreInput"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIgnoreInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bIsMouseOverClient"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsMouseOverClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bIsPlayInEditorViewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsPlayInEditorViewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bLockDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bLockDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bShowTitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bShowTitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bSuppressTransitionMessage"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bSuppressTransitionMessage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UGameViewportClient"), SYSSTR("bUseSoftwareCursorWidgets"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bUseSoftwareCursorWidgets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArAllowLazyLoading"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArAllowLazyLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArContainsCode"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArContainsCode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArContainsMap"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArContainsMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArEngineVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArForceByteSwapping"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArForceByteSwapping"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArForceUnicode"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArForceUnicode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIgnoreArchetypeRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreArchetypeRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIgnoreClassGeneratedByRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreClassGeneratedByRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIgnoreClassRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreClassRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIgnoreOuterRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreOuterRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsCountingMemory"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsCountingMemory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsCriticalError"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsCriticalError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsError"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsFilterEditorOnly"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsFilterEditorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsLoading"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsModifyingWeakAndStrongReferences"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsModifyingWeakAndStrongReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsNetArchive"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsNetArchive"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsObjectReferenceCollector"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsObjectReferenceCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsPersistent"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsPersistent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsSaveGame"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsSaveGame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsSaving"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsSaving"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsTextFormat"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsTextFormat"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArIsTransacting"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsTransacting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArNoDelta"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNoDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArNoIntraPropertyDelta"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNoIntraPropertyDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArPortFlags"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArRequiresLocalizationGather"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArRequiresLocalizationGather"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArShouldSkipBulkData"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArShouldSkipBulkData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArUseCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUseCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("ArWantBinaryPropertySerialization"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArWantBinaryPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("SerializedProperty"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchive"), SYSSTR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("CurrentID"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("CurrentID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("EngineMessageClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("EngineMessageClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("GameModeClassAliases"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameModeClassAliases"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("GameSession"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("HUDClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("InactivePlayerArray"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerArray"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("InactivePlayerStateLifeSpan"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerStateLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("MatchState"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MatchState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("MaxInactivePlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MaxInactivePlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("MinRespawnDelay"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MinRespawnDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("NumBots"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumBots"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("NumPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("NumSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("NumTravellingPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumTravellingPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("OptionsString"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("SpectatorClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("bDelayedStart"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bDelayedStart"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("bHandleDedicatedServerReplays"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bHandleDedicatedServerReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("bPauseable"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bPauseable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("bStartPlayersAsSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bStartPlayersAsSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AGameMode"), SYSSTR("bUseSeamlessTravel"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bUseSeamlessTravel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("AttachmentReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AttachmentReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("AutoReceiveInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AutoReceiveInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("CachedLastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CachedLastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("ControllingMatineeActors"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ControllingMatineeActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("CreationTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CreationTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("CustomTimeDilation"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CustomTimeDilation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("DetachFence"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DetachFence"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("HiddenEditorViews"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("HiddenEditorViews"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("InitialLifeSpan"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InitialLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("InputComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("InputConsumeOption_DEPRECATED"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputConsumeOption_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("InputPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("LastNetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastNetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("LastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("Layers"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("Layers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("MinNetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("MinNetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetCullDistanceSquared"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetCullDistanceSquared"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetDormancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDormancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetDriverName"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDriverName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetTag"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("NetTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("NetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnActorBeginOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorBeginOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnActorEndOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorEndOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnActorHit"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorHit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnBeginCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnBeginCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnClicked"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnClicked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnEndCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnEndPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnInputTouchBegin"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchBegin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnInputTouchEnd"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnd"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnInputTouchEnter"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnter"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnInputTouchLeave"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchLeave"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnReleased"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnReleased"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnTakeAnyDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeAnyDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnTakePointDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakePointDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("OnTakeRadialDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeRadialDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("ParentComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("PrimaryActorTick"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("PrimaryActorTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("RayTracingGroupId"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RayTracingGroupId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("RemoteRole"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RemoteRole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("ReplicatedComponentsInfo"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedComponentsInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("ReplicatedMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("ReplicatedSubObjects"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedSubObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("Role"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("Role"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("RootComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RootComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("SpawnCollisionHandlingMethod"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("SpawnCollisionHandlingMethod"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("Tags"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("Tags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("TimerHandle_LifeSpanExpired"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("TimerHandle_LifeSpanExpired"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("UpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("UpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorBeginningPlayFromLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorBeginningPlayFromLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorEnableCollision"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorEnableCollision"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorInitialized"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorIsBeingConstructed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorIsBeingConstructed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorIsBeingDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorIsBeingDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorSeamlessTraveled"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorSeamlessTraveled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bActorWantsDestroyDuringBeginPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorWantsDestroyDuringBeginPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bAllowReceiveTickEventOnDedicatedServer"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAllowReceiveTickEventOnDedicatedServer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bAllowTickBeforeBeginPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAllowTickBeforeBeginPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bAlwaysRelevant"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAlwaysRelevant"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bAsyncPhysicsTickEnabled"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAsyncPhysicsTickEnabled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bAutoDestroyWhenFinished"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAutoDestroyWhenFinished"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bBlockInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bBlockInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bCallPreReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCallPreReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bCallPreReplicationForReplay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCallPreReplicationForReplay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bCanBeDamaged"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCanBeDamaged"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bCanBeInCluster"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCanBeInCluster"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bCollideWhenPlacing"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCollideWhenPlacing"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bEnableAutoLODGeneration"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bEnableAutoLODGeneration"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bExchangedRoles"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bExchangedRoles"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bFindCameraComponentWhenViewTarget"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bFindCameraComponentWhenViewTarget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bForceNetAddressable"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bForceNetAddressable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bGenerateOverlapEventsDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bGenerateOverlapEventsDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bHasDeferredComponentRegistration"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasDeferredComponentRegistration"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bHasFinishedSpawning"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasFinishedSpawning"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bHasRegisteredAllComponents"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasRegisteredAllComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bHidden"), -1); val != -1) Unreal::AActor::MemberOffsets.emplace(STR("bHidden"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bIgnoresOriginShifting"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bIgnoresOriginShifting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bIsEditorOnlyActor"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bIsEditorOnlyActor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bNetCheckedInitialPhysicsState"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetCheckedInitialPhysicsState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bNetLoadOnClient"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetLoadOnClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bNetStartup"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetStartup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bNetTemporary"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetTemporary"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bNetUseOwnerRelevancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetUseOwnerRelevancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bOnlyRelevantToOwner"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bOnlyRelevantToOwner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bPendingNetUpdate"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bPendingNetUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bRelevantForLevelBounds"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRelevantForLevelBounds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bRelevantForNetworkReplays"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRelevantForNetworkReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bReplayRewindable"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplayRewindable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bReplicateMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicateMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bReplicateUsingRegisteredSubObjectList"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicateUsingRegisteredSubObjectList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bReplicates"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicates"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bRunningUserConstructionScript"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRunningUserConstructionScript"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bTearOff"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bTearOff"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("AActor"), SYSSTR("bTickFunctionsRegistered"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bTickFunctionsRegistered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UPlayer"), SYSSTR("ConfiguredInternetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredInternetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UPlayer"), SYSSTR("ConfiguredLanSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredLanSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UPlayer"), SYSSTR("CurrentNetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("CurrentNetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FByteProperty"), SYSSTR("Enum"), -1); val != -1)
    Unreal::FByteProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("ULocalPlayer"), SYSSTR("AspectRatioAxisConstraint"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("AspectRatioAxisConstraint"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("ULocalPlayer"), SYSSTR("ControllerId"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ControllerId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("ULocalPlayer"), SYSSTR("LastViewLocation"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("LastViewLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("ULocalPlayer"), SYSSTR("ViewportClient"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ViewportClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("ULocalPlayer"), SYSSTR("bSentSplitJoin"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("bSentSplitJoin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FMulticastDelegateProperty"), SYSSTR("SignatureFunction"), -1); val != -1)
    Unreal::FMulticastDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FObjectPropertyBase"), SYSSTR("PropertyClass"), -1); val != -1)
    Unreal::FObjectPropertyBase::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArAllowLazyLoading"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArAllowLazyLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArContainsCode"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArContainsCode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArContainsMap"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArContainsMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArEngineVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArForceByteSwapping"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArForceByteSwapping"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArForceUnicode"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArForceUnicode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIgnoreArchetypeRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreArchetypeRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIgnoreClassGeneratedByRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreClassGeneratedByRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIgnoreClassRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreClassRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIgnoreOuterRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreOuterRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsCountingMemory"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsCountingMemory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsCriticalError"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsCriticalError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsError"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsFilterEditorOnly"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsFilterEditorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsLoading"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsLoadingFromCookedPackage"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsLoadingFromCookedPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsModifyingWeakAndStrongReferences"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsModifyingWeakAndStrongReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsNetArchive"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsNetArchive"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsObjectReferenceCollector"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsObjectReferenceCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsPersistent"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsPersistent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsSaveGame"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsSaveGame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsSaving"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsSaving"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsTextFormat"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsTextFormat"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArIsTransacting"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsTransacting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArLicenseeUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArNoDelta"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArNoDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArNoIntraPropertyDelta"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArNoIntraPropertyDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArPortFlags"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArRequiresLocalizationGather"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArRequiresLocalizationGather"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArShouldSkipBulkData"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipBulkData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArShouldSkipCompilingAssets"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipCompilingAssets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArUseCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUseCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArUseUnversionedPropertySerialization"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUseUnversionedPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("ArWantBinaryPropertySerialization"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArWantBinaryPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("SerializedProperty"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArchiveState"), SYSSTR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FField"), SYSSTR("ClassPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FField"), SYSSTR("FlagsPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("FlagsPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FField"), SYSSTR("NamePrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FField"), SYSSTR("Next"), -1); val != -1) Unreal::FField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FField"), SYSSTR("Owner"), -1); val != -1) Unreal::FField::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FClassProperty"), SYSSTR("MetaClass"), -1); val != -1)
    Unreal::FClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FBoolProperty"), SYSSTR("ByteMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FBoolProperty"), SYSSTR("ByteOffset"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FBoolProperty"), SYSSTR("FieldMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FBoolProperty"), SYSSTR("FieldSize"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UScriptStruct"), SYSSTR("CppStructOps"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("CppStructOps"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UScriptStruct"), SYSSTR("StructFlags"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("StructFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UScriptStruct"), SYSSTR("bCppStructOpsFromBaseClass"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bCppStructOpsFromBaseClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UScriptStruct"), SYSSTR("bPrepareCppStructOpsCompleted"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bPrepareCppStructOpsCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("ActiveLevelCollectionIndex"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ActiveLevelCollectionIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("AudioTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("BlockTillLevelStreamingCompletedEpoch"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BlockTillLevelStreamingCompletedEpoch"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("BuildStreamingDataTimer"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BuildStreamingDataTimer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("CachedViewInfoRenderedLastFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CachedViewInfoRenderedLastFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("CleanupWorldTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CleanupWorldTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("CommittedPersistentLevelName"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CommittedPersistentLevelName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("ContentBundleManager"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ContentBundleManager"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("DebugDrawTraceTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DebugDrawTraceTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("DeltaRealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaRealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("DeltaTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("ExtraReferencedObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ExtraReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("FullPurgeTriggered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("FullPurgeTriggered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("IsInBlockTillLevelStreamingCompleted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("IsInBlockTillLevelStreamingCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("LWILastAssignedUID"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LWILastAssignedUID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("LastRenderTime"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("LastTimeUnbuiltLightingWasEncountered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastTimeUnbuiltLightingWasEncountered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("LevelSequenceActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LevelSequenceActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NextSwitchCountdown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextSwitchCountdown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NextURL"), -1); val != -1) Unreal::UWorld::MemberOffsets.emplace(STR("NextURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NumInvalidReflectionCaptureComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumInvalidReflectionCaptureComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NumLightingUnbuiltObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumLightingUnbuiltObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NumStreamingLevelsBeingLoaded"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumStreamingLevelsBeingLoaded"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NumTextureStreamingDirtyResources"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingDirtyResources"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NumTextureStreamingUnbuiltComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingUnbuiltComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("NumUnbuiltReflectionCaptures"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumUnbuiltReflectionCaptures"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("OnWorldPartitionInitializedEvent"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OnWorldPartitionInitializedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("OnWorldPartitionUninitializedEvent"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OnWorldPartitionUninitializedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("OriginLocation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OriginLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("OriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("PauseDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PauseDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("PerModuleDataObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PerModuleDataObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("PlayerNum"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PlayerNum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("PreparingLevelNames"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PreparingLevelNames"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("RealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("RequestedOriginLocation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RequestedOriginLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("ServerStreamingLevelsVisibility"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ServerStreamingLevelsVisibility"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("StreamingLevelsPrefix"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingLevelsPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("StreamingVolumeUpdateDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingVolumeUpdateDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("TimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("TimeSinceLastPendingKillPurge"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSinceLastPendingKillPurge"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("UnpausedTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("UnpausedTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("ViewLocationsRenderedLastFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ViewLocationsRenderedLastFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bActorsInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bActorsInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bAggressiveLOD"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAggressiveLOD"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bAllowAudioPlayback"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowAudioPlayback"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bAllowDeferredPhysicsStateCreation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowDeferredPhysicsStateCreation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bAreConstraintsDirty"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAreConstraintsDirty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bBegunPlay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bBegunPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bCleanedUpWorld"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCleanedUpWorld"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bCreateRenderStateForHiddenComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCreateRenderStateForHiddenComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bDebugDrawAllTraceTags"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugDrawAllTraceTags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bDebugFrameStepExecution"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugFrameStepExecution"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bDebugPauseExecution"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugPauseExecution"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bDoDelayedUpdateCullDistanceVolumes"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDoDelayedUpdateCullDistanceVolumes"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bDropDetail"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDropDetail"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bHack_Force_UsesGameHiddenFlags_True"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHack_Force_UsesGameHiddenFlags_True"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bHasEverBeenInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHasEverBeenInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bInTick"), -1); val != -1) Unreal::UWorld::MemberOffsets.emplace(STR("bInTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bInitializedAndNeedsCleanup"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bInitializedAndNeedsCleanup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsBuilt"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsBuilt"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsCameraMoveableWhenPaused"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsCameraMoveableWhenPaused"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsDefaultLevel"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsDefaultLevel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsLevelStreamingFrozen"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsLevelStreamingFrozen"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsRunningConstructionScript"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsRunningConstructionScript"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsTearingDown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsTearingDown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bIsWorldInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsWorldInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bKismetScriptError"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bKismetScriptError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bMarkedObjectsPendingKill"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMarkedObjectsPendingKill"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bMatchStarted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMatchStarted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bOriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bOriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bPlayersOnly"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPlayersOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bPlayersOnlyPending"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPlayersOnlyPending"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bPostTickComponentUpdate"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPostTickComponentUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bRequestedBlockOnAsyncLoading"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bRequestedBlockOnAsyncLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bRequiresHitProxies"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bRequiresHitProxies"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bShouldDelayGarbageCollect"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldDelayGarbageCollect"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bShouldForceUnloadStreamingLevels"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldForceUnloadStreamingLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bShouldForceVisibleStreamingLevels"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldForceVisibleStreamingLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bShouldSimulatePhysics"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldSimulatePhysics"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bShouldTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bStartup"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bStartup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bStreamingDataDirty"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bStreamingDataDirty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bSupportsMakingInvisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingInvisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bSupportsMakingVisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingVisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bTickNewlySpawned"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bTickNewlySpawned"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bTriggerPostLoadMap"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bTriggerPostLoadMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UWorld"), SYSSTR("bWorldWasLoadedThisTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bWorldWasLoadedThisTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FSetProperty"), SYSSTR("ElementProp"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("ElementProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassAddReferencedObjects"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassAddReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassCastFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassCastFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassConfigName"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConfigName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassConstructor"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConstructor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassDefaultObject"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassDefaultObject"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassGeneratedBy"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassGeneratedBy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassUnique"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassUnique"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassVTableHelperCtorCaller"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassVTableHelperCtorCaller"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("ClassWithin"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassWithin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("CppClassStaticFunctions"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("CppClassStaticFunctions"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("FirstOwnedClassRep"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FirstOwnedClassRep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("Interfaces"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("Interfaces"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("NetFields"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("NetFields"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("SparseClassData"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("SparseClassDataStruct"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassDataStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("UberGraphFramePointerProperty"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("UberGraphFramePointerProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("bCooked"), -1); val != -1) Unreal::UClass::MemberOffsets.emplace(STR("bCooked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UClass"), SYSSTR("bLayoutChanging"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bLayoutChanging"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UEnum"), SYSSTR("CppForm"), -1); val != -1) Unreal::UEnum::MemberOffsets.emplace(STR("CppForm"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UEnum"), SYSSTR("CppType"), -1); val != -1) Unreal::UEnum::MemberOffsets.emplace(STR("CppType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UEnum"), SYSSTR("EnumDisplayNameFn"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumDisplayNameFn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UEnum"), SYSSTR("EnumFlags_Internal"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UEnum"), SYSSTR("EnumPackage"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("UEnum"), SYSSTR("Names"), -1); val != -1) Unreal::UEnum::MemberOffsets.emplace(STR("Names"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FMapProperty"), SYSSTR("KeyProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("KeyProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FMapProperty"), SYSSTR("ValueProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("ValueProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FStructProperty"), SYSSTR("Struct"), -1); val != -1)
    Unreal::FStructProperty::MemberOffsets.emplace(STR("Struct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FArrayProperty"), SYSSTR("Inner"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("Inner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FInterfaceProperty"), SYSSTR("InterfaceClass"), -1); val != -1)
    Unreal::FInterfaceProperty::MemberOffsets.emplace(STR("InterfaceClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(SYSSTR("FFieldPathProperty"), SYSSTR("PropertyClass"), -1); val != -1)
    Unreal::FFieldPathProperty::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
