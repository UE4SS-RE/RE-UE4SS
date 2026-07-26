#include <array>
#include <cstddef>
#include <unordered_map>

#include <File/File.hpp>
#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtualBaseVC.hpp>

namespace
{
    bool s_wrong_slot_called{};
    bool s_expected_slot_called{};

    class VirtualFixture
    {
    public:
        static std::unordered_map<RC::File::StringType, uint32_t> VTableLayoutMap;

        void Invoke(void* argument)
        {
            IMPLEMENT_UNREAL_VIRTUAL_WRAPPER(VirtualFixture, Invoke, void, PARAMS(void*), ARGS(argument))
        }
    };

    std::unordered_map<RC::File::StringType, uint32_t> VirtualFixture::VTableLayoutMap{};

    extern "C" void wrong_slot(VirtualFixture*, void*)
    {
        s_wrong_slot_called = true;
    }

    extern "C" void expected_slot(VirtualFixture*, void*)
    {
        s_expected_slot_called = true;
    }

    struct FakeItaniumObject
    {
        void** vtable;
    };
} // namespace

int main()
{
    std::array<void*, 4> vtable{};
    vtable[2] = reinterpret_cast<void*>(&wrong_slot);
    vtable[3] = reinterpret_cast<void*>(&expected_slot);

    FakeItaniumObject fake_object{vtable.data()};
    auto* fixture = reinterpret_cast<VirtualFixture*>(&fake_object);

    VirtualFixture::VTableLayoutMap.emplace(STR("__vecDelDtor"), 0x0);
    VirtualFixture::VTableLayoutMap.emplace(STR("Invoke"), 0x10);

    int argument{};
    fixture->Invoke(&argument);

    if (s_wrong_slot_called)
    {
        return 1;
    }
    return s_expected_slot_called ? 0 : 2;
}
