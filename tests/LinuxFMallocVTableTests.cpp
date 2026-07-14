#include <array>
#include <cstddef>

#include <Unreal/Core/HAL/UnrealMemory.hpp>
#include <Unreal/FMemory.hpp>
#include <Unreal/UnrealInitializer.hpp>

namespace
{
    bool s_wrong_slot_called{};
    bool s_free_slot_called{};

    extern "C" void wrong_slot(RC::Unreal::FMalloc*, void*)
    {
        s_wrong_slot_called = true;
    }

    extern "C" void free_slot(RC::Unreal::FMalloc*, void*)
    {
        s_free_slot_called = true;
    }

    struct FakeItaniumAllocator
    {
        void** vtable;
    };
} // namespace

int main()
{
    std::array<void*, 8> vtable{};
    vtable[6] = reinterpret_cast<void*>(&wrong_slot);
    vtable[7] = reinterpret_cast<void*>(&free_slot);

    FakeItaniumAllocator fake_allocator{vtable.data()};
    auto* allocator = reinterpret_cast<RC::Unreal::FMalloc*>(&fake_allocator);

    RC::Unreal::FMalloc::VTableLayoutMap.clear();
    RC::Unreal::FMalloc::VTableLayoutMap.emplace(STR("__vecDelDtor"), 0x0);
    RC::Unreal::FMalloc::VTableLayoutMap.emplace(STR("Free"), 0x30);
    RC::Unreal::GMalloc = &allocator;
    RC::Unreal::UnrealInitializer::StaticStorage::bVersionedContainerIsInitialized = true;

    int allocation{};
    RC::Unreal::FMemory::Free(&allocation);

    if (s_wrong_slot_called)
    {
        return 1;
    }
    return s_free_slot_called ? 0 : 2;
}
