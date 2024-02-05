#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <codecvt>

#include "UE4SSProgram.hpp"
#include "Platform.hpp"

#include <dlfcn.h>
#include <link.h>
#include <pthread.h>
#include <unistd.h>

using namespace RC;

pthread_t ue4ss_mainthread;

// constrcutor
void __attribute__((constructor)) UE4SS_Start()
{
    // find libUE4SS.so path using dlfcn
    Dl_info dl_info;
    if (dladdr((void *)UE4SS_Start, &dl_info) == 0)
    {
        std::cerr << "Failed to find libUE4SS.so path" << std::endl;
        return;
    }
    
    SystemStringType ue4sspath = to_generic_string(dl_info.dli_fname);
    auto program = new UE4SSProgram(ue4sspath, {});
    // use pthread here
    pthread_create(&ue4ss_mainthread, nullptr, [](void *arg) -> void * {
        auto program = (UE4SSProgram *)arg;
        program->init();

        if (auto e = program->get_error_object(); e->has_error())
        {
            // If the output system errored out then use printf_s as a fallback
            // Logging will only happen to the debug console but it's something at least
            if (!Output::has_internal_error())
            {
                Output::send<LogLevel::Error>(SYSSTR("Fatal Error: {}\n"), to_generic_string(e->get_message()));
            }
            else
            {
                printf("Error: %s\n", e->get_message());
            }
        }
        return nullptr;
    }, program);
}

// destructor
void __attribute__((destructor)) UE4SS_End()
{
    UE4SSProgram::static_cleanup();
}

std::filesystem::path get_executable_path() {
    // get executable path on Linux
    char result[1024];
    ssize_t count = readlink("/proc/self/exe", result, 1023);
    return std::filesystem::path(std::string(result, (count > 0) ? count : 0));
}

void add_dlsearch_folder(std::filesystem::path &path) {
    // not implemented
}
