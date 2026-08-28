#include <Mod/CppUserModBase.hpp>
#include <UE4SS_SDK/RuntimeSDKTest.hpp>
#include <Unreal/Hooks/Hooks.hpp>
#include <Unreal/UAssetRegistry.hpp>

class SDKRuntimeTestMod : public RC::CppUserModBase
{
  public:
    SDKRuntimeTestMod()
    {
        ModName = STR("SDKRuntimeTestMod");
        ModVersion = STR("1.0");
        ModDescription = STR("Validates a generated SDK against live Unreal reflection.");
        ModAuthors = STR("UE4SS SDK generator contributors");
    }

    auto on_unreal_init() -> void override
    {
        RC::Unreal::Hook::RegisterEngineTickPreCallback(
                [](auto&, RC::Unreal::UEngine*, float, bool) {
                    RC::Unreal::UAssetRegistry::LoadAllAssets();
                    WSDK::run_test();
                },
                {true, true, STR("SDKRuntimeTestMod"), STR("RunLiveSDKTest")});
    }
};

extern "C"
{
    __declspec(dllexport) RC::CppUserModBase* start_mod()
    {
        return new SDKRuntimeTestMod();
    }

    __declspec(dllexport) void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
