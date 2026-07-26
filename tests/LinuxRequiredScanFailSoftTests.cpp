#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <Unreal/Signatures.hpp>
#include <Unreal/UnrealInitializer.hpp>

namespace
{
    struct PsScanConfig
    {
        bool g_uobject_array{};
        bool fname_tostring_fstring{};
        bool fname_ctor_wchar{};
        bool gmalloc{};
        bool static_construct_object_internal{};
        bool ftext_fstring{};
        bool engine_version{};
        bool fuobject_hash_tables_get{};
        bool gnatives{};
        bool console_manager_singleton{};
        bool gameengine_tick{};
    };

    struct PsCtx
    {
        void (*default_)(RC::CharType* message);
        void (*normal)(RC::CharType* message);
        void (*verbose)(RC::CharType* message);
        void (*warning)(RC::CharType* message);
        void (*error)(RC::CharType* message);
        PsScanConfig config{};
    };

    struct PsEngineVersion
    {
        uint16_t major{};
        uint16_t minor{};
    };

    struct PsScanResults
    {
        void* g_uobject_array{};
        void* fname_tostring_fstring{};
        void* fname_ctor_wchar{};
        void* gmalloc{};
        void* static_construct_object_internal{};
        void* ftext_fstring{};
        PsEngineVersion engine_version{};
        void* fuobject_hash_tables_get{};
        void* gnatives{};
        void* console_manager_singleton{};
        void* gameengine_tick{};
    };

    int g_required_override_attempts{};
    int g_address_storage{};
} // namespace

extern "C" bool __wrap_ps_scan(PsCtx&, PsScanResults& results)
{
    auto* address = static_cast<void*>(&g_address_storage);
    results.g_uobject_array = address;
    results.fname_tostring_fstring = address;
    results.fname_ctor_wchar = address;
    results.gmalloc = address;
    results.static_construct_object_internal = address;
    results.ftext_fstring = address;
    results.engine_version = {5, 1};
    results.fuobject_hash_tables_get = address;
    results.gnatives = address;
    results.console_manager_singleton = address;
    results.gameengine_tick = address;
    return true;
}

int main()
{
    using namespace RC::Unreal;

    UnrealInitializer::Config config{};
    config.SecondsToScanBeforeGivingUp = 0;
    config.ScanOverrides.guobjectarray = [](std::vector<RC::SignatureContainer>&, Signatures::ScanResult& result) {
        ++g_required_override_attempts;
        result.Errors.emplace_back("required GUObjectArray override did not resolve");
    };

    UnrealInitializer::StaticStorage::GlobalConfig = config;
    UnrealInitializer::StaticStorage::GlobalConfig.bIsForcedPreScan = false;
    UnrealInitializer::StaticStorage::bScanFullyCompleted = false;

    try
    {
        UnrealInitializer::ScanGame();
    }
    catch (const std::runtime_error& error)
    {
        if (g_required_override_attempts != 1)
        {
            return 2;
        }
        if (std::string_view{error.what()}.find("required GUObjectArray override did not resolve") == std::string_view::npos)
        {
            return 3;
        }
        return UnrealInitializer::StaticStorage::bScanFullyCompleted ? 4 : 0;
    }

    return g_required_override_attempts == 0 ? 1 : 5;
}
