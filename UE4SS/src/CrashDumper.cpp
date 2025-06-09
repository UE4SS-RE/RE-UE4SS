#include <CrashDumper.hpp>
#include <string>
#include <format>
#include <bit>
#include <UE4SSProgram.hpp>
#include <Unreal/Core/Windows/WindowsHWrapper.hpp>

#include <polyhook2/PE/IatHook.hpp>
#include <dbghelp.h>
#include <Helpers/Win32Error.hpp>
#include <String/StringType.hpp>

namespace fs = std::filesystem;

using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point_cast;

namespace RC
{
    const int DumpType =
            MiniDumpNormal | MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithModuleHeaders | MiniDumpWithAvxXStateContext;

    static bool FullMemoryDump = false;

    LONG WINAPI ExceptionHandler(_EXCEPTION_POINTERS* exception_pointers)
    {
        const auto now = time_point_cast<seconds>(system_clock::now());
        const StringType dump_path = fmt::format(STR("{}\\crash_{:%Y_%m_%d_%H_%M_%S}.dmp"), StringType{UE4SSProgram::get_program().get_working_directory()}, now);

        const HANDLE file =
                CreateFileW(FromCharTypePtr<wchar_t>(dump_path.c_str()), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (file == INVALID_HANDLE_VALUE)
        {
            const StringType message = fmt::format(STR("Failed to create crashdump file, reason: {}"), Win32Error(GetLastError()).c_str());
            MessageBoxW(NULL, FromCharTypePtr<wchar_t>(message.c_str()), L"Fatal Error!", MB_OK);
            return EXCEPTION_CONTINUE_SEARCH;
        }

        _MINIDUMP_EXCEPTION_INFORMATION exception_information{};
        exception_information.ThreadId = GetCurrentThreadId();
        exception_information.ExceptionPointers = exception_pointers;
        exception_information.ClientPointers = NULL;

        const int additional_dump_flags = FullMemoryDump ? MiniDumpWithFullMemory | MiniDumpIgnoreInaccessibleMemory : 0;
        bool ok = MiniDumpWriteDump(GetCurrentProcess(),
                                    GetCurrentProcessId(),
                                    file,
                                    static_cast<MINIDUMP_TYPE>(DumpType | additional_dump_flags),
                                    &exception_information,
                                    NULL,
                                    NULL);
        CloseHandle(file);

        if (!ok)
        {
            const StringType message = fmt::format(STR("Failed to write crashdump file, reason: {}"), Win32Error(GetLastError()).c_str());
            MessageBoxW(NULL, FromCharTypePtr<wchar_t>(message.c_str()), L"Fatal Error!", MB_OK);
            return EXCEPTION_CONTINUE_SEARCH;
        }

        const StringType message = fmt::format(STR("Crashdump written to: {}"), dump_path);
        MessageBoxW(NULL, FromCharTypePtr<wchar_t>(message.c_str()), L"Fatal Error!", MB_OK);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER filter)
    {
        return nullptr;
    }

    CrashDumper::CrashDumper()
    {
    }

    CrashDumper::~CrashDumper()
    {
        m_set_unhandled_exception_filter_hook->unHook();
        SetUnhandledExceptionFilter(reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(m_previous_exception_filter));
    }

    void CrashDumper::enable()
    {
        SetErrorMode(SEM_FAILCRITICALERRORS);
        m_previous_exception_filter = SetUnhandledExceptionFilter(ExceptionHandler);

        m_set_unhandled_exception_filter_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                               "SetUnhandledExceptionFilter",
                                                                               std::bit_cast<uint64_t>(&HookedSetUnhandledExceptionFilter),
                                                                               &m_hook_trampoline_set_unhandled_exception_filter_hook,
                                                                               L"");
        m_set_unhandled_exception_filter_hook->hook();
        this->enabled = true;
    }

    void CrashDumper::set_full_memory_dump(bool enabled)
    {
        FullMemoryDump = enabled;
    }

} // namespace RC
