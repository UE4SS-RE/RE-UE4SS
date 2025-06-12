#ifdef _WIN32
    #include <windows.h>
#endif

#include <algorithm>
#include <ranges>
#include <format>
#include <regex>

#include <String/StringType.hpp>

#include "Helpers/SysError.hpp"
#include "Helpers/String.hpp"

namespace RC
{
    SysError::SysError(const int error_code, const std::error_category &category)
        : m_error_category(&category)
    {
        assign(static_cast<unsigned long>(error_code));
    }

    SysError::SysError(const unsigned long error_code, const std::error_category& category)
        : m_error_category(&category)
    {
        assign(error_code);
    }

    auto SysError::assign(unsigned long error_code) -> void
    {
        m_error_text.assign(std::format(L"[0x{:x}] {}", error_code, format_error(static_cast<int>(error_code))));
    }

#ifdef _WIN32
    SysError::SysError(const HRESULT hresult): m_error_category(&std::system_category())
    {
        assign(hresult);
    }

    auto SysError::assign(HRESULT hresult) -> void
    {
        if (HRESULT_FACILITY(hresult) == FACILITY_WIN32)
        {
            assign(static_cast<unsigned long>(hresult));
        }
        else
        {
           // this error code will most likely be formatted to an unrelated error message, inform the caller
           m_error_text.assign(std::format(L"Failed to format the error message: HRESULT 0x{:x} is not a WIN32 error code", hresult));
        }
    }
#endif

    auto SysError::category() const -> StringType
    {
        return to_wstring(m_error_category ? m_error_category->name() : DEFAULT_ERROR_CATEGORY.name());
    }

    auto SysError::format_error(const int error_code) const -> StringType
    {
        const std::error_category& error_category = m_error_category ? *m_error_category : DEFAULT_ERROR_CATEGORY;
        const std::error_code ec(error_code, error_category);
        // remove new line(s) and tabs
        auto result = std::regex_replace(to_wstring(std::system_error(ec).what()), std::wregex(STR("(\t|\r?\n)")), STR(" "));
        // right trim
        result.erase(std::ranges::find_if(std::ranges::reverse_view(result), [](const CharType c) -> bool {
            return !std::isspace<CharType>(c, std::locale::classic());
        }).base(), result.end());

        return result;
    }
}
