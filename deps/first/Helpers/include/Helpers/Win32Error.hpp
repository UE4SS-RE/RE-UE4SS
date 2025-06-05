#pragma once

#ifdef _WIN32

#include <windows.h>
#include <format>

namespace RC
{
    template<typename char_t>
    class win32_error
    {
    public:
        explicit win32_error(DWORD errorCode, DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
            : m_pText(nullptr)
        {
            assign(errorCode, dwLanguageId);
        }

        explicit win32_error(HRESULT hResult, DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
            : m_pText(nullptr)
        {
            assign(hResult, dwLanguageId);
        }

        ~win32_error()
        {
            clear();
        }

        auto clear() -> void
        {
            if (m_pText != nullptr)
            {
                ::LocalFree(m_pText);
                m_pText = nullptr;
            }
        }

        auto assign(DWORD errorCode, DWORD dwLanguageId) -> const char_t*
        {
            clear();
            m_pText = format_error(errorCode, dwLanguageId);

            if (m_pText == nullptr)
            {
                wprintf(L"Failed to format the error message for code 0x%08lX: error 0x%08lX\n", errorCode, GetLastError());
                m_pText = static_cast<char_t*>(LocalAlloc(0, 1));
                *m_pText = '\0';
            }

            return m_pText;
        }

        auto assign(HRESULT hResult, DWORD dwLanguageId) -> void
        {
            if (HRESULT_FACILITY(hResult) == FACILITY_WIN32)
            {
                assign(static_cast<DWORD>(hResult), dwLanguageId);
            }
            else
            {
                wprintf(L"Failed to format the error message: HRESULT 0x%08lX is not a WIN32 error code\n", hResult);
                m_pText = static_cast<char_t*>(LocalAlloc(0, 1));
                *m_pText = '\0';
            }
        }

        [[nodiscard]] auto c_str() const -> char_t* { return m_pText; }
        explicit operator char_t*() const { return c_str(); }
    private:
        static auto format_error(DWORD errorCode, DWORD dwLanguageId) -> char_t*;

        char_t* m_pText;
    };
}


#endif
