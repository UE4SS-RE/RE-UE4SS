#pragma once

#ifdef _WIN32

namespace RC
{
    class Win32Error
    {
    public:
        explicit Win32Error(DWORD errorCode, const DWORD* dwLanguageId = nullptr);
        explicit Win32Error(HRESULT hResult, const DWORD* dwLanguageId = nullptr);

        auto assign(DWORD errorCode, DWORD dwLanguageId) -> void;
        auto assign(HRESULT hResult, DWORD dwLanguageId) -> void;

        [[nodiscard]] auto c_str() const -> const CharType* { return m_ErrorText.c_str(); }

        explicit operator const CharType*() const { return c_str(); } // must be explicit, or it will cause an ambiguous call to overloaded function error when passed to RC::to_string
        operator const StringType&() const { return m_ErrorText; } // implicit cast so it can be passed to RC::to_string
    private:
        static auto format_error(DWORD errorCode, DWORD dwLanguageId) -> CharType*;
        StringType m_ErrorText;
    };
}


#endif
