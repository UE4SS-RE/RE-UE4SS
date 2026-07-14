#include <chrono>
#include <cstdio>
#include <string_view>
#include <thread>

#include <dlfcn.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    Dl_info dlopen_info{};
    auto* visible_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!visible_dlopen || dladdr(visible_dlopen, &dlopen_info) == 0 || !dlopen_info.dli_fname
        || std::string_view{dlopen_info.dli_fname}.find("libUE4SS") == std::string_view::npos)
    {
        return 3;
    }

    auto* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library)
    {
        return 4;
    }

    using AnswerFunction = int (*)();
    auto answer = reinterpret_cast<AnswerFunction>(dlsym(library, "shared_library_fixture_answer"));
    if (!answer || answer() != 42)
    {
        dlclose(library);
        return 5;
    }

    if (dlclose(library) != 0)
    {
        return 6;
    }

    std::puts("UE4SS_PRELOAD_PROBE_READY");
    return 0;
}
