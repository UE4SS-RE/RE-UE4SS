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
    auto ue4sspath = to_system_string(dl_info.dli_fname);
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
                Output::send<LogLevel::Error>(SYSSTR("Fatal Error: {}\n"), e->get_message());
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

struct link_map_private {
    Elf64_Addr l_addr;
    char *l_name;
    Elf64_Dyn *l_ld; 
    struct link_map_private *l_next, *l_prev;
    struct link_map_private *l_real;
    Lmid_t l_ns;
    struct libname_list *l_libname;
    Elf64_Dyn *l_info[DT_NUM + 0 + DT_VERSIONTAGNUM + DT_EXTRANUM + DT_VALNUM + DT_ADDRNUM];
};

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
    Dl_info dl_info;
    Elf64_Sym *sym;
    // libsteam_api is okay, the UE engine is causing problem
    void *current___cxa_throw = dlsym(RTLD_DEFAULT, "__cxa_throw");
    dladdr1(current___cxa_throw, &dl_info, (void**) &sym, RTLD_DL_SYMENT);
    fprintf(stderr, "__cxa_throw found in %s @ %p\n", dl_info.dli_fname, current___cxa_throw);
    if ((unsigned long)current___cxa_throw < 0x7fffffff) {
        fprintf(stderr, "maybe UE, patched\n");
        mprotect((void*)ALIGN_DOWN_PAGE(sym), ALIGN_UP_PAGE(sym + 1) - ALIGN_DOWN_PAGE(sym), PROT_READ | PROT_WRITE);
        sym->st_info = ELF64_ST_INFO(STB_LOCAL, ELF64_ST_TYPE(sym->st_info));
        sym->st_other = STV_INTERNAL;
        // assume this is UE5's address range and remove this symbol
        void *working___cxa_throw = dlsym(RTLD_DEFAULT, "__cxa_throw");
        struct link_map_private* map1;
        dladdr1(working___cxa_throw, &dl_info, (void**) &map1, RTLD_DL_SYMENT);
        fprintf(stderr, "now we're using __cxa_throw from %s @ %p\n", dl_info.dli_fname, working___cxa_throw);

        struct data_encap {
            void *working___cxa_throw;
            void *current___cxa_throw;
        } data = {working___cxa_throw, current___cxa_throw};
        dl_iterate_phdr([](struct dl_phdr_info *info, size_t size, void *data) -> int {
            auto working___cxa_throw = ((struct data_encap*)data)->working___cxa_throw;
            auto current___cxa_throw = ((struct data_encap*)data)->current___cxa_throw;
            fprintf(stderr, "Examing %s @ %p... \n", info->dlpi_name, info->dlpi_addr);
            // skip vdso 
            if (strstr(info->dlpi_name, "vdso") != nullptr) {
                fprintf(stderr, "  Skipping %s\n", info->dlpi_name);
                return 0;
            }
            // get dynamic
            Elf64_Dyn *dyn = nullptr;
            for (int i = 0; i < info->dlpi_phnum; i++) {
                if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
                    dyn = (Elf64_Dyn *)(info->dlpi_phdr[i].p_vaddr + info->dlpi_addr);
                }
            }

            // iterate dynamic, find relocation to __cxa_throw
            struct RELA_INFOS {
                Elf64_Rela *rela;
                ssize_t sz;
            } rela_infos[2] = {};
            // symbol table
            Elf64_Sym *sym = nullptr;
            // string table
            char *strtab = nullptr;
            for (int i = 0; dyn[i].d_tag != DT_NULL; i++) {
                // DT_JMPREL
                if (dyn[i].d_tag == DT_JMPREL) {
                    rela_infos[0].rela = (Elf64_Rela *)(dyn[i].d_un.d_ptr);
                }
                if (dyn[i].d_tag == DT_RELA) {
                    rela_infos[1].rela = (Elf64_Rela *)(dyn[i].d_un.d_ptr);
                }
                // DT_SYMTAB
                if (dyn[i].d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)(dyn[i].d_un.d_ptr);
                }
                // DT_STRTAB
                if (dyn[i].d_tag == DT_STRTAB) {
                    strtab = (char *)(dyn[i].d_un.d_ptr);
                }
                // DT_PLTRELSZ
                if (dyn[i].d_tag == DT_PLTRELSZ) {
                    rela_infos[0].sz = dyn[i].d_un.d_val;
                }
                // DT_RELASZ
                if (dyn[i].d_tag == DT_RELASZ) {
                    rela_infos[1].sz = dyn[i].d_un.d_val;
                }
            }
            if (sym == nullptr || strtab == nullptr) {
                fprintf(stderr, "  No sym or strtab found %p %p\n", sym, strtab);
                return 0;
            }
            if ((rela_infos[0].rela == nullptr) && (rela_infos[1].rela == nullptr)) {
                fprintf(stderr, "  No rela found %p %p\n", rela_infos[0].rela, rela_infos[1].rela);
                return 0;
            }
            auto process_rela = [&] (Elf64_Rela *rela, ssize_t sz, Elf64_Sym *sym, char *strtab) {
                fprintf(stderr, "  Found rela @ %p, sym @ %p, strtab @ %p\n", rela, sym, strtab);
                for (int i = 0; i < sz / sizeof(Elf64_Rela); i++) {
                    if (sym[ELF64_R_SYM(rela[i].r_info)].st_name != 0) {
                        char *name = strtab + sym[ELF64_R_SYM(rela[i].r_info)].st_name;
                        if (strncmp(name, "__cxa_throw", 11) == 0) {
                            fprintf(stderr, "  Found __cxa_throw symbol @ %p\n", rela[i].r_offset);
                        } else {
                            continue;
                        }
                        // got table address
                        unsigned long* address = (unsigned long *)(rela[i].r_offset + info->dlpi_addr);
                        if (*address == (unsigned long)current___cxa_throw) {
                            fprintf(stderr, "  Found __cxa_throw in .plt.got @ %p, offset = %d\n", address, rela[i].r_offset);
                            mprotect((void*)ALIGN_DOWN_PAGE(address), ALIGN_UP_PAGE(address + 1) - ALIGN_DOWN_PAGE(address), PROT_READ | PROT_WRITE);
                            *address = (unsigned long)working___cxa_throw;
                            fprintf(stderr, "  Patched __cxa_throw to @ %p\n", working___cxa_throw);
                        }
                    }
                }
            };
            for (int i = 0; i < 2; i++) {
                if (rela_infos[i].rela != nullptr) {
                    process_rela(rela_infos[i].rela, rela_infos[i].sz, sym, strtab);
                }
            }
            return 0;
        }, &data);
    
    }
    
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
