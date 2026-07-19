#include <cstdint>

struct PsEngineVersion
{
    uint16_t major{};
    uint16_t minor{};
};

struct PsFileScanResults
{
    PsEngineVersion engine_version{};
    uint64_t pattern_address{};
};

extern "C" bool ps_scan_file(const char* path, const char* pattern, PsFileScanResults* results);

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 3)
    {
        return 2;
    }

    PsFileScanResults results{};
    const auto* pattern = argc == 3 ? argv[2] : "DE C0 AD 0B 5E A1 77 13 37 42 AB CD EF 01 23 45";
    if (!ps_scan_file(argv[1], pattern, &results))
    {
        return 3;
    }
    if (results.engine_version.major != 5 || results.engine_version.minor != 1)
    {
        return 4;
    }
    return results.pattern_address != 0 ? 0 : 5;
}
