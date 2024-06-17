#pragma once

#include <codecvt>
#include <cwctype>
#include <locale>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <File/Macros.hpp>

#include <String/StringType.hpp>

namespace RC
{
    /* explode_by_occurrence -> START

    FUNCTION: explode_by_occurrence
    Find or explode a string by a delimiter

    The defaults for these functions are set in stone
    If you change them then you'll have to review every single usage in the entire solution
    */

    enum class ExplodeType
    {
        FromStart,
        FromEnd
    };

    template <typename CharT>
    auto inline explode_by_occurrence(const std::basic_string<CharT>& in_str_wide, const CharT delimiter, ExplodeType start_or_end) -> std::basic_string<CharT>
    {
        size_t occurrence = (start_or_end == ExplodeType::FromStart ? in_str_wide.find_first_of(delimiter) : in_str_wide.find_last_of(delimiter));

        std::basic_string<CharT> return_value;
        if (occurrence != std::basic_string<CharT>::npos)
        {
            return_value = start_or_end == ExplodeType::FromEnd ? in_str_wide.substr(occurrence + 1, std::basic_string<CharT>::npos) : in_str_wide.substr(0, occurrence);
        }
        else
        {
            return_value = in_str_wide;
        }

        return return_value;
    }

    template <typename CharT>
    auto inline explode_by_occurrence(const std::basic_string<CharT>& in_str, const char delimiter, const int32_t occurrence) -> std::basic_string<CharT>
    {
        size_t found_occurrence{};
        for (int64_t i = 0; i < std::count(in_str.begin(), in_str.end(), delimiter); i++)
        {
            found_occurrence = in_str.find(delimiter, found_occurrence + 1);
            if (i + 1 == occurrence)
            {
                return in_str.substr(0, found_occurrence);
            }
        }

        // No occurrence was found, returning empty string for now
        return {};
    }

    template <typename CharT>
    auto inline explode_by_occurrence(const std::basic_string<CharT>& in_str, const char delimiter) -> std::vector<std::basic_string<CharT>>
    {
        std::vector<std::basic_string<CharT>> result;

        size_t counter{};
        size_t start_offset{};

        for (const CharT* current_char = in_str.c_str(); *current_char; ++current_char)
        {
            if (*current_char == delimiter || counter == in_str.length() - 1)
            {
                std::basic_string<CharT> sub_str = in_str.substr(start_offset, counter - start_offset + (counter == in_str.length() - 1 ? 1 : 0));
                if (start_offset > 0)
                {
                    sub_str.erase(0, 1);
                }
                result.emplace_back(sub_str);

                start_offset = counter;
            }

            ++counter;
        }

        return result;
    }

    template <typename CharT>
    auto inline explode_by_occurrence(const std::basic_string<CharT>& in_str, const CharT delimiter, const int32_t occurrence) -> std::basic_string<CharT>
    {
        size_t found_occurrence{};
        for (int64_t i = 0; i < std::count(in_str.begin(), in_str.end(), delimiter); i++)
        {
            found_occurrence = in_str.find(delimiter, found_occurrence + 1);
            if (i + 1 == occurrence)
            {
                return in_str.substr(0, found_occurrence);
            }
        }

        // No occurrence was found, returning empty string for now
        return {};
    }
    /* explode_by_occurrence -> END */

    auto inline to_wstring(std::string& input) -> std::wstring
    {
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
        return converter.from_bytes(input);
#pragma warning(default : 4996)
    }

    auto inline to_wstring(std::string_view input) -> std::wstring
    {
#ifdef PLATFORM_WINDOWS
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
        return converter.from_bytes(input.data(), input.data() + input.length());
#pragma warning(default : 4996)
#else
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};
        return converter.from_bytes(input.data(), input.data() + input.length());
#endif
#if __clang__
#pragma clang diagnostic pop
#endif
    }

    auto inline to_const_wstring(std::string_view input) -> const std::wstring&
    {
        static std::unordered_map<std::string_view, std::wstring> wstringpool;
        static std::shared_mutex wstringpool_lock;

        // Allow multiple readers that are stalled when any thread is writing.
        {
            std::shared_lock<std::shared_mutex> read_guard(wstringpool_lock);
            if (wstringpool.contains(input)) return wstringpool[input];
        }

        auto temp_input = std::string{input};
        auto new_str = to_wstring(temp_input);

        // Stall the readers to insert a new string.
        {
            std::lock_guard<std::shared_mutex> write_guard(wstringpool_lock);
            const auto& [emplaced_iter, unused] = wstringpool.emplace(input, std::move(new_str));
            return emplaced_iter->second;
        }
    }

    auto inline to_wstring(std::wstring_view input) -> std::wstring
    {
        return std::wstring{input};
    }

    auto inline to_wstring(std::wstring& input) -> std::wstring
    {
        return std::wstring{input};
    }

    auto inline to_wstring(std::u16string& input) -> std::wstring
    {
#ifdef PLATFORM_WINDOWS
        return {input.begin(), input.end()};
#else
        throw std::runtime_error{"There is no reason to use this function on non-Windows platforms"};
#endif
    }

    auto inline to_wstring(std::u16string_view input) -> std::wstring
    {
#ifdef PLATFORM_WINDOWS
        return {input.begin(), input.end()};
#else
        throw std::runtime_error{"There is no reason to use this function on non-Windows platforms"};
#endif
    }
    
    auto inline to_string(std::wstring& input) -> std::string
    {
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
        return converter.to_bytes(input);
#pragma warning(default : 4996)
    }

    auto inline to_string(std::wstring_view input) -> std::string
    {
        auto temp_input = std::wstring{input};
        return to_string(temp_input);
    }

    auto inline to_string(std::u16string_view input) -> std::string
    {
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter{};
        return converter.to_bytes(input.data(), input.data() + input.length());
#pragma warning(default : 4996)
#if __clang__
#pragma clang diagnostic pop
#endif
    }

    auto inline to_u16string(std::wstring& input) -> std::u16string
    {
        return {input.begin(), input.end()};
    }

    auto inline to_u16string(std::wstring_view input) -> std::u16string
    {
        auto temp_input = std::wstring{input};
        return to_u16string(temp_input);
    }

    auto inline to_u16string(std::string& input) -> std::u16string
    {
        return {input.begin(), input.end()};
    }

    auto inline to_u16string(std::string_view input) -> std::u16string
    {
        auto temp_input = std::string{input};
        return to_u16string(temp_input);
    }

    auto inline to_generic_string(const auto& input) -> StringType
    {
        if constexpr (std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<decltype(input)>>>, StringViewType>)
        {
            return StringType{input};
        }
        else if constexpr (std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<decltype(input)>>>, StringType> ||
                           std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<decltype(input)>>>, CharType>)
        {
            return input;
        }
        else
        {
#if RC_IS_ANSI == 1
            return to_string(input);
#else
            return to_wstring(input);
#endif
        }
    }

    auto inline to_ue(std::string_view input)
    {
        if constexpr (std::is_same_v<CharType, wchar_t>)
        {
            return to_wstring(input);
        }
        else
        {
            return to_u16string(input);
        }
    }

    namespace String
    {
        auto inline iequal(std::wstring_view a, std::wstring_view b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const wchar_t a_char, const wchar_t b_char) {
                       return std::towlower(a_char) == std::towlower(b_char);
                   });
        }

        auto inline iequal(std::wstring& a, const wchar_t* b)
        {
            return iequal(a, std::wstring_view{b});
        }

        auto inline iequal(const wchar_t* a, std::wstring& b)
        {
            return iequal(std::wstring_view{a}, b);
        }

        auto inline iequal(std::string_view a, std::string_view b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const char a_char, const char b_char) {
                       return (std::tolower(a_char) == std::tolower(b_char));
                   });
        }

        auto inline iequal(std::string& a, const char* b)
        {
            return iequal(a, std::string_view{b});
        }

        auto inline str_cmp_insensitive(const char* a, std::string& b)
        {
            return iequal(std::string_view{a}, b);
        }
    } // namespace String
} // namespace RC
