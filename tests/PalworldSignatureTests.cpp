#include <array>
#include <cstdint>
#include <cstdio>
#include <utility>

struct PsEngineVersion
{
    uint16_t major{};
    uint16_t minor{};
};

struct PsFileResolutionResults
{
    PsEngineVersion engine_version{};
    uint64_t guobject_array{};
    uint64_t fname_tostring{};
    uint64_t fname_ctor_wchar{};
    uint64_t gmalloc{};
    uint64_t static_construct_object_internal{};
    uint64_t ftext_fstring{};
    uint64_t fuobject_hash_tables_get{};
    uint64_t gnatives{};
    uint64_t console_manager_singleton{};
    uint64_t gameengine_tick{};
};

extern "C" bool ps_scan_file_ue4ss(const char* path, PsFileResolutionResults* results);

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    PsFileResolutionResults results{};
    if (!ps_scan_file_ue4ss(argv[1], &results))
    {
        std::fprintf(stderr, "Palworld signature scan failed to read or resolve the ELF image\n");
        return 3;
    }

    if (results.engine_version.major != 5 || results.engine_version.minor != 1)
    {
        std::fprintf(stderr,
                     "Expected Unreal Engine 5.1, got %u.%u\n",
                     results.engine_version.major,
                     results.engine_version.minor);
        return 4;
    }

    const std::array required_resolvers{
            std::pair{"GUObjectArray", results.guobject_array},
            std::pair{"FNameToString", results.fname_tostring},
            std::pair{"FNameCtorWchar", results.fname_ctor_wchar},
            std::pair{"GMalloc", results.gmalloc},
            std::pair{"StaticConstructObjectInternal", results.static_construct_object_internal},
            std::pair{"FTextFString", results.ftext_fstring},
            std::pair{"GNatives", results.gnatives},
            std::pair{"ConsoleManagerSingleton", results.console_manager_singleton},
            std::pair{"UGameEngineTick", results.gameengine_tick},
    };

    bool missing_required{};
    for (const auto& [name, address] : required_resolvers)
    {
        std::printf("%-31s 0x%llx\n", name, static_cast<unsigned long long>(address));
        if (address == 0)
        {
            std::fprintf(stderr, "Missing required resolver: %s\n", name);
            missing_required = true;
        }
    }

    std::printf("%-31s 0x%llx%s\n",
                "FUObjectHashTablesGet",
                static_cast<unsigned long long>(results.fuobject_hash_tables_get),
                results.fuobject_hash_tables_get == 0 ? " (optional; inlined in this build)" : "");

    return missing_required ? 5 : 0;
}
