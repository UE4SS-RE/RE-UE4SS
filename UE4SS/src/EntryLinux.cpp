#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <codecvt>

#include "UE4SSProgram.hpp"
#include "Platform.hpp"

#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>

using namespace RC;

pthread_t ue4ss_mainthread;

void UE4SS_Start()
{
    // find libUE4SS.so path using dlfcn
    Dl_info dl_info;
    if (dladdr((void *)UE4SS_Start, &dl_info) == 0)
    {
        std::cerr << "Failed to find libUE4SS.so path" << std::endl;
        return;
    }
    //#define EARLY_THROW_TEST
    #ifdef EARLY_THROW_TEST
    try {
        throw std::runtime_error{"Hello from Here!"};
    } catch (std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
    }
    #endif
    SystemStringType ue4sspath = to_generic_string(dl_info.dli_fname);
    auto program = new UE4SSProgram(ue4sspath, {});
    
    // use pthread here
    pthread_create(&ue4ss_mainthread, nullptr, [](void *arg) -> void * {
        auto program = (UE4SSProgram *)arg;
        program->init();
        fprintf(stderr, "inited in thread\n");
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

static bool UE4SSInited = false;

/*
void __attribute__((constructor)) UE4SS_Init() {
    // It may not init in correct order..
    if (!UE4SSInited) {
        printf("UE4SS_Init Called\n");
       // UE4SS_Start();
        printf("UE4SS_Start Returned\n");
        UE4SSInited = true;
    }
}
*/

static int (*next_main)(int, char **, char **);
static bool inited = false;
int hooked_main(int argc, char **argv, char **envp) {
    // delay the hook to make sure all objects are constructed
    fprintf(stderr, "Running hooked main before actual main my pid = %d\n", getpid());
    if (!inited) {
        fprintf(stderr, "Do another hook?\n");
        UE4SS_Start();
        inited = true;
    } else {
        fprintf(stderr, "UE4SS already inited? \n");
    }
    return next_main(argc, argv, envp);
}

#define ALIGN_UP_PAGE(x) (( ((unsigned long)(x)) + 0xFFF) & ~0xFFF)
#define ALIGN_DOWN_PAGE(x) (((unsigned long)(x)) & ~0xFFF)

extern char **environ;

extern "C" {
int  __libc_start_main(
    int (*main)(int, char **, char **),
    int argc,
    char **argv,
    int (*init)(int, char **, char **),
    void (*fini)(void),
    void (*rtld_fini)(void),
    void *stack_end)
{
    next_main = main;
    typeof(&__libc_start_main) orig = (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");
    // gxx fix
    void *current___gxx_personality_v0 = dlsym(RTLD_NEXT, "__gxx_personality_v0");
    Dl_info dl_info;
    Elf64_Sym *sym;
    dladdr1(current___gxx_personality_v0, &dl_info, (void**) &sym, RTLD_DL_SYMENT);
    fprintf(stderr, "__gxx_personality_v0 found in %s @ %p\n", dl_info.dli_fname, current___gxx_personality_v0);
    if (strstr(dl_info.dli_fname, "libsteam_api.so") != nullptr) {
        fprintf(stderr, "libsteam_api shit detected\n");
        mprotect((void*)ALIGN_DOWN_PAGE(sym), ALIGN_UP_PAGE(sym + 1) - ALIGN_DOWN_PAGE(sym), PROT_READ | PROT_WRITE);
        sym->st_info = ELF64_ST_INFO(STB_LOCAL, ELF64_ST_TYPE(sym->st_info));
        sym->st_other = STV_INTERNAL;
    }
    current___gxx_personality_v0 = dlsym(RTLD_DEFAULT, "__gxx_personality_v0");
    fprintf(stderr, "Now the current___gxx_personality_v0 is %p\n", current___gxx_personality_v0);

    // remove LD_PRELOAD from environ
    int nenv = 0;
    for (int i = 0; environ[i]; i++)
        nenv ++;
    for (int i = 0; i < nenv; i++) {
        if (strncmp(environ[i], "LD_PRELOAD=", 11) == 0) {
            fprintf(stderr, "Found LD_PRELOAD @%d %s\n", i, environ[i]);
            environ[i] = environ[nenv - 1];
            nenv --;
        }
    }
    environ[nenv] = NULL;

    return orig(hooked_main, argc, argv, init, fini, rtld_fini, stack_end);
}

}

// destructor
void __attribute__((destructor)) UE4SS_End()
{
    if (UE4SSInited) {
        // UE4SSProgram::static_cleanup();
        UE4SSInited = false;
    }
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
