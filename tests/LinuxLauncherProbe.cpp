#include <cstdio>
#include <cstdlib>
#include <string_view>

#include <dlfcn.h>

int main(int argc, char** argv)
{
    const auto* ld_preload = std::getenv("LD_PRELOAD");
    const auto* ue4ss_module_path = std::getenv("UE4SS_MODULE_PATH");
    const auto* launch_target = std::getenv("UE4SS_LAUNCH_TARGET_EXE");
    const auto* preload_was_set = std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET");
    const auto* original_preload = std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD");
    std::printf("LD_PRELOAD=%s\n", ld_preload ? ld_preload : "");
    std::printf("UE4SS_MODULE_PATH=%s\n", ue4ss_module_path ? ue4ss_module_path : "");
    std::printf("UE4SS_LAUNCH_TARGET_EXE=%s\n", launch_target ? launch_target : "");
    std::printf("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=%s\n", preload_was_set ? preload_was_set : "");
    std::printf("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=%s\n", original_preload ? original_preload : "");

    Dl_info dlopen_info{};
    auto* visible_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!visible_dlopen || dladdr(visible_dlopen, &dlopen_info) == 0 || !dlopen_info.dli_fname)
    {
        return 5;
    }
    const std::string_view preload_owner{dlopen_info.dli_fname};
    if (!preload_owner.contains("libUE4SS") && !preload_owner.starts_with("/proc/self/fd/"))
    {
        return 6;
    }
    std::printf("PRELOAD_OWNER=%s\n", dlopen_info.dli_fname);

    std::printf("ARGC=%d\n", argc);
    for (int index = 0; index < argc; ++index)
    {
        std::printf("ARG[%d]=%s\n", index, argv[index]);
    }
    return 0;
}
