#pragma once

#include <cstdint>

#include <Unreal/Core/Containers/Array.hpp>
#include <Unreal/World.hpp>

namespace RC
{
    using namespace Unreal;

    struct MemoryItem;

    auto Test_GetUnsignedMemorySetup() -> MemoryItem*;
    auto Test_GetSignedMemorySetup() -> MemoryItem*;
    auto Test_GetPlayerControllerVTablePointer() -> MemoryItem*;

    auto GetWorldTest() -> UWorld*;
    auto GetArrayTest() -> Unreal::TArray<UObject*>&;
    auto GetArrayTest2() -> Unreal::TArray<int16_t>&;
    auto GetArrayTest3() -> Unreal::TArray<FName>;
    auto Test_Get_UObject_Nullptr() -> UObject*;
} // namespace RC
