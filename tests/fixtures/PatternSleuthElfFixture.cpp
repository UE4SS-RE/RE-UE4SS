#include <cstdint>

[[gnu::used]] const char16_t g_branch_name[] = u"++UE5+Release-5.1";
[[gnu::used]] const char16_t g_build_version[] = u"++UE5+Release-5.1-CL-12345678";
[[gnu::used]] const char16_t g_build_date[] = u"Jul 14 2026";

extern "C" [[gnu::used, gnu::noinline]] void patternsleuth_elf_fixture_bytes()
{
    asm volatile(".byte 0xC7, 0x46, 0x20, 0x05, 0x00, 0x01, 0x00, 0x66, 0x89, 0x6E, 0x24\n\t"
                 ".byte 0xDE, 0xC0, 0xAD, 0x0B, 0x5E, 0xA1, 0x77, 0x13, 0x37, 0x42, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45");
}

int main()
{
    return 0;
}
