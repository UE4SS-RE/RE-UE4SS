#include <CallStackDebug/CallStackDebug.hpp>
#include <Helpers/String.hpp>
#include <fmt/core.h>
#include <fmt/xchar.h>

#if RC_STACKTRACE_ENABLED
#include <stacktrace>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#else
#error "CallStackDebug is only supported on Windows"
#endif

namespace RC::CallStackDebug
{
    thread_local StringType s_call_stack_cache{};

    auto generate_new_call_stack() -> void
    {
        s_call_stack_cache.clear();
        const auto trace = std::stacktrace::current();
        // Skipping entry 0 because that's this function, and we're assuming the user is not interested in this code.
        for (size_t i = 1; i < trace.size(); ++i)
        {
            const auto& current_frame = trace[i];
            const auto original_address = reinterpret_cast<uintptr_t>(current_frame.native_handle());
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

    auto get_call_stack() -> StringType
    {
        if (s_call_stack_cache.empty())
        {
            generate_new_call_stack();
        }
        return s_call_stack_cache;
    }
} // namespace RC::PDB
