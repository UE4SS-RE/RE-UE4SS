auto ParseMemberOffset = [](const File::StringType& val_str, auto& offsets, auto& bitfields, const File::StringType& member_name)
{
    try {
        int32_t offset = std::stoi(val_str, nullptr, 0);
        offsets.emplace(member_name, offset);
        size_t colon1 = val_str.find(':');
        if (colon1 != File::StringType::npos && colon1 + 1 < val_str.size()) {
            uint8_t bit_pos = static_cast<uint8_t>(std::stoi(val_str.substr(colon1 + 1)));
            uint8_t bit_len = 1;
            uint8_t storage_size = 1;
            size_t colon2 = val_str.find(':', colon1 + 1);
            if (colon2 != File::StringType::npos && colon2 + 1 < val_str.size()) {
                bit_len = static_cast<uint8_t>(std::stoi(val_str.substr(colon2 + 1)));
                size_t colon3 = val_str.find(':', colon2 + 1);
                if (colon3 != File::StringType::npos && colon3 + 1 < val_str.size()) {
                    storage_size = static_cast<uint8_t>(std::stoi(val_str.substr(colon3 + 1)));
                }
            }
            bitfields.emplace(member_name, Unreal::BitfieldInfo{bit_pos, bit_len, storage_size});
        }
    } catch (const std::exception&) {}
};

if (auto val = parser.get_int64(STR("UObjectBase"), STR("ObjectFlags"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ObjectFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("InternalIndex"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("InternalIndex_Private"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("InternalIndex_Private"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("InternalIndex_Private"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Class"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("ClassPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Name"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("NamePrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Outer"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("OuterPrivate"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("OuterPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("OuterPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("Size"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Size"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("Alignment"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Alignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("FakeVPtr"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("FakeVPtr"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("bSuppressEventTag"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bSuppressEventTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("bAutoEmitLineTerminator"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bAutoEmitLineTerminator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("SuperStruct"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("SuperStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("Children"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Children"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PropertiesSize"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertiesSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("MinAlignment"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("MinAlignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("Script"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Script"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PropertyLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertyLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("RefLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("RefLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("DestructorLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("DestructorLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PostConstructLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PostConstructLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ScriptObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ChildProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ChildProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ScriptAndPropertyObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptAndPropertyObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("UnresolvedScriptProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("UnresolvedScriptProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("StructStateFlags"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("StructStateFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportConsole"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportConsole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("DebugProperties"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("DebugProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("TitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("TitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("SplitscreenInfo"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("SplitscreenInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MaxSplitscreenPlayers"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MaxSplitscreenPlayers"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UGameViewportClient"), STR("bShowTitleSafeZone"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UGameViewportClient::MemberOffsets, Unreal::UGameViewportClient::BitfieldInfos, STR("bShowTitleSafeZone"));
if (auto val_str = parser.get_string(STR("UGameViewportClient"), STR("bIsPlayInEditorViewport"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UGameViewportClient::MemberOffsets, Unreal::UGameViewportClient::BitfieldInfos, STR("bIsPlayInEditorViewport"));
if (auto val_str = parser.get_string(STR("UGameViewportClient"), STR("bDisableWorldRendering"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UGameViewportClient::MemberOffsets, Unreal::UGameViewportClient::BitfieldInfos, STR("bDisableWorldRendering"));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ActiveSplitscreenType"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ActiveSplitscreenType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("World"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("World"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bSuppressTransitionMessage"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bSuppressTransitionMessage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewModeIndex"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewModeIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("EngineShowFlags"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("EngineShowFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("Viewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Viewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportFrame"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("Window"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Window"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportOverlayWidget"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportOverlayWidget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("GameLayerManagerPtr"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("GameLayerManagerPtr"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentBufferVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentBufferVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HighResScreenshotDialog"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HighResScreenshotDialog"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CursorWidgets"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CursorWidgets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("StatUnitData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatUnitData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("StatHitchesData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatHitchesData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bDisableSplitScreenOverride"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableSplitScreenOverride"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIgnoreInput"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIgnoreInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MouseCaptureMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseCaptureMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bHideCursorDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHideCursorDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bHasAudioFocus"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHasAudioFocus"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bLockDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bLockDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MouseLockMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseLockMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bUseSoftwareCursorWidgets"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bUseSoftwareCursorWidgets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HardwareCursorCache"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HardwareCursorCache"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HardwareCursors"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HardwareCursors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIsMouseOverClient"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsMouseOverClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentNaniteVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentNaniteVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentLumenVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentLumenVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentVirtualShadowMapVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentVirtualShadowMapVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentStrataVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentStrataVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentGroomVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentGroomVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentSubstrateVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentSubstrateVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArEngineVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsLoading"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsLoading"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsSaving"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsSaving"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsTransacting"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsTransacting"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArWantBinaryPropertySerialization"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArWantBinaryPropertySerialization"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArForceUnicode"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArForceUnicode"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsPersistent"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsPersistent"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsError"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsError"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsCriticalError"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsCriticalError"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArContainsCode"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArContainsCode"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArContainsMap"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArContainsMap"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArRequiresLocalizationGather"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArRequiresLocalizationGather"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArForceByteSwapping"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArForceByteSwapping"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIgnoreArchetypeRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIgnoreArchetypeRef"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArNoDelta"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArNoDelta"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIgnoreOuterRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIgnoreOuterRef"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIgnoreClassGeneratedByRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIgnoreClassGeneratedByRef"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIgnoreClassRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIgnoreClassRef"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArAllowLazyLoading"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArAllowLazyLoading"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsObjectReferenceCollector"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsObjectReferenceCollector"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsModifyingWeakAndStrongReferences"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsModifyingWeakAndStrongReferences"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsCountingMemory"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsCountingMemory"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArShouldSkipBulkData"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArShouldSkipBulkData"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsFilterEditorOnly"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsFilterEditorOnly"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsSaveGame"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsSaveGame"));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArPortFlags"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("SerializedProperty"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArUseCustomPropertyList"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArUseCustomPropertyList"));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsTextFormat"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsTextFormat"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArIsNetArchive"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArIsNetArchive"));
if (auto val_str = parser.get_string(STR("FArchive"), STR("ArNoIntraPropertyDelta"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchive::MemberOffsets, Unreal::FArchive::BitfieldInfos, STR("ArNoIntraPropertyDelta"));
if (auto val = parser.get_int64(STR("FArchive"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsLoading"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsLoading"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsSaving"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsSaving"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsTransacting"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsTransacting"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsTextFormat"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsTextFormat"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArWantBinaryPropertySerialization"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArWantBinaryPropertySerialization"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArUseUnversionedPropertySerialization"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArUseUnversionedPropertySerialization"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArForceUnicode"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArForceUnicode"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsPersistent"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsPersistent"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsError"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsError"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsCriticalError"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsCriticalError"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArContainsCode"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArContainsCode"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArContainsMap"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArContainsMap"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArRequiresLocalizationGather"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArRequiresLocalizationGather"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArForceByteSwapping"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArForceByteSwapping"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIgnoreArchetypeRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIgnoreArchetypeRef"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArNoDelta"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArNoDelta"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArNoIntraPropertyDelta"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArNoIntraPropertyDelta"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIgnoreOuterRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIgnoreOuterRef"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIgnoreClassGeneratedByRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIgnoreClassGeneratedByRef"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIgnoreClassRef"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIgnoreClassRef"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArAllowLazyLoading"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArAllowLazyLoading"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsObjectReferenceCollector"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsObjectReferenceCollector"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsModifyingWeakAndStrongReferences"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsModifyingWeakAndStrongReferences"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsCountingMemory"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsCountingMemory"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArShouldSkipBulkData"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArShouldSkipBulkData"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsFilterEditorOnly"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsFilterEditorOnly"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsSaveGame"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsSaveGame"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsNetArchive"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsNetArchive"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArUseCustomPropertyList"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArUseCustomPropertyList"));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArPortFlags"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArEngineVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("SerializedProperty"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("NextProxy"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("NextProxy"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsLoadingFromCookedPackage"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsLoadingFromCookedPackage"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArShouldSkipCompilingAssets"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArShouldSkipCompilingAssets"));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArLicenseeUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUEVer"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArShouldSkipUpdateCustomVersion"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArShouldSkipUpdateCustomVersion"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArMergeOverrides"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArMergeOverrides"));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("SavePackageData"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("SavePackageData"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArPreserveArrayElements"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArPreserveArrayElements"));
if (auto val_str = parser.get_string(STR("FArchiveState"), STR("ArIsSavingOptionalObject"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FArchiveState::MemberOffsets, Unreal::FArchiveState::BitfieldInfos, STR("ArIsSavingOptionalObject"));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("OptionsString"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameSessionClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSessionClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("HUDClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("SpectatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameSession"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AGameModeBase"), STR("bUseSeamlessTravel"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameModeBase::MemberOffsets, Unreal::AGameModeBase::BitfieldInfos, STR("bUseSeamlessTravel"));
if (auto val_str = parser.get_string(STR("AGameModeBase"), STR("bStartPlayersAsSpectators"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameModeBase::MemberOffsets, Unreal::AGameModeBase::BitfieldInfos, STR("bStartPlayersAsSpectators"));
if (auto val_str = parser.get_string(STR("AGameModeBase"), STR("bPauseable"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameModeBase::MemberOffsets, Unreal::AGameModeBase::BitfieldInfos, STR("bPauseable"));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("ServerStatReplicatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("ServerStatReplicator"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameNetDriverReplicationSystem"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameNetDriverReplicationSystem"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MatchState"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MatchState"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AGameMode"), STR("bUseSeamlessTravel"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameMode::MemberOffsets, Unreal::AGameMode::BitfieldInfos, STR("bUseSeamlessTravel"));
if (auto val_str = parser.get_string(STR("AGameMode"), STR("bPauseable"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameMode::MemberOffsets, Unreal::AGameMode::BitfieldInfos, STR("bPauseable"));
if (auto val_str = parser.get_string(STR("AGameMode"), STR("bStartPlayersAsSpectators"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameMode::MemberOffsets, Unreal::AGameMode::BitfieldInfos, STR("bStartPlayersAsSpectators"));
if (auto val_str = parser.get_string(STR("AGameMode"), STR("bDelayedStart"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AGameMode::MemberOffsets, Unreal::AGameMode::BitfieldInfos, STR("bDelayedStart"));
if (auto val = parser.get_int64(STR("AGameMode"), STR("OptionsString"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("HUDClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumBots"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumBots"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MinRespawnDelay"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MinRespawnDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("GameSession"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumTravellingPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumTravellingPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("CurrentID"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("CurrentID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("EngineMessageClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("EngineMessageClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("SpectatorClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("InactivePlayerArray"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerArray"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("GameModeClassAliases"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameModeClassAliases"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("InactivePlayerStateLifeSpan"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerStateLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bHandleDedicatedServerReplays"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bHandleDedicatedServerReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MaxInactivePlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MaxInactivePlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("PrimaryActorTick"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("PrimaryActorTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CustomTimeDilation"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CustomTimeDilation"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bHidden"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bHidden"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bNetTemporary"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bNetTemporary"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bNetStartup"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bNetStartup"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bOnlyRelevantToOwner"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bOnlyRelevantToOwner"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bAlwaysRelevant"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bAlwaysRelevant"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bReplicateMovement"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bReplicateMovement"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bTearOff"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bTearOff"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bExchangedRoles"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bExchangedRoles"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bPendingNetUpdate"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bPendingNetUpdate"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bNetLoadOnClient"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bNetLoadOnClient"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bNetUseOwnerRelevancy"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bNetUseOwnerRelevancy"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bBlockInput"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bBlockInput"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bRunningUserConstructionScript"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bRunningUserConstructionScript"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bHasFinishedSpawning"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bHasFinishedSpawning"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorEnableCollision"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorEnableCollision"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bReplicates"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bReplicates"));
if (auto val = parser.get_int64(STR("AActor"), STR("RemoteRole"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RemoteRole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Owner"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("AttachmentReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AttachmentReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Role"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Role"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetDormancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDormancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("AutoReceiveInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AutoReceiveInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputConsumeOption_DEPRECATED"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputConsumeOption_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetCullDistanceSquared"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetCullDistanceSquared"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetTag"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("LastNetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastNetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetDriverName"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDriverName"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bAutoDestroyWhenFinished"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bAutoDestroyWhenFinished"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bCanBeDamaged"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bCanBeDamaged"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorIsBeingDestroyed"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorIsBeingDestroyed"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bCollideWhenPlacing"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bCollideWhenPlacing"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bFindCameraComponentWhenViewTarget"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bFindCameraComponentWhenViewTarget"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bRelevantForNetworkReplays"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bRelevantForNetworkReplays"));
if (auto val = parser.get_int64(STR("AActor"), STR("SpawnCollisionHandlingMethod"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("SpawnCollisionHandlingMethod"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CreationTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CreationTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Children"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Children"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RootComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RootComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ControllingMatineeActors"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ControllingMatineeActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InitialLifeSpan"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InitialLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("TimerHandle_LifeSpanExpired"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("TimerHandle_LifeSpanExpired"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bAllowReceiveTickEventOnDedicatedServer"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bAllowReceiveTickEventOnDedicatedServer"));
if (auto val = parser.get_int64(STR("AActor"), STR("Layers"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Layers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponentActor"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponentActor"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorInitialized"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorInitialized"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorHasBegunPlay"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorHasBegunPlay"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorSeamlessTraveled"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorSeamlessTraveled"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bIgnoresOriginShifting"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bIgnoresOriginShifting"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bEnableAutoLODGeneration"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bEnableAutoLODGeneration"));
if (auto val = parser.get_int64(STR("AActor"), STR("Tags"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Tags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("HiddenEditorViews"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("HiddenEditorViews"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakeAnyDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeAnyDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakePointDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakePointDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorBeginOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorBeginOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorEndOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorEndOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnBeginCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnBeginCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnEndCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnClicked"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnClicked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnReleased"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnReleased"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchBegin"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchBegin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchEnd"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnd"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchEnter"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnter"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchLeave"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchLeave"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorHit"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorHit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnEndPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("DetachFence"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DetachFence"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bAllowTickBeforeBeginPlay"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bAllowTickBeforeBeginPlay"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bTickFunctionsRegistered"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bTickFunctionsRegistered"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bNetCheckedInitialPhysicsState"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bNetCheckedInitialPhysicsState"));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponentActor_DEPRECATED"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponentActor_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponent"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("ActorHasBegunPlay"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("ActorHasBegunPlay"));
if (auto val = parser.get_int64(STR("AActor"), STR("MinNetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("MinNetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bGenerateOverlapEventsDuringLevelStreaming"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bGenerateOverlapEventsDuringLevelStreaming"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bHasDeferredComponentRegistration"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bHasDeferredComponentRegistration"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bCanBeInCluster"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bCanBeInCluster"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bReplayRewindable"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bReplayRewindable"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bIsEditorOnlyActor"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bIsEditorOnlyActor"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorWantsDestroyDuringBeginPlay"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorWantsDestroyDuringBeginPlay"));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakeRadialDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeRadialDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CachedLastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CachedLastRenderTime"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bRelevantForLevelBounds"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bRelevantForLevelBounds"));
if (auto val = parser.get_int64(STR("AActor"), STR("LastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorBeginningPlayFromLevelStreaming"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorBeginningPlayFromLevelStreaming"));
if (auto val = parser.get_int64(STR("AActor"), STR("UpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("UpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorIsBeingConstructed"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorIsBeingConstructed"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bForceNetAddressable"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bForceNetAddressable"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bCallPreReplication"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bCallPreReplication"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bCallPreReplicationForReplay"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bCallPreReplicationForReplay"));
if (auto val = parser.get_int64(STR("AActor"), STR("RayTracingGroupId"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RayTracingGroupId"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bReplicateUsingRegisteredSubObjectList"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bReplicateUsingRegisteredSubObjectList"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bHasRegisteredAllComponents"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bHasRegisteredAllComponents"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bAsyncPhysicsTickEnabled"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bAsyncPhysicsTickEnabled"));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedSubObjects"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedSubObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedComponentsInfo"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedComponentsInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("PhysicsReplicationMode"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("PhysicsReplicationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ActorCategory"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ActorCategory"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("AActor"), STR("bHasPreRegisteredAllComponents"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bHasPreRegisteredAllComponents"));
if (auto val_str = parser.get_string(STR("AActor"), STR("bActorIsPendingPostNetInit"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::AActor::MemberOffsets, Unreal::AActor::BitfieldInfos, STR("bActorIsPendingPostNetInit"));
if (auto val = parser.get_int64(STR("AActor"), STR("HLODLayer"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("HLODLayer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("CurrentNetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("CurrentNetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("ConfiguredInternetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredInternetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("ConfiguredLanSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredLanSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ViewportClient"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ViewportClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("LastViewLocation"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("LastViewLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("AspectRatioAxisConstraint"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("AspectRatioAxisConstraint"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("ULocalPlayer"), STR("bSentSplitJoin"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::ULocalPlayer::MemberOffsets, Unreal::ULocalPlayer::BitfieldInfos, STR("bSentSplitJoin"));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ControllerId"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ControllerId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("bEmulateSplitscreen"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("bEmulateSplitscreen"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ConnectionIdentifier"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ConnectionIdentifier"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("ClassPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Owner"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Next"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("NamePrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("FlagsPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("FlagsPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UField"), STR("Next"), -1); val != -1)
    Unreal::UField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UField"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UField::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("ArrayDim"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ArrayDim"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("ElementSize"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ElementSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PropertyFlags"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("RepIndex"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("RepNotifyFunc"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepNotifyFunc"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("Offset_Internal"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("Offset_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PropertyLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("NextRef"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("NextRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("DestructorLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("DestructorLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PostConstructLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PostConstructLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMulticastDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FMulticastDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMulticastDelegateProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FMulticastDelegateProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FObjectPropertyBase"), STR("PropertyClass"), -1); val != -1)
    Unreal::FObjectPropertyBase::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FObjectPropertyBase"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FObjectPropertyBase::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("ContextHandle"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("ContextHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("TravelURL"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("TravelURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("TravelType"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("TravelType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("LastURL"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("LastURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("LastRemoteURL"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("LastRemoteURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("LevelsToLoadForPendingMapChange"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("LevelsToLoadForPendingMapChange"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PendingMapChangeFailureDescription"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PendingMapChangeFailureDescription"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("FWorldContext"), STR("bShouldCommitPendingMapChange"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::FWorldContext::MemberOffsets, Unreal::FWorldContext::BitfieldInfos, STR("bShouldCommitPendingMapChange"));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("GameViewport"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("GameViewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEInstance"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEInstance"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEPrefix"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIERemapPrefix"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIERemapPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("RunAsDedicated"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("RunAsDedicated"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("bWaitingOnOnlineSubsystem"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("bWaitingOnOnlineSubsystem"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("ExternalReferences"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("ExternalReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("ThisCurrentWorld"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("ThisCurrentWorld"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("CustomDescription"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("CustomDescription"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("AudioDeviceID"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("AudioDeviceID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEFixedTickSeconds"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEFixedTickSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEAccumulatedTickSeconds"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEAccumulatedTickSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("bIsPrimaryPIEInstance"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("bIsPrimaryPIEInstance"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("GarbageObjectsToVerify"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("GarbageObjectsToVerify"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("StructFlags"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("StructFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("bCppStructOpsFromBaseClass"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bCppStructOpsFromBaseClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("bPrepareCppStructOpsCompleted"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bPrepareCppStructOpsCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("CppStructOps"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("CppStructOps"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ExtraReferencedObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ExtraReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PerModuleDataObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PerModuleDataObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("StreamingLevelsPrefix"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingLevelsPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ViewLocationsRenderedLastFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ViewLocationsRenderedLastFrame"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bWorldWasLoadedThisTick"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bWorldWasLoadedThisTick"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bTriggerPostLoadMap"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bTriggerPostLoadMap"));
if (auto val = parser.get_int64(STR("UWorld"), STR("AuthorityGameMode"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AuthorityGameMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NetworkActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NetworkActors"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bRequiresHitProxies"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bRequiresHitProxies"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bStreamingDataDirty"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bStreamingDataDirty"));
if (auto val = parser.get_int64(STR("UWorld"), STR("BuildStreamingDataTimer"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BuildStreamingDataTimer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("URL"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("URL"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bInTick"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bInTick"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsBuilt"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsBuilt"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bTickNewlySpawned"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bTickNewlySpawned"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bPostTickComponentUpdate"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bPostTickComponentUpdate"));
if (auto val = parser.get_int64(STR("UWorld"), STR("PlayerNum"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PlayerNum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("TimeSinceLastPendingKillPurge"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSinceLastPendingKillPurge"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("FullPurgeTriggered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("FullPurgeTriggered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldDelayGarbageCollect"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldDelayGarbageCollect"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsWorldInitialized"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsWorldInitialized"));
if (auto val = parser.get_int64(STR("UWorld"), STR("StreamingVolumeUpdateDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingVolumeUpdateDelay"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsLevelStreamingFrozen"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsLevelStreamingFrozen"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bShouldForceUnloadStreamingLevels"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bShouldForceUnloadStreamingLevels"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bShouldForceVisibleStreamingLevels"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bShouldForceVisibleStreamingLevels"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bDoDelayedUpdateCullDistanceVolumes"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bDoDelayedUpdateCullDistanceVolumes"));
if (auto val = parser.get_int64(STR("UWorld"), STR("bHack_Force_UsesGameHiddenFlags_True"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHack_Force_UsesGameHiddenFlags_True"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsRunningConstructionScript"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsRunningConstructionScript"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bShouldSimulatePhysics"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bShouldSimulatePhysics"));
if (auto val = parser.get_int64(STR("UWorld"), STR("DebugDrawTraceTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DebugDrawTraceTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LastTimeUnbuiltLightingWasEncountered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastTimeUnbuiltLightingWasEncountered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("TimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("RealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AudioTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeltaTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PauseDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PauseDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bOriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bOriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextURL"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextSwitchCountdown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextSwitchCountdown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PreparingLevelNames"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PreparingLevelNames"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CommittedPersistentLevelName"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CommittedPersistentLevelName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumLightingUnbuiltObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumLightingUnbuiltObjects"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bDropDetail"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bDropDetail"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bAggressiveLOD"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bAggressiveLOD"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsDefaultLevel"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsDefaultLevel"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bRequestedBlockOnAsyncLoading"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bRequestedBlockOnAsyncLoading"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bActorsInitialized"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bActorsInitialized"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bBegunPlay"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bBegunPlay"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bMatchStarted"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bMatchStarted"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bPlayersOnly"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bPlayersOnly"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bPlayersOnlyPending"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bPlayersOnlyPending"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bStartup"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bStartup"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsTearingDown"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsTearingDown"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bKismetScriptError"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bKismetScriptError"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bDebugPauseExecution"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bDebugPauseExecution"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bAllowAudioPlayback"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bAllowAudioPlayback"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bDebugFrameStepExecution"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bDebugFrameStepExecution"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bAreConstraintsDirty"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bAreConstraintsDirty"));
if (auto val = parser.get_int64(STR("UWorld"), STR("AsyncPreRegisterLevelStreamingTasks"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AsyncPreRegisterLevelStreamingTasks"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bCreateRenderStateForHiddenComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCreateRenderStateForHiddenComponents"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bShouldTick"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bShouldTick"));
if (auto val = parser.get_int64(STR("UWorld"), STR("UnpausedTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("UnpausedTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumInvalidReflectionCaptureComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumInvalidReflectionCaptureComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumTextureStreamingUnbuiltComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingUnbuiltComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumTextureStreamingDirtyResources"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingDirtyResources"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bIsCameraMoveableWhenPaused"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bIsCameraMoveableWhenPaused"));
if (auto val = parser.get_int64(STR("UWorld"), STR("OriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugDrawAllTraceTags"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugDrawAllTraceTags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ActiveLevelCollectionIndex"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ActiveLevelCollectionIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumUnbuiltReflectionCaptures"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumUnbuiltReflectionCaptures"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bCleanedUpWorld"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bCleanedUpWorld"));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bMarkedObjectsPendingKill"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bMarkedObjectsPendingKill"));
if (auto val = parser.get_int64(STR("UWorld"), STR("LevelSequenceActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LevelSequenceActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumStreamingLevelsBeingLoaded"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumStreamingLevelsBeingLoaded"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CleanupWorldTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CleanupWorldTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAllowDeferredPhysicsStateCreation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowDeferredPhysicsStateCreation"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bInitializedAndNeedsCleanup"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bInitializedAndNeedsCleanup"));
if (auto val = parser.get_int64(STR("UWorld"), STR("IsInBlockTillLevelStreamingCompleted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("IsInBlockTillLevelStreamingCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("BlockTillLevelStreamingCompletedEpoch"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BlockTillLevelStreamingCompletedEpoch"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LWILastAssignedUID"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LWILastAssignedUID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeltaRealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaRealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bSupportsMakingVisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingVisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bSupportsMakingInvisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingInvisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LastRenderTime"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UWorld"), STR("bHasEverBeenInitialized"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UWorld::MemberOffsets, Unreal::UWorld::BitfieldInfos, STR("bHasEverBeenInitialized"));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsBeingCleanedUp"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsBeingCleanedUp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PendingVisibilityLock"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PendingVisibilityLock"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PostProcessVolumeCachedSizes"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PostProcessVolumeCachedSizes"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("FunctionFlags"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FunctionFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RepOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RepOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("NumParms"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("NumParms"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("ParmsSize"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ParmsSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("ReturnValueOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ReturnValueOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RPCId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RPCResponseId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCResponseId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("FirstPropertyToInit"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FirstPropertyToInit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("EventGraphFunction"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("EventGraphCallOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphCallOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("Func"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("Func"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassConstructor"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConstructor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassVTableHelperCtorCaller"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassVTableHelperCtorCaller"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassAddReferencedObjects"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassAddReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassUnique"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassUnique"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassCastFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassCastFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassWithin"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassWithin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassGeneratedBy"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassGeneratedBy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassConfigName"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConfigName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bCooked"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bCooked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("NetFields"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("NetFields"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassDefaultObject"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassDefaultObject"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("Interfaces"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("Interfaces"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ReferenceTokenStream"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ReferenceTokenStream"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("NativeFunctionLookupTable"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("NativeFunctionLookupTable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ParentFuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ParentFuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("InterfaceFuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("InterfaceFuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SuperFuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SuperFuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("UberGraphFramePointerProperty"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("UberGraphFramePointerProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SparseClassData"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SparseClassDataStruct"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassDataStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FirstOwnedClassRep"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FirstOwnedClassRep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bLayoutChanging"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bLayoutChanging"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("AllFunctionsCache"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("AllFunctionsCache"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ReferenceSchema"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ReferenceSchema"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FuncMapLock"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FuncMapLock"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("AllFunctionsCacheLock"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("AllFunctionsCacheLock"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bNeedsDynamicSubobjectInstancing"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bNeedsDynamicSubobjectInstancing"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppType"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("CppType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("Names"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("Names"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppForm"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("CppForm"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumDisplayNameFn"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumDisplayNameFn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumFlags"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumFlags_Internal"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumPackage"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FStructProperty"), STR("Struct"), -1); val != -1)
    Unreal::FStructProperty::MemberOffsets.emplace(STR("Struct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FStructProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FStructProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("Inner"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("Inner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("ArrayFlags"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("ArrayFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("KeyProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("KeyProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("ValueProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("ValueProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("MapLayout"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("MapLayout"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("MapFlags"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("MapFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("FieldSize"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("ByteOffset"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("ByteMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("FieldMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FByteProperty"), STR("Enum"), -1); val != -1)
    Unreal::FByteProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FByteProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FByteProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("UnderlyingProp"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("UnderlyingProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("Enum"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FClassProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FClassProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSoftClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FSoftClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSoftClassProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FSoftClassProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FDelegateProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FDelegateProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FInterfaceProperty"), STR("InterfaceClass"), -1); val != -1)
    Unreal::FInterfaceProperty::MemberOffsets.emplace(STR("InterfaceClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FInterfaceProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FInterfaceProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldPathProperty"), STR("PropertyClass"), -1); val != -1)
    Unreal::FFieldPathProperty::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldPathProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FFieldPathProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSetProperty"), STR("ElementProp"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("ElementProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSetProperty"), STR("SetLayout"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("SetLayout"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSetProperty"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("RowStruct"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("RowStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("RowMap"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("RowMap"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UDataTable"), STR("bStripFromClientBuilds"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UDataTable::MemberOffsets, Unreal::UDataTable::BitfieldInfos, STR("bStripFromClientBuilds"));
if (auto val_str = parser.get_string(STR("UDataTable"), STR("bIgnoreExtraFields"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UDataTable::MemberOffsets, Unreal::UDataTable::BitfieldInfos, STR("bIgnoreExtraFields"));
if (auto val_str = parser.get_string(STR("UDataTable"), STR("bIgnoreMissingFields"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UDataTable::MemberOffsets, Unreal::UDataTable::BitfieldInfos, STR("bIgnoreMissingFields"));
if (auto val = parser.get_int64(STR("UDataTable"), STR("ImportKeyField"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("ImportKeyField"), static_cast<int32_t>(val));
if (auto val_str = parser.get_string(STR("UDataTable"), STR("bPreserveExistingValues"), {}); !val_str.empty())
    ParseMemberOffset(val_str, Unreal::UDataTable::MemberOffsets, Unreal::UDataTable::BitfieldInfos, STR("bPreserveExistingValues"));
if (auto val = parser.get_int64(STR("UDataTable"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("Object"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("Object"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("ClusterAndFlags"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("ClusterAndFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("SerialNumber"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("SerialNumber"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("Flags"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("Flags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("ClusterIndex"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("ClusterIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("ClusterRootIndex"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("ClusterRootIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("RefCount"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("RefCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("FlagsAndRefCount"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("FlagsAndRefCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("RemoteId"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("RemoteId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("ObjectPtrLow"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("ObjectPtrLow"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectItem"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FUObjectItem::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("ObjFirstGCIndex"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("ObjFirstGCIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("ObjLastNonGCIndex"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("ObjLastNonGCIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("OpenForDisregardForGC"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("OpenForDisregardForGC"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("ObjObjects"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("ObjObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("ObjAvailableList"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("ObjAvailableList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("UObjectCreateListeners"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("UObjectCreateListeners"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("UObjectDeleteListeners"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("UObjectDeleteListeners"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("MaxObjectsNotConsideredByGC"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("MaxObjectsNotConsideredByGC"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("MasterSerialNumber"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("MasterSerialNumber"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("PrimarySerialNumber"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("PrimarySerialNumber"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("bShouldRecycleObjectIndices"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("bShouldRecycleObjectIndices"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("ObjAvailableListEstimateCount"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("ObjAvailableListEstimateCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FUObjectArray"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FUObjectArray::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("Objects"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("Objects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("PreAllocatedObjects"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("PreAllocatedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("MaxElements"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("MaxElements"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("NumElements"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("NumElements"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("MaxChunks"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("MaxChunks"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("NumChunks"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("NumChunks"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("TUObjectArray"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::TUObjectArray::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("Name"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("FName"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("FFieldClass"), STR("FName"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("FName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("Id"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("Id"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("CastFlags"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("CastFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("ClassFlags"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("ClassFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("SuperClass"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("SuperClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("DefaultObject"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("DefaultObject"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("ConstructFn"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("ConstructFn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("UnqiueNameIndexCounter"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("UnqiueNameIndexCounter"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("UniqueNameIndexCounter"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("UniqueNameIndexCounter"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldClass"), STR("UEP_TotalSize"), -1); val != -1)
    Unreal::FFieldClass::MemberOffsets.emplace(STR("UEP_TotalSize"), static_cast<int32_t>(val));
