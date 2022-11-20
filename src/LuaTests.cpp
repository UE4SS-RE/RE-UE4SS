#include <LuaTests.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <LuaScriptMemoryAccess.hpp>
#include <Unreal/UObjectGlobals.hpp>

namespace RC
{
    auto Test_GetUnsignedMemorySetup() -> MemoryItem*
    {
        uint8_t* mem = static_cast<uint8_t*>(std::malloc(0x8));
        mem[0] = 1;
        mem[1] = 2;
        mem[2] = 3;
        mem[3] = 4;
        mem[4] = 5;
        mem[5] = 6;
        mem[6] = 7;
        mem[7] = 8;
        return std::bit_cast<MemoryItem*>(mem);
    }

    auto Test_GetSignedMemorySetup() -> MemoryItem*
    {
        int8_t* mem = static_cast<int8_t*>(std::malloc(0x8));
        mem[0] = 1;
        mem[1] = 2;
        mem[2] = 3;
        mem[3] = 4;
        mem[4] = 5;
        mem[5] = 6;
        mem[6] = 7;
        mem[7] = 8;
        return std::bit_cast<MemoryItem*>(mem);
    }

    auto Test_GetPlayerControllerVTablePointer() -> MemoryItem*
    {
        return std::bit_cast<MemoryItem*>(*std::bit_cast<void**>(Unreal::UObjectGlobals::FindFirstOf(STR("PlayerController"))));
    }

    auto GetWorldTest() -> UWorld*
    {
        auto world = static_cast<UWorld*>(UObjectGlobals::FindFirstOf(STR("World")));
        auto& net_fields = world->GetPreparingLevelNames();
        Output::send(STR("net_fields num: {}\n"), net_fields.Num());
        Output::send(STR("net_fields max: {}\n"), net_fields.Max());
        return world;
    }

    auto GetArrayTest() -> TArray<UObject*>&
    {
        auto objects = static_cast<TArray<UObject*>*>(FMemory::Malloc(sizeof(TArray<UObject*>)));
        new(objects) TArray<UObject*>{};
        objects->Add(UObjectGlobals::FindObject(nullptr, nullptr, STR("/Script/CoreUObject.Object")));
        objects->Add(UObjectGlobals::FindObject(nullptr, nullptr, STR("/Script/Engine.MaterialExpression")));
        objects->Add(UObjectGlobals::FindObject(nullptr, nullptr, STR("/Script/Engine.MaterialExpressionTextureBase")));
        objects->Add(UObjectGlobals::FindObject(nullptr, nullptr, STR("/Script/Engine.MaterialExpressionTextureSample")));
        objects->Add(UObjectGlobals::FindObject(nullptr, nullptr, STR("/Script/Engine.MaterialExpressionTextureSampleParameter")));
        return *objects;
    }

    auto GetArrayTest2() -> TArray<int16_t>&
    {
        auto ints = static_cast<TArray<int16_t>*>(FMemory::Malloc(sizeof(TArray<int16_t>)));
        new(ints) TArray<int16_t>{};
        ints->Add(1);
        ints->Add(2);
        ints->Add(3);
        ints->Add(4);
        ints->Add(5);
        return *ints;
    }

    auto GetArrayTest3() -> TArray<FName>
    {
        //auto names = static_cast<TArray<FName>*>(FMemory::Malloc(sizeof(TArray<FName>)));
        //new(names) TArray<FName>{};
        TArray<FName> names{};
        names.Add(FName(STR("HelloOne"), EFindName::FNAME_Add));
        names.Add(FName(STR("HelloTwo"), EFindName::FNAME_Add));
        names.Add(FName(STR("HelloThree"), EFindName::FNAME_Add));
        names.Add(FName(STR("HelloFour"), EFindName::FNAME_Add));
        names.Add(FName(STR("HelloFive"), EFindName::FNAME_Add));
        return names;
    }

    auto Test_Get_UObject_Nullptr() -> UObject*
    {
        return nullptr;
    }
}