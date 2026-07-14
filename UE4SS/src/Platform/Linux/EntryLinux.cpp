#ifdef __linux__

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <thread>

#include <dlfcn.h>

#include "UE4SSProgram.hpp"
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

using namespace RC;

namespace
{
    using DlopenFunction = void* (*)(const char*, int);

    std::atomic_bool s_ue4ss_started{false};
    std::atomic_bool s_runtime_ready{false};
    std::atomic<DlopenFunction> s_real_dlopen{};
    thread_local bool s_inside_dlopen{};

    class DlopenRecursionGuard
    {
      public:
        DlopenRecursionGuard()
        {
            s_inside_dlopen = true;
        }

        ~DlopenRecursionGuard()
        {
            s_inside_dlopen = false;
        }
    };

    auto thread_so_start(std::filesystem::path module_path) -> void
    {
        s_runtime_ready.wait(false, std::memory_order_acquire);
        try
        {
            auto* program = new UE4SSProgram(module_path, {});

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
                    std::fprintf(stderr, "UE4SS: %s\n", e->get_message());
                }
            }
        }
        catch (const std::exception& exception)
        {
            std::fprintf(stderr, "UE4SS: initialization failed: %s\n", exception.what());
        }
        catch (...)
        {
            std::fputs("UE4SS: initialization failed with an unknown exception\n", stderr);
        }
    }
} // namespace

namespace RC
{
    void notify_dlopen(const char* filename)
    {
        if (UE4SSProgram::s_program)
        {
            UE4SSProgram::s_program->fire_dll_load_for_cpp_mods(ensure_str(filename));
        }
    }
} // namespace RC

extern "C" __attribute__((visibility("default"))) void* dlopen(const char* filename, int flags)
{
    if (s_inside_dlopen)
    {
        if (const auto real_dlopen = s_real_dlopen.load(std::memory_order_acquire))
        {
            return real_dlopen(filename, flags);
        }
        return nullptr;
    }

    DlopenRecursionGuard recursion_guard{};
    auto real_dlopen = s_real_dlopen.load(std::memory_order_acquire);
    if (!real_dlopen)
    {
        real_dlopen = reinterpret_cast<DlopenFunction>(dlsym(RTLD_NEXT, "dlopen"));
        if (!real_dlopen)
        {
            std::fputs("UE4SS: failed to resolve the real dlopen\n", stderr);
            return nullptr;
        }
        s_real_dlopen.store(real_dlopen, std::memory_order_release);
    }

    auto* library = real_dlopen(filename, flags);
    if (library && filename)
    {
        try
        {
            RC::notify_dlopen(filename);
        }
        catch (const std::exception& exception)
        {
            std::fprintf(stderr, "UE4SS: C++ mod dlopen notification failed: %s\n", exception.what());
        }
        catch (...)
        {
            std::fputs("UE4SS: C++ mod dlopen notification failed with an unknown exception\n", stderr);
        }
    }
    return library;
}

// Called when the .so is loaded (LD_PRELOAD or dlopen). We must not block here:
// spawn the init thread and return so the dynamic linker can finish.
__attribute__((constructor(101))) static void ue4ss_so_attached()
{
    if (const auto* disable_auto_start = std::getenv("UE4SS_DISABLE_AUTO_START");
        disable_auto_start && std::strcmp(disable_auto_start, "1") == 0)
    {
        return;
    }

    bool expected = false;
    if (!s_ue4ss_started.compare_exchange_strong(expected, true))
    {
        return;
    }

    try
    {
        // Locate our own .so path; UE4SSProgram derives all directories from it.
        Dl_info dl_info{};
        if (dladdr(reinterpret_cast<void*>(&ue4ss_so_attached), &dl_info) == 0 || !dl_info.dli_fname)
        {
            std::fputs("UE4SS: failed to determine libUE4SS.so path; startup disabled\n", stderr);
            s_ue4ss_started = false;
            return;
        }

        std::thread init_thread{&thread_so_start, std::filesystem::path{dl_info.dli_fname}};
        init_thread.detach();
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "UE4SS: failed to start initialization: %s\n", exception.what());
        s_ue4ss_started = false;
    }
    catch (...)
    {
        std::fputs("UE4SS: failed to start initialization with an unknown exception\n", stderr);
        s_ue4ss_started = false;
    }
}

__attribute__((constructor(65535))) static void ue4ss_runtime_ready()
{
    s_runtime_ready.store(true, std::memory_order_release);
    s_runtime_ready.notify_all();
}

__attribute__((destructor)) static void ue4ss_so_detached()
{
    if (s_ue4ss_started.exchange(false))
    {
        try
        {
            UE4SSProgram::static_cleanup();
        }
        catch (...)
        {
            std::fputs("UE4SS: cleanup failed during shared-object unload\n", stderr);
        }
    }
}

#endif // __linux__
