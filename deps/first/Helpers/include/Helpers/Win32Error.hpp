#pragma once

#ifdef _WIN32

namespace RC
{
    /**
     *  Wrapper for WinAPI error code to be used as a string object
     */
    class Win32Error
    {
    public:
        /**
         * Constructor for error codes coming from GetLastError()
         * @param errorCode : a WinAPI error code
         */
        explicit Win32Error(DWORD errorCode);
        /**
         * Constructor for COM HRESULT codes -- only works for FACILITY_WIN32
         * @param hResult : a COM operation result code
         */
        explicit Win32Error(HRESULT hResult);

        /**
         * Assign an error code and update the object with the corresponding error message
         * @param errorCode : a WinAPI error code from GetLastError()
         * @param dwLanguageId : the language identifier for the requested message passed to FormatMessage
         */
        auto assign(DWORD errorCode, DWORD dwLanguageId) -> void;
        /**
         * Assign an error code and update the object with the corresponding error message
         * @param hResult : a COM operation result code
         * @param dwLanguageId : the language identifier for the requested message passed to FormatMessage
         */
        auto assign(HRESULT hResult, DWORD dwLanguageId) -> void;
        /**
         * Returns a pointer to an array that contains a null-terminated sequence of characters representing the current value of the string object
         * @return a pointer to the c-string representation of the string object's value
         */
        [[nodiscard]] auto c_str() const -> const CharType* { return m_ErrorText.c_str(); }
        /**
         * Explicit cast operator to const CharType*
         */
        explicit operator const CharType*() const { return c_str(); } // must be explicit, or it will cause an ambiguous call to overloaded function error when passed to RC::to_string
        /**
         * Implicit cast operator to const StringType& so that the wrapper can be passed to RC::to_string, for instance
         */
        operator const StringType&() const { return m_ErrorText; }
    private:
        /**
         * Formats the error message corresponding to the specified error code.
         * The allocation is handled by FormatMessage itself and the resulting pointer must be freed by calling LocalFree.
         * @param errorCode : a WinAPI or COM error code
         * @param dwLanguageId : the language identifier for the requested message passed to FormatMessage
         * @return a buffer containing the formatted message if successful; nullptr otherwise
         */
        static auto format_error(DWORD errorCode, DWORD dwLanguageId) -> CharType*;
        /**
         * The string object backing the wrapper
         */
        StringType m_ErrorText;
    };
}


#endif
