#pragma once

#include <system_error>

namespace RC
{
    inline const std::error_category& default_error_category() noexcept
    {
#ifdef _WIN32
        return std::system_category();
#else
        return std::generic_category();
#endif
    }

#ifdef _WIN32
    using PlatformResultCode = HRESULT;
#else
    using PlatformResultCode = int;
#endif
}

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
         * @param error_code : a system error code
         * @param category : the error category (std::system_category() for OS specific codes or std::generic_category() for POSIX codes)
         */
        explicit SysError(int error_code, const std::error_category& category = default_error_category());
        /**
         * Constructor for error code
         * @param error_code : a system error code
        * @param category : the error category (std::system_category() for OS specific codes or std::generic_category() for POSIX codes)
         */
        explicit SysError(unsigned long error_code, const std::error_category& category = default_error_category());
        /**
         * Assign an error code and update the object with the corresponding error message
         * @param error_code : a system error code
         */
        auto assign(unsigned long error_code) -> void;
#ifdef _WIN32
        /**
         * Constructor for COM HRESULT codes -- only works for FACILITY_WIN32
         * @param hresult : a COM operation result code
         */
        explicit SysError(HRESULT hresult);

        /**
         * Assign an error code and update the object with the corresponding error message
         * @param hresult : a COM operation result code
         */
        auto assign(HRESULT hresult) -> void;
#endif
        /**
         * Returns a pointer to an array that contains a null-terminated sequence of characters representing the current value of the string object
         * @return a pointer to the c-string representation of the string object's value
         */
        [[nodiscard]] auto c_str() const noexcept -> const CharType* { return m_error_text.c_str(); }
        /**
         * Returns the name of the error category
         * @return the name of the error category
         */
        [[nodiscard]] auto category() const -> StringType;
        /**
         * Explicit cast operator to const CharType*
         */
        explicit operator const CharType*() const noexcept { return c_str(); } // must be explicit, or it will cause an ambiguous call to overloaded function error when passed to RC::to_string
        /**
         * Implicit cast operator to const StringType& so that the wrapper can be passed to RC::to_string, for instance
         */
        operator const StringType&() const noexcept { return m_error_text; }
    private:
        /**
         * Formats the error message corresponding to the specified error code.
         * @param error_code : a system error code or COM error code
         * @return a string containing the formatted message, if successful
         */
        [[nodiscard]] auto format_error(int error_code) const -> StringType;
        /**
         * The error category (std::system_category() for OS specific codes or std::generic_category() for POSIX codes)
         */
        const std::error_category* m_error_category = nullptr;
        /**
         * The string object backing the wrapper
         */
        StringType m_error_text;
    };
}
