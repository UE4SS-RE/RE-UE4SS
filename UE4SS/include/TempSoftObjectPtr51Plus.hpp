#pragma once

// This file is intended to contain TSoftObjectPtr<T> for 5.1+.
// It's a temporary file, and I don't know how we're going to deal with the 5.1 change in the Unreal module.

#include <Common.hpp>
#include <Unreal/FAssetData.hpp>

namespace RC::Unreal
{
    struct FSoftObjectPath_As51Plus
    {
        FTopLevelAssetPath AssetPath{};
        FString SubPathString{};
    };

    struct FSoftObjectPtr_As51Plus : public TPersistentObjectPtr<FSoftObjectPath_As51Plus>
    {
    };

    template <typename T>
    struct TSoftObjectPtr
    {
        FSoftObjectPtr_As51Plus SoftObjectPtr;
    };

    template <typename T>
    struct TSoftClassPtr
    {
        FSoftObjectPtr_As51Plus SoftObjectPtr;
    };
} // namespace RC::Unreal
