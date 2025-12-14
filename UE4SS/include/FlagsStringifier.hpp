#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <Unreal/CoreUObject/UObject/Class.hpp>

namespace RC
{
    using namespace Unreal;

    struct ObjectFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "object_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "object_flags_menu";

        static auto get_raw_flags(UObject* object) -> uint32_t
        {
            return static_cast<uint32_t>(object->GetObjectFlags());
        }

        ObjectFlagsStringifier(UObject* object)
        {
            if (object->HasAnyFlags(RF_NoFlags))
            {
                flag_parts.emplace_back("RF_NoFlags");
            }
            if (object->HasAnyFlags(RF_Public))
            {
                flag_parts.emplace_back("RF_Public");
            }
            if (object->HasAnyFlags(RF_Standalone))
            {
                flag_parts.emplace_back("RF_Standalone");
            }
            if (object->HasAnyFlags(RF_MarkAsNative))
            {
                flag_parts.emplace_back("RF_MarkAsNative");
            }
            if (object->HasAnyFlags(RF_Transactional))
            {
                flag_parts.emplace_back("RF_Transactional");
            }
            if (object->HasAnyFlags(RF_ClassDefaultObject))
            {
                flag_parts.emplace_back("RF_ClassDefaultObject");
            }
            if (object->HasAnyFlags(RF_ArchetypeObject))
            {
                flag_parts.emplace_back("RF_ArchetypeObject");
            }
            if (object->HasAnyFlags(RF_Transient))
            {
                flag_parts.emplace_back("RF_Transient");
            }
            if (object->HasAnyFlags(RF_MarkAsRootSet))
            {
                flag_parts.emplace_back("RF_MarkAsRootSet");
            }
            if (object->HasAnyFlags(RF_TagGarbageTemp))
            {
                flag_parts.emplace_back("RF_TagGarbageTemp");
            }
            if (object->HasAnyFlags(RF_NeedInitialization))
            {
                flag_parts.emplace_back("RF_NeedInitialization");
            }
            if (object->HasAnyFlags(RF_NeedLoad))
            {
                flag_parts.emplace_back("RF_NeedLoad");
            }
            if (object->HasAnyFlags(RF_KeepForCooker))
            {
                flag_parts.emplace_back("RF_KeepForCooker");
            }
            if (object->HasAnyFlags(RF_NeedPostLoad))
            {
                flag_parts.emplace_back("RF_NeedPostLoad");
            }
            if (object->HasAnyFlags(RF_NeedPostLoadSubobjects))
            {
                flag_parts.emplace_back("RF_NeedPostLoadSubobjects");
            }
            if (object->HasAnyFlags(RF_NewerVersionExists))
            {
                flag_parts.emplace_back("RF_NewerVersionExists");
            }
            if (object->HasAnyFlags(RF_BeginDestroyed))
            {
                flag_parts.emplace_back("RF_BeginDestroyed");
            }
            if (object->HasAnyFlags(RF_FinishDestroyed))
            {
                flag_parts.emplace_back("RF_FinishDestroyed");
            }
            if (object->HasAnyFlags(RF_BeingRegenerated))
            {
                flag_parts.emplace_back("RF_BeingRegenerated");
            }
            if (object->HasAnyFlags(RF_DefaultSubObject))
            {
                flag_parts.emplace_back("RF_DefaultSubObject");
            }
            if (object->HasAnyFlags(RF_WasLoaded))
            {
                flag_parts.emplace_back("RF_WasLoaded");
            }
            if (object->HasAnyFlags(RF_TextExportTransient))
            {
                flag_parts.emplace_back("RF_TextExportTransient");
            }
            if (object->HasAnyFlags(RF_LoadCompleted))
            {
                flag_parts.emplace_back("RF_LoadCompleted");
            }
            if (object->HasAnyFlags(RF_InheritableComponentTemplate))
            {
                flag_parts.emplace_back("RF_InheritableComponentTemplate");
            }
            if (object->HasAnyFlags(RF_DuplicateTransient))
            {
                flag_parts.emplace_back("RF_DuplicateTransient");
            }
            if (object->HasAnyFlags(RF_StrongRefOnFrame))
            {
                flag_parts.emplace_back("RF_StrongRefOnFrame");
            }
            if (object->HasAnyFlags(RF_NonPIEDuplicateTransient))
            {
                flag_parts.emplace_back("RF_NonPIEDuplicateTransient");
            }
            if (object->HasAnyFlags(RF_Dynamic))
            {
                flag_parts.emplace_back("RF_Dynamic");
            }
            if (object->HasAnyFlags(RF_WillBeLoaded))
            {
                flag_parts.emplace_back("RF_WillBeLoaded");
            }
            if (object->HasAnyFlags(RF_HasExternalPackage))
            {
                flag_parts.emplace_back("RF_HasExternalPackage");
            }
            if (object->HasAnyFlags(RF_AllFlags))
            {
                flag_parts.emplace_back("RF_AllFlags");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    struct ClassFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "class_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "class_flags_menu";

        static auto get_raw_flags(UClass* object) -> uint32_t
        {
            return static_cast<uint32_t>(object->GetClassFlags());
        }

        ClassFlagsStringifier(UClass* uclass)
        {
            if (get_raw_flags(uclass) == 0)
            {
                flag_parts.emplace_back("CLASS_None");
            }
            if (uclass->HasAnyClassFlags(CLASS_Abstract))
            {
                flag_parts.emplace_back("CLASS_Abstract");
            }
            if (uclass->HasAnyClassFlags(CLASS_DefaultConfig))
            {
                flag_parts.emplace_back("CLASS_DefaultConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_Config))
            {
                flag_parts.emplace_back("CLASS_Config");
            }
            if (uclass->HasAnyClassFlags(CLASS_Transient))
            {
                flag_parts.emplace_back("CLASS_Transient");
            }
            if (uclass->HasAnyClassFlags(CLASS_Parsed))
            {
                flag_parts.emplace_back("CLASS_Parsed");
            }
            if (uclass->HasAnyClassFlags(CLASS_MatchedSerializers))
            {
                flag_parts.emplace_back("CLASS_MatchedSerializers");
            }
            if (uclass->HasAnyClassFlags(CLASS_ProjectUserConfig))
            {
                flag_parts.emplace_back("CLASS_ProjectUserConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_Native))
            {
                flag_parts.emplace_back("CLASS_Native");
            }
            if (uclass->HasAnyClassFlags(CLASS_NoExport))
            {
                flag_parts.emplace_back("CLASS_NoExport");
            }
            if (uclass->HasAnyClassFlags(CLASS_NotPlaceable))
            {
                flag_parts.emplace_back("CLASS_NotPlaceable");
            }
            if (uclass->HasAnyClassFlags(CLASS_PerObjectConfig))
            {
                flag_parts.emplace_back("CLASS_PerObjectConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_ReplicationDataIsSetUp))
            {
                flag_parts.emplace_back("CLASS_ReplicationDataIsSetUp");
            }
            if (uclass->HasAnyClassFlags(CLASS_EditInlineNew))
            {
                flag_parts.emplace_back("CLASS_EditInlineNew");
            }
            if (uclass->HasAnyClassFlags(CLASS_CollapseCategories))
            {
                flag_parts.emplace_back("CLASS_CollapseCategories");
            }
            if (uclass->HasAnyClassFlags(CLASS_Interface))
            {
                flag_parts.emplace_back("CLASS_Interface");
            }
            if (uclass->HasAnyClassFlags(CLASS_CustomConstructor))
            {
                flag_parts.emplace_back("CLASS_CustomConstructor");
            }
            if (uclass->HasAnyClassFlags(CLASS_Const))
            {
                flag_parts.emplace_back("CLASS_Const");
            }
            if (uclass->HasAnyClassFlags(CLASS_LayoutChanging))
            {
                flag_parts.emplace_back("CLASS_LayoutChanging");
            }
            if (uclass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
            {
                flag_parts.emplace_back("CLASS_CompiledFromBlueprint");
            }
            if (uclass->HasAnyClassFlags(CLASS_MinimalAPI))
            {
                flag_parts.emplace_back("CLASS_MinimalAPI");
            }
            if (uclass->HasAnyClassFlags(CLASS_RequiredAPI))
            {
                flag_parts.emplace_back("CLASS_RequiredAPI");
            }
            if (uclass->HasAnyClassFlags(CLASS_DefaultToInstanced))
            {
                flag_parts.emplace_back("CLASS_DefaultToInstanced");
            }
            if (uclass->HasAnyClassFlags(CLASS_TokenStreamAssembled))
            {
                flag_parts.emplace_back("CLASS_TokenStreamAssembled");
            }
            if (uclass->HasAnyClassFlags(CLASS_HasInstancedReference))
            {
                flag_parts.emplace_back("CLASS_HasInstancedReference");
            }
            if (uclass->HasAnyClassFlags(CLASS_Hidden))
            {
                flag_parts.emplace_back("CLASS_Hidden");
            }
            if (uclass->HasAnyClassFlags(CLASS_Deprecated))
            {
                flag_parts.emplace_back("CLASS_Deprecated");
            }
            if (uclass->HasAnyClassFlags(CLASS_HideDropDown))
            {
                flag_parts.emplace_back("CLASS_HideDropDown");
            }
            if (uclass->HasAnyClassFlags(CLASS_GlobalUserConfig))
            {
                flag_parts.emplace_back("CLASS_GlobalUserConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_Intrinsic))
            {
                flag_parts.emplace_back("CLASS_Intrinsic");
            }
            if (uclass->HasAnyClassFlags(CLASS_Constructed))
            {
                flag_parts.emplace_back("CLASS_Constructed");
            }
            if (uclass->HasAnyClassFlags(CLASS_ConfigDoNotCheckDefaults))
            {
                flag_parts.emplace_back("CLASS_ConfigDoNotCheckDefaults");
            }
            if (uclass->HasAnyClassFlags(CLASS_NewerVersionExists))
            {
                flag_parts.emplace_back("CLASS_NewerVersionExists");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    struct ClassCastFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "class_cast_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "class_cast_flags_menu";

        static auto get_raw_flags(UClass* object) -> uint64_t
        {
            return static_cast<uint64_t>(object->GetClassCastFlags());
        }

        ClassCastFlagsStringifier(UClass* uclass)
        {
            if (get_raw_flags(uclass) == 0)
            {
                flag_parts.emplace_back("CASTCLASS_None");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UField))
            {
                flag_parts.emplace_back("CASTCLASS_UField");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FInt8Property))
            {
                flag_parts.emplace_back("CASTCLASS_FInt8Property");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UEnum))
            {
                flag_parts.emplace_back("CASTCLASS_UEnum");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UStruct))
            {
                flag_parts.emplace_back("CASTCLASS_UStruct");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UScriptStruct))
            {
                flag_parts.emplace_back("CASTCLASS_UScriptStruct");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UClass))
            {
                flag_parts.emplace_back("CASTCLASS_UClass");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FByteProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FByteProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FIntProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FIntProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FFloatProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FFloatProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FUInt64Property))
            {
                flag_parts.emplace_back("CASTCLASS_FUInt64Property");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FClassProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FClassProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FUInt32Property))
            {
                flag_parts.emplace_back("CASTCLASS_FUInt32Property");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FInterfaceProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FInterfaceProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FNameProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FNameProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FStrProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FStrProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FObjectProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FObjectProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FBoolProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FBoolProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FUInt16Property))
            {
                flag_parts.emplace_back("CASTCLASS_FUInt16Property");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UFunction))
            {
                flag_parts.emplace_back("CASTCLASS_UFunction");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FStructProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FStructProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FArrayProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FArrayProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FInt64Property))
            {
                flag_parts.emplace_back("CASTCLASS_FInt64Property");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FDelegateProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FDelegateProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FNumericProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FNumericProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FMulticastDelegateProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FMulticastDelegateProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FObjectPropertyBase))
            {
                flag_parts.emplace_back("CASTCLASS_FObjectPropertyBase");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FWeakObjectProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FWeakObjectProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FLazyObjectProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FLazyObjectProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FSoftObjectProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FSoftObjectProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FTextProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FTextProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FInt16Property))
            {
                flag_parts.emplace_back("CASTCLASS_FInt16Property");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FDoubleProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FDoubleProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FSoftClassProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FSoftClassProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UPackage))
            {
                flag_parts.emplace_back("CASTCLASS_UPackage");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_ULevel))
            {
                flag_parts.emplace_back("CASTCLASS_ULevel");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_AActor))
            {
                flag_parts.emplace_back("CASTCLASS_AActor");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_APlayerController))
            {
                flag_parts.emplace_back("CASTCLASS_APlayerController");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_APawn))
            {
                flag_parts.emplace_back("CASTCLASS_APawn");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_USceneComponent))
            {
                flag_parts.emplace_back("CASTCLASS_USceneComponent");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UPrimitiveComponent))
            {
                flag_parts.emplace_back("CASTCLASS_UPrimitiveComponent");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_USkinnedMeshComponent))
            {
                flag_parts.emplace_back("CASTCLASS_USkinnedMeshComponent");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_USkeletalMeshComponent))
            {
                flag_parts.emplace_back("CASTCLASS_USkeletalMeshComponent");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UBlueprint))
            {
                flag_parts.emplace_back("CASTCLASS_UBlueprint");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UDelegateFunction))
            {
                flag_parts.emplace_back("CASTCLASS_UDelegateFunction");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_UStaticMeshComponent))
            {
                flag_parts.emplace_back("CASTCLASS_UStaticMeshComponent");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FMapProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FMapProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FSetProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FSetProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FEnumProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FEnumProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_USparseDelegateFunction))
            {
                flag_parts.emplace_back("CASTCLASS_USparseDelegateFunction");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FMulticastInlineDelegateProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FMulticastInlineDelegateProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FMulticastSparseDelegateProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FMulticastSparseDelegateProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FFieldPathProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FFieldPathProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FObjectPtrProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FObjectPtrProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FClassPtrProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FClassPtrProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FLargeWorldCoordinatesRealProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FLargeWorldCoordinatesRealProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FOptionalProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FOptionalProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FVValueProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FVValueProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FVRestValueProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FVRestValueProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FUtf8StrProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FUtf8StrProperty");
            }
            if (uclass->HasAnyCastFlag(CASTCLASS_FAnsiStrProperty))
            {
                flag_parts.emplace_back("CASTCLASS_FAnsiStrProperty");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(flag_part);
            });
        }
    };

    struct FunctionFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "func_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "func_flags_menu";

        static auto get_raw_flags(UFunction* function) -> uint32_t
        {
            return static_cast<uint32_t>(function->GetFunctionFlags());
        }

        FunctionFlagsStringifier(UFunction* function)
        {
            if (get_raw_flags(function) == 0)
            {
                flag_parts.emplace_back("FUNC_None");
            }
            if (function->HasAnyFunctionFlags(FUNC_Final))
            {
                flag_parts.emplace_back("FUNC_Final");
            }
            if (function->HasAnyFunctionFlags(FUNC_RequiredAPI))
            {
                flag_parts.emplace_back("FUNC_RequiredAPI");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly))
            {
                flag_parts.emplace_back("FUNC_BlueprintAuthorityOnly");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintCosmetic))
            {
                flag_parts.emplace_back("FUNC_BlueprintCosmetic");
            }
            if (function->HasAnyFunctionFlags(FUNC_Net))
            {
                flag_parts.emplace_back("FUNC_Net");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetReliable))
            {
                flag_parts.emplace_back("FUNC_NetReliable");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetRequest))
            {
                flag_parts.emplace_back("FUNC_NetRequest");
            }
            if (function->HasAnyFunctionFlags(FUNC_Exec))
            {
                flag_parts.emplace_back("FUNC_Exec");
            }
            if (function->HasAnyFunctionFlags(FUNC_Native))
            {
                flag_parts.emplace_back("FUNC_Native");
            }
            if (function->HasAnyFunctionFlags(FUNC_Event))
            {
                flag_parts.emplace_back("FUNC_Event");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetResponse))
            {
                flag_parts.emplace_back("FUNC_NetResponse");
            }
            if (function->HasAnyFunctionFlags(FUNC_Static))
            {
                flag_parts.emplace_back("FUNC_Static");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetMulticast))
            {
                flag_parts.emplace_back("FUNC_NetMulticast");
            }
            if (function->HasAnyFunctionFlags(FUNC_UbergraphFunction))
            {
                flag_parts.emplace_back("FUNC_UbergraphFunction");
            }
            if (function->HasAnyFunctionFlags(FUNC_MulticastDelegate))
            {
                flag_parts.emplace_back("FUNC_MulticastDelegate");
            }
            if (function->HasAnyFunctionFlags(FUNC_Public))
            {
                flag_parts.emplace_back("FUNC_Public");
            }
            if (function->HasAnyFunctionFlags(FUNC_Private))
            {
                flag_parts.emplace_back("FUNC_Private");
            }
            if (function->HasAnyFunctionFlags(FUNC_Protected))
            {
                flag_parts.emplace_back("FUNC_Protected");
            }
            if (function->HasAnyFunctionFlags(FUNC_Delegate))
            {
                flag_parts.emplace_back("FUNC_Delegate");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetServer))
            {
                flag_parts.emplace_back("FUNC_NetServer");
            }
            if (function->HasAnyFunctionFlags(FUNC_HasOutParms))
            {
                flag_parts.emplace_back("FUNC_HasOutParms");
            }
            if (function->HasAnyFunctionFlags(FUNC_HasDefaults))
            {
                flag_parts.emplace_back("FUNC_HasDefaults");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetClient))
            {
                flag_parts.emplace_back("FUNC_NetClient");
            }
            if (function->HasAnyFunctionFlags(FUNC_DLLImport))
            {
                flag_parts.emplace_back("FUNC_DLLImport");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
            {
                flag_parts.emplace_back("FUNC_BlueprintCallable");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintEvent))
            {
                flag_parts.emplace_back("FUNC_BlueprintEvent");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintPure))
            {
                flag_parts.emplace_back("FUNC_BlueprintPure");
            }
            if (function->HasAnyFunctionFlags(FUNC_EditorOnly))
            {
                flag_parts.emplace_back("FUNC_EditorOnly");
            }
            if (function->HasAnyFunctionFlags(FUNC_Const))
            {
                flag_parts.emplace_back("FUNC_Const");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetValidate))
            {
                flag_parts.emplace_back("FUNC_NetValidate");
            }
            if (function->HasAnyFunctionFlags(FUNC_AllFlags))
            {
                flag_parts.emplace_back("FUNC_AllFlags");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    struct PropertyFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        PropertyFlagsStringifier(EPropertyFlags flags)
        {
            if ((flags & CPF_Edit) != 0)
            {
                flag_parts.emplace_back("CPF_Edit");
            }
            if ((flags & CPF_ConstParm) != 0)
            {
                flag_parts.emplace_back("CPF_ConstParm");
            }
            if ((flags & CPF_BlueprintVisible) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintVisible");
            }
            if ((flags & CPF_ExportObject) != 0)
            {
                flag_parts.emplace_back("CPF_ExportObject");
            }
            if ((flags & CPF_BlueprintReadOnly) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintReadOnly");
            }
            if ((flags & CPF_Net) != 0)
            {
                flag_parts.emplace_back("CPF_Net");
            }
            if ((flags & CPF_EditFixedSize) != 0)
            {
                flag_parts.emplace_back("CPF_EditFixedSize");
            }
            if ((flags & CPF_Parm) != 0)
            {
                flag_parts.emplace_back("CPF_Parm");
            }
            if ((flags & CPF_OutParm) != 0)
            {
                flag_parts.emplace_back("CPF_OutParm");
            }
            if ((flags & CPF_ZeroConstructor) != 0)
            {
                flag_parts.emplace_back("CPF_ZeroConstructor");
            }
            if ((flags & CPF_ReturnParm) != 0)
            {
                flag_parts.emplace_back("CPF_ReturnParm");
            }
            if ((flags & CPF_DisableEditOnTemplate) != 0)
            {
                flag_parts.emplace_back("CPF_DisableEditOnTemplate");
            }
            if ((flags & CPF_Transient) != 0)
            {
                flag_parts.emplace_back("CPF_Transient");
            }
            if ((flags & CPF_Config) != 0)
            {
                flag_parts.emplace_back("CPF_Config");
            }
            if ((flags & CPF_DisableEditOnInstance) != 0)
            {
                flag_parts.emplace_back("CPF_DisableEditOnInstance");
            }
            if ((flags & CPF_EditConst) != 0)
            {
                flag_parts.emplace_back("CPF_EditConst");
            }
            if ((flags & CPF_GlobalConfig) != 0)
            {
                flag_parts.emplace_back("CPF_GlobalConfig");
            }
            if ((flags & CPF_InstancedReference) != 0)
            {
                flag_parts.emplace_back("CPF_InstancedReference");
            }
            if ((flags & CPF_DuplicateTransient) != 0)
            {
                flag_parts.emplace_back("CPF_DuplicateTransient");
            }
            if ((flags & CPF_SubobjectReference) != 0)
            {
                flag_parts.emplace_back("CPF_SubobjectReference");
            }
            if ((flags & CPF_SaveGame) != 0)
            {
                flag_parts.emplace_back("CPF_SaveGame");
            }
            if ((flags & CPF_NoClear) != 0)
            {
                flag_parts.emplace_back("CPF_NoClear");
            }
            if ((flags & CPF_ReferenceParm) != 0)
            {
                flag_parts.emplace_back("CPF_ReferenceParm");
            }
            if ((flags & CPF_BlueprintAssignable) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintAssignable");
            }
            if ((flags & CPF_Deprecated) != 0)
            {
                flag_parts.emplace_back("CPF_Deprecated");
            }
            if ((flags & CPF_IsPlainOldData) != 0)
            {
                flag_parts.emplace_back("CPF_IsPlainOldData");
            }
            if ((flags & CPF_RepSkip) != 0)
            {
                flag_parts.emplace_back("CPF_RepSkip");
            }
            if ((flags & CPF_RepNotify) != 0)
            {
                flag_parts.emplace_back("CPF_RepNotify");
            }
            if ((flags & CPF_Interp) != 0)
            {
                flag_parts.emplace_back("CPF_Interp");
            }
            if ((flags & CPF_NonTransactional) != 0)
            {
                flag_parts.emplace_back("CPF_NonTransactional");
            }
            if ((flags & CPF_EditorOnly) != 0)
            {
                flag_parts.emplace_back("CPF_EditorOnly");
            }
            if ((flags & CPF_NoDestructor) != 0)
            {
                flag_parts.emplace_back("CPF_NoDestructor");
            }
            if ((flags & CPF_AutoWeak) != 0)
            {
                flag_parts.emplace_back("CPF_AutoWeak");
            }
            if ((flags & CPF_ContainsInstancedReference) != 0)
            {
                flag_parts.emplace_back("CPF_ContainsInstancedReference");
            }
            if ((flags & CPF_AssetRegistrySearchable) != 0)
            {
                flag_parts.emplace_back("CPF_AssetRegistrySearchable");
            }
            if ((flags & CPF_SimpleDisplay) != 0)
            {
                flag_parts.emplace_back("CPF_SimpleDisplay");
            }
            if ((flags & CPF_AdvancedDisplay) != 0)
            {
                flag_parts.emplace_back("CPF_AdvancedDisplay");
            }
            if ((flags & CPF_Protected) != 0)
            {
                flag_parts.emplace_back("CPF_Protected");
            }
            if ((flags & CPF_BlueprintCallable) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintCallable");
            }
            if ((flags & CPF_BlueprintAuthorityOnly) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintAuthorityOnly");
            }
            if ((flags & CPF_TextExportTransient) != 0)
            {
                flag_parts.emplace_back("CPF_TextExportTransient");
            }
            if ((flags & CPF_NonPIEDuplicateTransient) != 0)
            {
                flag_parts.emplace_back("CPF_NonPIEDuplicateTransient");
            }
            if ((flags & CPF_ExposeOnSpawn) != 0)
            {
                flag_parts.emplace_back("CPF_ExposeOnSpawn");
            }
            if ((flags & CPF_PersistentInstance) != 0)
            {
                flag_parts.emplace_back("CPF_PersistentInstance");
            }
            if ((flags & CPF_UObjectWrapper) != 0)
            {
                flag_parts.emplace_back("CPF_UObjectWrapper");
            }
            if ((flags & CPF_HasGetValueTypeHash) != 0)
            {
                flag_parts.emplace_back("CPF_HasGetValueTypeHash");
            }
            if ((flags & CPF_NativeAccessSpecifierPublic) != 0)
            {
                flag_parts.emplace_back("CPF_NativeAccessSpecifierPublic");
            }
            if ((flags & CPF_NativeAccessSpecifierProtected) != 0)
            {
                flag_parts.emplace_back("CPF_NativeAccessSpecifierProtected");
            }
            if ((flags & CPF_NativeAccessSpecifierPrivate) != 0)
            {
                flag_parts.emplace_back("CPF_NativeAccessSpecifierPrivate");
            }
            if ((flags & CPF_SkipSerialization) != 0)
            {
                flag_parts.emplace_back("CPF_SkipSerialization");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };
} // namespace RC