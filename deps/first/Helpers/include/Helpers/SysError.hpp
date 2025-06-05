#pragma once

#include <system_error>

#ifdef _WIN32
    #define DEFAULT_ERROR_CATEGORY std::system_category()
#else
    #define DEFAULT_ERROR_CATEGORY std::generic_category()
#endif

namespace RC
{
    /**
     *  Wrapper for error messages from system error code to be used as a string object
     */
    class SysError
    {
    public:
        /**
         * Constructor for error code
         * @param errorCode : a system error code
         * @param category : the error category (std::system_category() for OS specific codes or std::generic_category() for POSIX codes)
         */
        explicit SysError(int errorCode, const std::error_category &category = DEFAULT_ERROR_CATEGORY);
        /**
         * Constructor for error code
         * @param errorCode : a system error code
        * @param category : the error category (std::system_category() for OS specific codes or std::generic_category() for POSIX codes)
         */
        explicit SysError(unsigned long errorCode, const std::error_category &category = DEFAULT_ERROR_CATEGORY);
        /**
         * Assign an error code and update the object with the corresponding error message
         * @param errorCode : a system error code
         */
        auto assign(unsigned long errorCode) -> void;
#ifdef _WIN32
        /**
         * Constructor for COM HRESULT codes -- only works for FACILITY_WIN32
         * @param hResult : a COM operation result code
         */
        explicit SysError(HRESULT hResult);

        /**
         * Assign an error code and update the object with the corresponding error message
         * @param hResult : a COM operation result code
         */
        auto assign(HRESULT hResult) -> void;
#endif
        /**
         * Returns a pointer to an array that contains a null-terminated sequence of characters representing the current value of the string object
         * @return a pointer to the c-string representation of the string object's value
         */
        [[nodiscard]] auto c_str() const -> const CharType* { return m_ErrorText.c_str(); }
        /**
         * Returns the name of the error category
         * @return the name of the error category
         */
        [[nodiscard]] auto category() const -> StringType;
        /**
         * Explicit cast operator to const CharType*
         */
        explicit operator const CharType*() const { return c_str(); } // must be explicit, or it will cause an ambiguous call to overloaded function error when passed to RC::to_string
        /**
         * Implicit cast operator to const StringType& so that the wrapper can be passed to RC::to_string, for instance
         */
        operator const StringType&() const { return m_ErrorText; }
        /**
         * The error category (std::system_category() for OS specific codes or std::generic_category() for POSIX codes)
         */
        const std::error_category* m_pErrorCategory = nullptr;
    private:
        /**
         * Formats the error message corresponding to the specified error code.
         * @param errorCode : a system error code or COM error code
         * @return a string containing the formatted message, if successful
         */
        auto format_error(int errorCode) const -> StringType;
        /**
         * The string object backing the wrapper
         */
        StringType m_ErrorText;
    };
}
