#ifndef UE4SS_LUATESTS_HPP
#define UE4SS_LUATESTS_HPP

#include <cstdint>

#include <Unreal/TArray.hpp>
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
}

#endif //UE4SS_LUATESTS_HPP
