#ifdef _WIN32

#include <Helpers/Win32Error.hpp>

namespace RC
{
    template<> auto win32_error<wchar_t>::format_error(DWORD errorCode, DWORD dwLanguageId) -> wchar_t*
    {
        LPVOID pBuffer = nullptr;
        DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK;

        return FormatMessageW(dwFlags, nullptr, errorCode, dwLanguageId, reinterpret_cast<wchar_t*>(&pBuffer), 0, nullptr) == 0
             ? nullptr : static_cast<wchar_t*>(pBuffer);
    }

    template<> auto win32_error<char>::format_error(DWORD errorCode, DWORD dwLanguageId) -> char*
    {
        LPVOID pBuffer = nullptr;
        DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK;

        return FormatMessageA(dwFlags, nullptr, errorCode, dwLanguageId, reinterpret_cast<char*>(&pBuffer), 0, nullptr) == 0
             ? nullptr : static_cast<char*>(pBuffer);
    }
}

#endif
