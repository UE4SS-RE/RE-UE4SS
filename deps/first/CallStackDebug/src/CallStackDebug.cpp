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
    auto generate_call_stack() -> StringType
    {
        StringType call_stack_string{};
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
                    return fmt::format(STR("Error when generating call stack: DuplicateHandle returned error: 0x{:X}\n"), GetLastError());
                }
                if (!SymInitialize(debug_handle, nullptr, true))
                {
                    return fmt::format(STR("Error when generating call stack: SymInitialize returned error: 0x{:X}\n"), GetLastError());
                }
                call_stack_string.append(fmt::format(STR("[{}] "), i - 1));
                char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
                auto symbol_info = reinterpret_cast<PSYMBOL_INFO>(buffer);
                symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol_info->MaxNameLen = MAX_SYM_NAME;
                DWORD64 sym_from_addr_displacement{};
                if (SymFromAddr(debug_handle, original_address, &sym_from_addr_displacement, symbol_info))
                {
                    const auto name_ascii_view = symbol_info->Name;
                    call_stack_string.append(ensure_str(name_ascii_view));
                    IMAGEHLP_LINE64 line{};
                    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                    DWORD sym_get_line_from_addr64_displacement{};
                    if (SymGetLineFromAddr64(debug_handle, original_address, &sym_get_line_from_addr64_displacement, &line))
                    {
                        call_stack_string.append(fmt::format(STR(" in '{}' @ L{}"), ensure_str(line.FileName), line.LineNumber));
                    }
                }
                else
                {
                    call_stack_string.append(STR("Unknown"));
                }
                call_stack_string.append(STR("\n"));
            }
        }
        return call_stack_string;
    }
} // namespace RC::PDB
