#pragma once

#pragma warning(disable : 4005)
#include <Unreal/AActor.hpp>
#include <Unreal/FField.hpp>
#include <Unreal/FFrame.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FDelegateProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FFieldPathProperty.hpp>
#include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
#include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FSetProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/Core/Containers/Array.hpp>
#include <Unreal/Core/Containers/Map.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UStruct.hpp>
#include <Unreal/UnrealFlags.hpp>
// #include <Unreal/CustomType.hpp>
#include <Unreal/FAssetData.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/UAssetRegistryHelpers.hpp>
#pragma warning(default : 4005)

namespace RC
{
    namespace UObjectGlobals = Unreal::UObjectGlobals;
    using EObjectFlags = Unreal::EObjectFlags;
    using UObjectBaseUtility = Unreal::UObjectBaseUtility;
    using UObject = Unreal::UObject;
    using AActor = Unreal::AActor;
    using UClass = Unreal::UClass;
    using UStruct = Unreal::UStruct;
    using UScriptStruct = Unreal::UScriptStruct;
    using FField = Unreal::FField;
    using FName = Unreal::FName;
    using FString = Unreal::FString;
    using FProperty = Unreal::FProperty;
    using FIntProperty = Unreal::FIntProperty;
    using FInt8Property = Unreal::FInt8Property;
    using FInt16Property = Unreal::FInt16Property;
    using FInt64Property = Unreal::FInt64Property;
    using FByteProperty = Unreal::FByteProperty;
    using FFloatProperty = Unreal::FFloatProperty;
    using FObjectProperty = Unreal::FObjectProperty;
    using FWeakObjectProperty = Unreal::FWeakObjectProperty;
    using FClassProperty = Unreal::FClassProperty;
    using FBoolProperty = Unreal::FBoolProperty;
    using FArrayProperty = Unreal::FArrayProperty;
    using FStructProperty = Unreal::FStructProperty;
    using FNameProperty = Unreal::FNameProperty;
    using FTextProperty = Unreal::FTextProperty;
    using FStrProperty = Unreal::FStrProperty;
    template <typename T>
    using TArray = Unreal::TArray<T>;
    using UFunction = Unreal::UFunction;
    template <typename T1, typename T2>
    using TMap = Unreal::TMap<T1, T2>;

    using UnrealScriptFunction = Unreal::UnrealScriptFunction;
    using FFrame = Unreal::FFrame;
    using UEnum = Unreal::UEnum;

    using FWeakObjectPtr = Unreal::FWeakObjectPtr;
    using FDelegateProperty = Unreal::FDelegateProperty;
    using FMulticastInlineDelegateProperty = Unreal::FMulticastInlineDelegateProperty;
    using FMulticastSparseDelegateProperty = Unreal::FMulticastSparseDelegateProperty;
    using FSetProperty = Unreal::FSetProperty;
    using FSoftClassProperty = Unreal::FSoftClassProperty;
    using FEnumProperty = Unreal::FEnumProperty;
    using FFieldPathProperty = Unreal::FFieldPathProperty;

    // using CustomArrayProperty = Unreal::CustomArrayProperty;
    // using CustomStructProperty = Unreal::CustomStructProperty;
    using FAssetData = Unreal::FAssetData;
    using UAssetRegistry = Unreal::UAssetRegistry;
    using UAssetRegistryHelpers = Unreal::UAssetRegistryHelpers;
} // namespace RC
