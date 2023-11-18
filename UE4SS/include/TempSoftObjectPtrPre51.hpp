#pragma once

// This file is intended to contain TSoftObjectPtr<T> for <=5.0.
// It's a temporary file, and I don't know how we're going to deal with the 5.1 change in the Unreal module.

#include <Common.hpp>
#include <Unreal/FAssetData.hpp>

namespace RC::Unreal
{
    template <typename T>
    struct TSoftObjectPtr
    {
        FSoftObjectPtr SoftObjectPtr;
    };

    template <typename T>
    struct TSoftClassPtr
    {
        FSoftObjectPtr SoftObjectPtr;
    };
} // namespace RC::Unreal
