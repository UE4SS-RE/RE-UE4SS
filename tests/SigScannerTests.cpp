#include <cstdio>
#include <vector>

#include <SigScanner/SinglePassSigScanner.hpp>

using namespace RC;

__attribute__((used, section(".rodata"))) static const unsigned char g_needle[] = {
        0xDE, 0xC0, 0xAD, 0x0B, 0x5E, 0xA1, 0x77, 0x13, 0x37, 0x42, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45};

static auto run_scan(SinglePassScanner::ScanMethod method) -> bool
{
    bool found = false;
    void* found_address = nullptr;

    std::vector<SignatureContainer> signatures;
    signatures.emplace_back(
            std::vector<SignatureData>{{.signature = "DE C0 AD 0B 5E A1 77 13 37 42 AB CD EF 01 23 45"}},
            [&](SignatureContainer& container) -> bool {
                found = true;
                found_address = container.get_match_address();
                return true;
            },
            [](SignatureContainer&) {});

    SinglePassScanner::SignatureContainerMap containers;
    containers.emplace(ScanTarget::MainExe, std::move(signatures));
    SinglePassScanner::m_scan_method = method;
    SinglePassScanner::start_scan(containers);

    if (!found)
    {
        std::fprintf(stderr, "FAILED: signature not found with scan method %d\n", static_cast<int>(method));
        return false;
    }
    if (found_address != static_cast<const void*>(g_needle))
    {
        std::fprintf(stderr,
                     "FAILED: match address %p != needle address %p with scan method %d\n",
                     found_address,
                     static_cast<const void*>(g_needle),
                     static_cast<int>(method));
        return false;
    }
    return true;
}

int main()
{
    if (SigScannerStaticData::populate_modules_from_dl_iterate_phdr() == 0)
    {
        std::fprintf(stderr, "FAILED: no ELF modules enumerated\n");
        return 1;
    }

    if (!run_scan(SinglePassScanner::ScanMethod::Scalar) || !run_scan(SinglePassScanner::ScanMethod::StdFind))
    {
        return 1;
    }

    std::printf("SigScannerTests: all passed\n");
    return 0;
}
