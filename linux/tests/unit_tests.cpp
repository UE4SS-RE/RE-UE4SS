#include "ue4ss/case_path.hpp"
#include "ue4ss/elf_image.hpp"
#include "ue4ss/mod_catalog.hpp"
#include "ue4ss/sha256.h"
#include "ue4ss/utf.hpp"
#if UE4SS_WITH_HOOK_BACKEND
#include "ue4ss/inline_hook.hpp"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/mman.h>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void test_sha256()
    {
        char template_path[] = "/tmp/ue4ss-sha-XXXXXX";
        const int descriptor = mkstemp(template_path);
        require(descriptor >= 0, "mkstemp failed");
        constexpr std::string_view contents{"abc"};
        require(write(descriptor, contents.data(), contents.size()) == static_cast<ssize_t>(contents.size()), "test file write failed");
        close(descriptor);
        std::array<char, 65> hash{};
        require(ue4ss_sha256_file(template_path, hash.data()) == 0, "SHA-256 file operation failed");
        unlink(template_path);
        require(hash.data() == std::string{"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"}, "SHA-256 vector mismatch");
    }

    void test_utf()
    {
        const std::string source = "Palworld \xf0\x9f\x8c\x8d";
        const std::u16string unreal = ue4ss::linux::utf8_to_unreal(source);
        require(ue4ss::linux::unreal_to_utf8(unreal) == source, "UTF roundtrip mismatch");
        bool rejected = false;
        try
        {
            static_cast<void>(ue4ss::linux::utf8_to_unreal("\xc0\x80"));
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        require(rejected, "overlong UTF-8 was accepted");
        require(sizeof(wchar_t) == 4u, "test host does not expose the Linux wchar_t ABI");
        require(sizeof(char16_t) == 2u, "char16_t is not suitable for Unreal TCHAR");
    }

    void test_case_paths()
    {
        char directory_template[] = "/tmp/ue4ss-case-XXXXXX";
        char* directory = mkdtemp(directory_template);
        require(directory != nullptr, "mkdtemp failed");
        const std::filesystem::path root{directory};
        std::filesystem::create_directories(root / "ExampleMod" / "Scripts");
        std::ofstream{root / "ExampleMod" / "Scripts" / "main.lua"} << "return true\n";

        auto result = ue4ss::linux::resolve_case_insensitive(root.string(), "examplemod/scripts/MAIN.LUA");
        require(result && result.path.ends_with("ExampleMod/Scripts/main.lua"), "case-insensitive path did not resolve");

        std::filesystem::create_directories(root / "ExampleMod" / "scripts");
        result = ue4ss::linux::resolve_case_insensitive(root.string(), "ExampleMod/SCRIPTS");
        require(result.error == ue4ss::linux::PathResolutionError::ambiguous, "ambiguous path variants were accepted");
        std::filesystem::remove_all(root);
    }

    void test_mod_catalog()
    {
        char directory_template[] = "/tmp/ue4ss-mods-XXXXXX";
        char* directory = mkdtemp(directory_template);
        require(directory != nullptr, "mkdtemp failed");
        const std::filesystem::path root{directory};

        std::filesystem::create_directories(root / "Mods" / "ValidMod" / "Scripts");
        std::ofstream{root / "Mods" / "ValidMod" / "enabled.txt"};
        std::ofstream{root / "Mods" / "ValidMod" / "Scripts" / "main.lua"} << "return true\n";

        std::filesystem::create_directories(root / "Mods" / "DisabledMod" / "Scripts");
        std::ofstream{root / "Mods" / "DisabledMod" / "Scripts" / "main.lua"} << "return true\n";

        std::filesystem::create_directories(root / "Mods" / "BrokenMod" / "Scripts");
        std::ofstream{root / "Mods" / "BrokenMod" / "enabled.txt"};

        std::filesystem::create_directories(root / "Mods" / "AmbiguousMod" / "Scripts");
        std::filesystem::create_directories(root / "Mods" / "AmbiguousMod" / "scripts");
        std::ofstream{root / "Mods" / "AmbiguousMod" / "enabled.txt"};
        std::ofstream{root / "Mods" / "AmbiguousMod" / "Scripts" / "main.lua"} << "return true\n";

        std::filesystem::create_directories(root / "Mods" / "Duplicate");
        std::filesystem::create_directories(root / "Mods" / "duplicate");
        std::ofstream{root / "Mods" / "Duplicate" / "enabled.txt"};
        std::ofstream{root / "Mods" / "duplicate" / "enabled.txt"};

        std::filesystem::create_directories(root / "Mods" / "LinkedMod" / "real-scripts");
        std::ofstream{root / "Mods" / "LinkedMod" / "enabled.txt"};
        std::ofstream{root / "Mods" / "LinkedMod" / "real-scripts" / "main.lua"} << "return true\n";
        std::filesystem::create_directory_symlink("real-scripts", root / "Mods" / "LinkedMod" / "Scripts");

        const auto catalog = ue4ss::linux::discover_lua_mods(root);
        require(catalog.enabled_mods.size() == 1u, "catalog did not isolate the one valid enabled mod");
        require(catalog.enabled_mods.front().name == "ValidMod", "catalog returned the wrong enabled mod");
        require(catalog.enabled_mods.front().main_script.filename() == "main.lua", "catalog returned the wrong main script");
        require(catalog.rejected_mods.size() == 4u, "catalog did not report every invalid enabled mod");
        require(std::any_of(catalog.rejected_mods.begin(), catalog.rejected_mods.end(), [](const std::string& error) {
                    return error.find("ambiguous case variants") != std::string::npos;
                }),
                "catalog accepted ambiguous Scripts directories");
        require(std::any_of(catalog.rejected_mods.begin(), catalog.rejected_mods.end(), [](const std::string& error) {
                    return error.find("case-colliding mod directories") != std::string::npos;
                }),
                "catalog accepted case-colliding mod names");
        require(std::any_of(catalog.rejected_mods.begin(), catalog.rejected_mods.end(), [](const std::string& error) {
                    return error.find("must not be symlinks") != std::string::npos;
                }),
                "catalog accepted a symlinked Scripts directory");
        std::filesystem::remove_all(root);
    }

    void test_elf()
    {
        const auto image = ue4ss::linux::inspect_current_process();
        require(image.elf_class == 64u, "current image is not ELF64");
        require(image.machine == EM_X86_64, "current image is not x86_64");
        require(image.sha256.size() == 64u, "current image does not have a SHA-256 identity");
        require(!image.modules.empty(), "process module map is empty");
        bool executable_segment = false;
        for (const auto& segment : image.segments)
        {
            executable_segment = executable_segment || segment.executable;
        }
        require(executable_segment, "main image does not contain an executable segment");
    }

#if UE4SS_WITH_HOOK_BACKEND
    extern "C" __attribute__((noinline)) int replacement_with_argument(int value)
    {
        return value + 100;
    }

    extern "C" __attribute__((noinline)) int original_with_argument(int value)
    {
        return value + 1;
    }

    extern "C" __attribute__((noinline)) int replacement_without_argument()
    {
        return 100;
    }

    class ExecutablePage
    {
      public:
        ExecutablePage()
        {
            m_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
            m_data = static_cast<std::uint8_t*>(mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            require(m_data != MAP_FAILED, "cannot allocate synthetic executable page");
            std::memset(m_data, 0x90, m_size);
        }

        ~ExecutablePage()
        {
            if (m_data != MAP_FAILED)
            {
                munmap(m_data, m_size);
            }
        }

        void make_executable()
        {
            require(mprotect(m_data, m_size, PROT_READ | PROT_EXEC) == 0, "cannot make synthetic page executable");
        }

        [[nodiscard]] std::uint8_t* data() const
        {
            return m_data;
        }

      private:
        std::uint8_t* m_data{static_cast<std::uint8_t*>(MAP_FAILED)};
        std::size_t m_size{};
    };

    void test_simple_inline_hook()
    {
        ExecutablePage page;
        constexpr std::array<std::uint8_t, 6> function_bytes{0x89, 0xf8, 0x83, 0xc0, 0x01, 0xc3}; // eax = edi + 1
        std::memcpy(page.data(), function_bytes.data(), function_bytes.size());
        page.make_executable();

        using Function = int (*)(int);
        const auto target = reinterpret_cast<Function>(page.data());
        require(target(5) == 6, "synthetic original function is invalid");

        ue4ss::linux::InlineHook hook;
        std::string error;
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error), "simple hook install failed: " + error);
        require(hook.is_installed(), "simple hook does not report installed state");
        require(target(5) == 105, "simple hook did not call replacement");
        const auto original = reinterpret_cast<Function>(hook.trampoline());
        require(original(5) == 6, "simple hook trampoline did not call original behavior");
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error), "idempotent hook install failed");
        require(hook.remove(error), "simple hook removal failed: " + error);
        require(target(5) == 6, "simple hook did not restore original bytes");
        require(hook.remove(error), "idempotent hook removal failed");
    }

    void test_rip_relative_inline_hook()
    {
        ExecutablePage page;
        constexpr std::array<std::uint8_t, 10> function_bytes{
                0x8b,
                0x05,
                0x1a,
                0x00,
                0x00,
                0x00, // mov eax, [rip + 26] -> offset 32
                0x83,
                0xc0,
                0x01, // add eax, 1
                0xc3, // ret
        };
        std::memcpy(page.data(), function_bytes.data(), function_bytes.size());
        const std::int32_t value = 41;
        std::memcpy(page.data() + 32, &value, sizeof(value));
        page.make_executable();

        using Function = int (*)();
        const auto target = reinterpret_cast<Function>(page.data());
        require(target() == 42, "synthetic RIP-relative function is invalid");

        ue4ss::linux::InlineHook hook;
        std::string error;
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_without_argument), error), "RIP-relative hook install failed: " + error);
        require(target() == 100, "RIP-relative hook did not call replacement");
        const auto original = reinterpret_cast<Function>(hook.trampoline());
        require(original() == 42, "RIP-relative instruction was not relocated correctly");
        require(hook.remove(error), "RIP-relative hook removal failed: " + error);
        require(target() == 42, "RIP-relative hook did not restore original behavior");
    }

    void test_relative_call_inline_hook()
    {
        ExecutablePage page;
        constexpr std::array<std::uint8_t, 9> function_bytes{
                0xe8,
                0x3b,
                0x00,
                0x00,
                0x00, // call offset 64
                0x83,
                0xc0,
                0x01, // add eax, 1
                0xc3, // ret
        };
        constexpr std::array<std::uint8_t, 6> helper_bytes{
                0xb8,
                0x07,
                0x00,
                0x00,
                0x00, // mov eax, 7
                0xc3, // ret
        };
        std::memcpy(page.data(), function_bytes.data(), function_bytes.size());
        std::memcpy(page.data() + 64, helper_bytes.data(), helper_bytes.size());
        page.make_executable();

        using Function = int (*)();
        const auto target = reinterpret_cast<Function>(page.data());
        require(target() == 8, "synthetic relative-call function is invalid");

        ue4ss::linux::InlineHook hook;
        std::string error;
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_without_argument), error), "relative-call hook install failed: " + error);
        const auto original = reinterpret_cast<Function>(hook.trampoline());
        require(original() == 8, "relative call was not relocated correctly");
        require(hook.remove(error), "relative-call hook removal failed: " + error);
    }

    void test_relative_jump_inline_hook()
    {
        ExecutablePage page;
        constexpr std::array<std::uint8_t, 5> function_bytes{
                0xe9,
                0x3b,
                0x00,
                0x00,
                0x00, // jump to offset 64
        };
        constexpr std::array<std::uint8_t, 6> helper_bytes{
                0xb8,
                0x07,
                0x00,
                0x00,
                0x00, // mov eax, 7
                0xc3, // ret
        };
        std::memcpy(page.data(), function_bytes.data(), function_bytes.size());
        std::memcpy(page.data() + 64, helper_bytes.data(), helper_bytes.size());
        page.make_executable();

        using Function = int (*)();
        const auto target = reinterpret_cast<Function>(page.data());
        require(target() == 7, "synthetic relative-jump function is invalid");

        ue4ss::linux::InlineHook hook;
        std::string error;
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_without_argument), error), "relative-jump hook install failed: " + error);
        const auto original = reinterpret_cast<Function>(hook.trampoline());
        require(original() == 7, "relative jump was not relocated correctly");
        require(hook.remove(error), "relative-jump hook removal failed: " + error);
        require(target() == 7, "relative-jump hook did not restore original behavior");
    }

    void test_short_conditional_branch_inline_hook()
    {
        ExecutablePage page;
        constexpr std::array<std::uint8_t, 24> function_bytes{
                0x85, 0xff,                         // test edi, edi
                0x74, 0x0e,                         // jz offset 18 (outside overwritten prologue)
                0xb8, 0x07, 0x00, 0x00, 0x00,       // mov eax, 7
                0xc3,                               // ret
                0x90, 0x90, 0x90, 0x90,             // padding consumed by the 14-byte patch
                0x90, 0x90, 0x90, 0x90,
                0xb8, 0x2a, 0x00, 0x00, 0x00,       // mov eax, 42
                0xc3,                               // ret
        };
        std::memcpy(page.data(), function_bytes.data(), function_bytes.size());
        page.make_executable();

        using Function = int (*)(int);
        const auto target = reinterpret_cast<Function>(page.data());
        require(target(1) == 7 && target(0) == 42, "synthetic short-branch function is invalid");

        ue4ss::linux::InlineHook hook;
        std::string error;
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error),
                "short conditional branch hook install failed: " + error);
        require(target(3) == 103, "short-branch hook did not call replacement");
        const auto original = reinterpret_cast<Function>(hook.trampoline());
        require(original(1) == 7 && original(0) == 42,
                "short conditional branch was not promoted and relocated correctly");
        require(hook.remove(error), "short-branch hook removal failed: " + error);
        require(target(1) == 7 && target(0) == 42, "short-branch hook did not restore original behavior");
    }

    void test_staged_inline_hook_retirement()
    {
        ExecutablePage page;
        constexpr std::array<std::uint8_t, 6> function_bytes{0x89, 0xf8, 0x83, 0xc0, 0x01, 0xc3};
        std::memcpy(page.data(), function_bytes.data(), function_bytes.size());
        page.make_executable();

        using Function = int (*)(int);
        const auto target = reinterpret_cast<Function>(page.data());
        ue4ss::linux::InlineHook hook;
        std::string error;
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error),
                "staged hook install failed: " + error);
        const auto in_flight_original = reinterpret_cast<Function>(hook.trampoline());
        require(target(4) == 104 && in_flight_original(4) == 5,
                "staged hook did not expose replacement and original paths");

        require(hook.restore(error), "staged hook restore failed: " + error);
        require(!hook.is_installed(), "restored staged hook still reports installed state");
        require(target(4) == 5, "staged restore did not make future target calls original");
        require(hook.trampoline() == reinterpret_cast<void*>(in_flight_original),
                "staged restore released its in-flight trampoline too early");
        require(in_flight_original(4) == 5,
                "retained trampoline stopped working before the in-flight drain");
        require(!hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error),
                "staged hook allowed reinstall before releasing the retired trampoline");

        require(hook.release(error), "staged trampoline release failed: " + error);
        require(hook.trampoline() == nullptr && hook.target() == nullptr,
                "staged trampoline release retained hook resources");
        require(hook.release(error), "staged trampoline release is not idempotent");
        require(hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error),
                "hook could not be reinstalled after staged release: " + error);
        require(hook.remove(error), "reinstalled staged hook could not be removed: " + error);
    }

    void test_pointer_hook()
    {
        using Function = int (*)(int);
        void* slot = reinterpret_cast<void*>(&original_with_argument);
        ue4ss::linux::PointerHook hook;
        std::string error;
        require(hook.install(&slot, reinterpret_cast<void*>(&replacement_with_argument), error), "pointer hook install failed: " + error);
        require(hook.is_installed(), "pointer hook does not report installed state");
        require(reinterpret_cast<Function>(slot)(5) == 105, "pointer hook did not replace the slot");
        require(reinterpret_cast<Function>(hook.trampoline())(5) == 6, "pointer hook did not preserve the original slot");
        require(hook.install(&slot, reinterpret_cast<void*>(&replacement_with_argument), error), "idempotent pointer hook install failed");
        require(hook.remove(error), "pointer hook removal failed: " + error);
        require(reinterpret_cast<Function>(slot)(5) == 6, "pointer hook did not restore the slot");
        require(hook.remove(error), "idempotent pointer hook removal failed");
    }

    void test_parallel_hook_instances()
    {
        std::atomic<bool> passed{true};
        std::array<std::thread, 4> workers;
        for (auto& worker : workers)
        {
            worker = std::thread{[&passed] {
                try
                {
                    ExecutablePage page;
                    constexpr std::array<std::uint8_t, 6> bytes{0x89, 0xf8, 0x83, 0xc0, 0x01, 0xc3};
                    std::memcpy(page.data(), bytes.data(), bytes.size());
                    page.make_executable();
                    using Function = int (*)(int);
                    const auto target = reinterpret_cast<Function>(page.data());
                    ue4ss::linux::InlineHook hook;
                    std::string error;
                    if (!hook.install(page.data(), reinterpret_cast<void*>(&replacement_with_argument), error) || target(3) != 103 || !hook.remove(error) ||
                        target(3) != 4)
                    {
                        passed = false;
                    }
                }
                catch (...)
                {
                    passed = false;
                }
            }};
        }
        for (auto& worker : workers)
        {
            worker.join();
        }
        require(passed, "parallel independent hook instances failed");
    }
#endif
} // namespace

int main()
{
    try
    {
        test_sha256();
        test_utf();
        test_case_paths();
        test_mod_catalog();
        test_elf();
#if UE4SS_WITH_HOOK_BACKEND
        test_simple_inline_hook();
        test_rip_relative_inline_hook();
        test_relative_call_inline_hook();
        test_relative_jump_inline_hook();
        test_short_conditional_branch_inline_hook();
        test_staged_inline_hook_retirement();
        test_pointer_hook();
        test_parallel_hook_instances();
#endif
        std::cout << "all UE4SS Linux unit tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "test failure: " << error.what() << '\n';
        return 1;
    }
}
