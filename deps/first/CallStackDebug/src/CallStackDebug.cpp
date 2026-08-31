#include <CallStackDebug/CallStackDebug.hpp>
#include <Helpers/String.hpp>
#include <fmt/core.h>
#include <fmt/xchar.h>

#if RC_STACKTRACE_ENABLED
#include <stacktrace>
#endif
#include <ranges>
#include <functional>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#else
#error "CallStackDebug is only supported on Windows"
#endif

namespace RC::CallStackDebug
{
    thread_local StringType s_call_stack_cache{};

    template <typename Callable>
    struct Cleanup
    {
        Callable cleanup_callable{};
        ~Cleanup()
        {
            cleanup_callable();
        }
    };

    static constexpr auto s_module_unknown = std::pair<uintptr_t, StringType>{std::numeric_limits<uintptr_t>::max(), {}};
    static constexpr auto s_module_unknown_by_name = std::numeric_limits<uintptr_t>::max();

    // Returns module base address and name that the given address belongs to.
    static auto get_module_from_address(void* in_address) -> std::pair<uintptr_t, StringType>
    {
        const auto address = reinterpret_cast<uintptr_t>(in_address);
        HMODULE modules[1024]{};
        DWORD bytes_required{};
        if (K32EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &bytes_required) == 0)
        {
            return s_module_unknown;
        }

        for (size_t i = 0; i < bytes_required / sizeof(HMODULE); ++i)
        {
            MODULEINFO info{};
            K32GetModuleInformation(GetCurrentProcess(), modules[i], &info, sizeof(MODULEINFO));
            const auto base = reinterpret_cast<uintptr_t>(info.lpBaseOfDll);
            if (address >= base && address <= base + info.SizeOfImage)
            {
                char module_raw_name[MAX_PATH];
                if (K32GetModuleBaseNameA(GetCurrentProcess(), modules[i], module_raw_name, sizeof(module_raw_name) / sizeof(char)) == 0)
                {
                    continue;
                }
                const auto module_name = ensure_str(module_raw_name);
                return {base, module_name};
            }
        }

        return s_module_unknown;
    }

    // Returns the base address of a module by name.
    static auto get_module_base_from_name(StringViewType name) -> uintptr_t
    {
        HMODULE modules[1024]{};
        DWORD bytes_required{};
        if (K32EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &bytes_required) == 0)
        {
            return s_module_unknown_by_name;
        }

        for (size_t i = 0; i < bytes_required / sizeof(HMODULE); ++i)
        {
            char module_raw_name[MAX_PATH];
            if (K32GetModuleBaseNameA(GetCurrentProcess(), modules[i], module_raw_name, sizeof(module_raw_name) / sizeof(char)) == 0)
            {
                continue;
            }
            const auto module_name = ensure_str(module_raw_name);
            if (name != module_name)
            {
                continue;
            }

            MODULEINFO info{};
            K32GetModuleInformation(GetCurrentProcess(), modules[i], &info, sizeof(MODULEINFO));
            const auto base = reinterpret_cast<uintptr_t>(info.lpBaseOfDll);
            return base;
        }

        return s_module_unknown_by_name;
    }

    static auto iterate_stack_trace(HANDLE debug_handle, auto&& trace, auto frame_resolver) -> void
    {
        // Skipping entry 0 because that's this function, and we're assuming the user is not interested in this code.
        for (size_t i = 1; i < trace.size(); ++i)
        {
            const auto& current_frame = trace[i];
            const uintptr_t original_address = frame_resolver(current_frame);
            {
                s_call_stack_cache.append(fmt::format(STR("[{}] "), i - 1));
                char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
                auto symbol_info = reinterpret_cast<PSYMBOL_INFO>(buffer);
                symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol_info->MaxNameLen = MAX_SYM_NAME;
                DWORD64 sym_from_addr_displacement{};
                if (SymFromAddr(debug_handle, original_address, &sym_from_addr_displacement, symbol_info))
                {
                    const auto name_ascii_view = symbol_info->Name;
                    s_call_stack_cache.append(ensure_str(name_ascii_view));
                    IMAGEHLP_LINE64 line{};
                    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                    DWORD sym_get_line_from_addr64_displacement{};
                    if (SymGetLineFromAddr64(debug_handle, original_address, &sym_get_line_from_addr64_displacement, &line))
                    {
                        s_call_stack_cache.append(fmt::format(STR(" in '{}' @ L{}"), ensure_str(line.FileName), line.LineNumber));
                    }
                }
                else
                {
                    s_call_stack_cache.append(STR("Unknown"));
                }
                s_call_stack_cache.append(STR("\n"));
            }
        }
    }

    auto generate_new_call_stack(const CallStackWithoutSymbols& call_stack) -> void
    {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        const HANDLE current_process = GetCurrentProcess();
        HANDLE debug_handle{};
        if (!DuplicateHandle(current_process, current_process, current_process, &debug_handle, 0, false, DUPLICATE_SAME_ACCESS))
        {
            s_call_stack_cache = fmt::format(STR("Error when generating call stack: DuplicateHandle returned error: 0x{:X}\n"), GetLastError());
            return;
        }
        if (!SymInitialize(debug_handle, nullptr, true))
        {
            s_call_stack_cache = fmt::format(STR("Error when generating call stack: SymInitialize returned error: 0x{:X}\n"), GetLastError());
            return;
        }
        auto cleanup = Cleanup([&]() {
            SymCleanup(debug_handle);
        });
        s_call_stack_cache.clear();
        if (!call_stack.empty())
        {
            iterate_stack_trace(debug_handle, call_stack, [](const ModuleNameAndOffsetPair& pair) {
                return get_module_base_from_name(pair.first) + pair.second;
            });
        }
        else
        {
            const auto trace = std::stacktrace::current();
            iterate_stack_trace(debug_handle, trace, [](const std::stacktrace_entry& entry) {
                return reinterpret_cast<uintptr_t>(entry.native_handle());
            });
        }
    }

    auto get_call_stack() -> StringType
    {
        if (s_call_stack_cache.empty())
        {
            generate_new_call_stack();
        }
        return s_call_stack_cache;
    }


    auto get_unsymbolized_call_stack() -> CallStackWithoutSymbols
    {
        CallStackWithoutSymbols call_stack{};
        const auto trace = std::stacktrace::current();
        for (const auto& entry : trace)
        {
            const auto [module_base, module_name] = get_module_from_address(entry.native_handle());
            const auto module_offset = reinterpret_cast<uintptr_t>(entry.native_handle()) - module_base;
            call_stack.emplace_back(module_name, module_offset);
        }
        return call_stack;
    }

    auto symbolicate_call_stack(const CallStackWithoutSymbols& call_stack) -> StringType
    {
        generate_new_call_stack(call_stack);
        return s_call_stack_cache;
    }
} // namespace RC::PDB
