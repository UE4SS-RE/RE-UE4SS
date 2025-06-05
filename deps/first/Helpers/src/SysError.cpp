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
    SysError::SysError(const int errorCode, const std::error_category &category)
        : m_pErrorCategory(&category)
    {
        assign(static_cast<unsigned long>(errorCode));
    }

    SysError::SysError(const unsigned long errorCode, const std::error_category &category)
        : m_pErrorCategory(&category)
    {
        assign(errorCode);
    }

    auto SysError::assign(unsigned long errorCode) -> void
    {
        m_ErrorText.assign(std::format(L"[0x{:x}] {}", errorCode, format_error(static_cast<int>(errorCode))));
    }

#ifdef _WIN32
    SysError::SysError(HRESULT hResult): m_pErrorCategory(&std::system_category())
    {
        assign(hResult);
    }

    auto SysError::assign(HRESULT hResult) -> void
    {
        if (HRESULT_FACILITY(hResult) == FACILITY_WIN32)
        {
            assign(static_cast<unsigned long>(hResult));
        }
        else
        {
           // this error code will most likely be formatted to an unrelated error message, inform the caller
           m_ErrorText.assign(std::format(L"Failed to format the error message: HRESULT 0x{:x} is not a WIN32 error code", hResult));
        }
    }
#endif

    auto SysError::category() const -> StringType
    {
        return to_wstring(m_pErrorCategory ? m_pErrorCategory->name() : DEFAULT_ERROR_CATEGORY.name());
    }

    auto SysError::format_error(const int errorCode) const -> StringType
    {
        const std::error_category& errorCategory = m_pErrorCategory ? *m_pErrorCategory : DEFAULT_ERROR_CATEGORY;
        const std::error_code ec(errorCode, errorCategory);
        // remove new line(s) and tabs
        auto result = std::regex_replace(to_wstring(std::system_error(ec).what()), std::wregex(STR("(\t|\r?\n)")), STR(" "));
        // right trim
        result.erase(std::ranges::find_if(std::ranges::reverse_view(result), [](const CharType ch) -> bool {
            return !std::isspace<CharType>(ch, std::locale::classic());
        }).base(), result.end());

        return result;
    }
}
