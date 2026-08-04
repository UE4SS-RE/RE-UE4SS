#pragma once

// This file is intended to contain structs that we don't support yet in the Unreal module.
// These types are likely going to be unusable and only exist to get rid of compiler errors.
// This will might also serve as a list of unimplemented basic types that we should support.

#include <Common.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>
#include <Unreal/Property/FLazyObjectProperty.hpp>
#include <Unreal/Property/FInterfaceProperty.hpp>

namespace RC::Unreal
{
    struct RC_UE4SS_API FKey
    {
        FName KeyName{};

        FName& GetKeyName()
        {
            return KeyName;
        }
    };

    template <typename T>
    struct TScriptInterface : public FScriptInterface
    {
    };

    template <typename T>
    struct TLazyObjectPtr : public FLazyObjectPtr
    {
    };
} // namespace RC::Unreal
