#ifdef __linux__

#include <cstdio>
#include <filesystem>
#include <thread>

#include <dlfcn.h>

#include "UE4SSProgram.hpp"
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

using namespace RC;

namespace
{
    bool s_ue4ss_started = false;

    auto thread_so_start(UE4SSProgram* program) -> void
    {
        // Wrapper for entire program
        // Everything must be channeled through MProgram
        // Mirrors thread_dll_start() in Platform/Win32/EntryWin32.cpp
        program->init();

        if (auto e = program->get_error_object(); e->has_error())
        {
            // If the output system errored out then use printf as a fallback
            if (!Output::has_internal_error())
            {
                Output::send<LogLevel::Error>(STR("Fatal Error: {}\n"), ensure_str(e->get_message()));
            }
            else
            {
                printf("Error: %s\n", e->get_message());
            }
        }
    }
} // namespace

// Called when the .so is loaded (LD_PRELOAD or dlopen). We must not block here:
// spawn the init thread and return so the dynamic linker can finish.
__attribute__((constructor)) static void ue4ss_so_attached()
{
    if (s_ue4ss_started)
    {
        return;
    }
    s_ue4ss_started = true;

    // Locate our own .so path; UE4SSProgram derives all directories from it.
    Dl_info dl_info{};
    if (dladdr(reinterpret_cast<void*>(&ue4ss_so_attached), &dl_info) == 0 || !dl_info.dli_fname)
    {
        fprintf(stderr, "UE4SS: failed to determine libUE4SS.so path; aborting startup\n");
        return;
    }

    auto program = new UE4SSProgram(std::filesystem::path{dl_info.dli_fname}, {});
    std::thread{&thread_so_start, program}.detach();
}

__attribute__((destructor)) static void ue4ss_so_detached()
{
    if (s_ue4ss_started)
    {
        UE4SSProgram::static_cleanup();
        s_ue4ss_started = false;
    }
}

#endif // __linux__
