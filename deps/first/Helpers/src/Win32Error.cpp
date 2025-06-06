#ifdef _WIN32
#include <windows.h>
#include <format>

#include <String/StringType.hpp>

#include "Helpers/Win32Error.hpp"

namespace RC
{
    Win32Error::Win32Error(DWORD errorCode)
    {
        static constexpr auto dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

        assign(errorCode, dwLanguageId);
    }

    Win32Error::Win32Error(HRESULT hResult)
    {
        static constexpr auto dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

        assign(hResult, dwLanguageId);
    }

    auto Win32Error::assign(DWORD errorCode, DWORD dwLanguageId) -> void
    {
        if (auto pErrorText = format_error(errorCode, dwLanguageId); pErrorText != nullptr)
        {
            m_ErrorText = std::format(L"[0x{:x}] {}", errorCode, pErrorText);
            LocalFree(pErrorText);
        }
        else
        {
            // the call to FormatMessage failed, but we can attempt to find out why
            const auto formatErrorCode = GetLastError();

            if (auto pFormatErrorText = format_error(formatErrorCode, dwLanguageId); pFormatErrorText != nullptr)
            {
                // we couldn't format the message for the initial error code, but we can inform the caller as to why
                m_ErrorText = std::format(L"Failed to format the error message for code 0x{:x}: error {}\n", errorCode, pFormatErrorText);
                LocalFree(pFormatErrorText);
            }
            else
            {
                // probably unlikely that the second call to FormatMessage fails, however fall back to the error code should it happen
                m_ErrorText = std::format(L"Failed to format the error message for code 0x{:x}: error code 0x{:x}\n", errorCode, formatErrorCode);
            }
        }
    }

    auto Win32Error::assign(HRESULT hResult, DWORD dwLanguageId) -> void
    {
        if (HRESULT_FACILITY(hResult) == FACILITY_WIN32)
        {
            assign(static_cast<DWORD>(hResult), dwLanguageId);
        }
        else
        {
           // this error code will most likely be formatted to an unrelated error message, inform the caller
           m_ErrorText = std::format(L"Failed to format the error message: HRESULT 0x{:x} is not a WIN32 error code\n", hResult);
        }
    }

    auto Win32Error::format_error(DWORD errorCode, DWORD dwLanguageId) -> CharType*
    {
        // see https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage#parameters
        DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM      // search the system message-table resource(s) for the requested message
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER  // allocates the buffer, use LocalFree to free the result
                      | FORMAT_MESSAGE_IGNORE_INSERTS   // formatting placeholders are ignored (we pass nullptr for the Arguments parameter)
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK;  // ignore regular line breaks in the message definition text
        LPVOID pBuffer = nullptr;
        // format the system message corresponding to the error code
        return FormatMessage(dwFlags, nullptr, errorCode, dwLanguageId, reinterpret_cast<CharType*>(&pBuffer), 0, nullptr) == 0
             ? nullptr : static_cast<CharType*>(pBuffer);
    }
}

#endif
