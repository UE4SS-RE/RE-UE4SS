#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <thread>

#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    auto print_environment(const char* prefix) -> void
    {
        const auto* preload = std::getenv("LD_PRELOAD");
        const auto* target = std::getenv("UE4SS_LAUNCH_TARGET_EXE");
        const auto* was_set = std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET");
        const auto* original = std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD");
        const auto* module = std::getenv("UE4SS_MODULE_PATH");
        std::printf("%s_LD_PRELOAD=%s\n", prefix, preload ? preload : "<unset>");
        std::printf("%s_TARGET=%s\n", prefix, target ? target : "<unset>");
        std::printf("%s_WAS_SET=%s\n", prefix, was_set ? was_set : "<unset>");
        std::printf("%s_ORIGINAL=%s\n", prefix, original ? original : "<unset>");
        std::printf("%s_MODULE=%s\n", prefix, module ? module : "<unset>");
    }
} // namespace

int main(int argc, char** argv)
{
    if (argc == 4 && std::string_view{argv[1]} == "--helper")
    {
        print_environment("HELPER");
        if (auto* ue4ss = dlopen(argv[2], RTLD_NOW | RTLD_NOLOAD))
        {
            dlclose(ue4ss);
            return 11;
        }

        const auto* preload = std::getenv("LD_PRELOAD");
        const auto* target = std::getenv("UE4SS_LAUNCH_TARGET_EXE");
        const auto* was_set = std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET");
        const auto* original = std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD");
        const auto* module = std::getenv("UE4SS_MODULE_PATH");
        if (!preload || std::string_view{preload} != argv[3] || target || was_set || original || module)
        {
            return 12;
        }
        std::puts("UE4SS_PRELOAD_SCOPE_HELPER_CLEAN");
        return 0;
    }

    if (argc == 3 && std::string_view{argv[1]} == "--box64-orphan")
    {
        if (auto* ue4ss = dlopen(argv[2], RTLD_NOW | RTLD_NOLOAD))
        {
            dlclose(ue4ss);
            std::this_thread::sleep_for(std::chrono::milliseconds{250});
            std::puts("UE4SS_BOX64_ORPHAN_PROBE_CLEAN");
            return 0;
        }
        return 21;
    }

    if (argc != 3)
    {
        std::fprintf(stderr, "usage: %s <libUE4SS.so> <expected original LD_PRELOAD>\n", argv[0]);
        return 2;
    }

    print_environment("HOST");
    const auto* preload = std::getenv("LD_PRELOAD");
    if (!preload || std::string_view{preload} != argv[2] || std::getenv("UE4SS_LAUNCH_TARGET_EXE") ||
        std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET") || std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD") ||
        std::getenv("UE4SS_MODULE_PATH"))
    {
        return 3;
    }

    const auto child = fork();
    if (child < 0)
    {
        return 4;
    }
    if (child == 0)
    {
        execl(argv[0], argv[0], "--helper", argv[1], argv[2], static_cast<char*>(nullptr));
        _exit(127);
    }

    int status{};
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        return 5;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    std::puts("UE4SS_PRELOAD_SCOPE_HOST_READY");
    return 0;
}
