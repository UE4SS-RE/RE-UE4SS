#ifdef _WIN32
#include <windows.h>
#include <format>

#include <String/StringType.hpp>

#include "Helpers/Win32Error.hpp"

namespace RC
{
    Win32Error::Win32Error(DWORD errorCode, const DWORD *dwLanguageId)
    {
        const auto langId = dwLanguageId == nullptr ? MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) : *dwLanguageId;

        assign(errorCode, langId);
    }

    Win32Error::Win32Error(HRESULT hResult, const DWORD *dwLanguageId)
    {
        const auto langId = dwLanguageId == nullptr ? MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) : *dwLanguageId;

        assign(hResult, langId);
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
            m_ErrorText = std::format(L"Failed to format the error message for code {:x}: error {:x}\n", errorCode, GetLastError());
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
           m_ErrorText = std::format(L"Failed to format the error message: HRESULT 0x%08lX is not a WIN32 error code\n", hResult);
        }
    }

    auto Win32Error::format_error(DWORD errorCode, DWORD dwLanguageId) -> CharType*
    {
        DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK;
        LPVOID pBuffer = nullptr;

        return FormatMessage(dwFlags, nullptr, errorCode, dwLanguageId, reinterpret_cast<CharType*>(&pBuffer), 0, nullptr) == 0
             ? nullptr : static_cast<CharType*>(pBuffer);
    }
}

#endif
