#pragma once

#include <codecvt>
#include <cwctype>
#include <locale>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

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

    // Auto String Conversion

    // All possible char types in this project
    template <typename T>
    struct _can_be_string_view_t : std::false_type
    {
    };
    template <>
    struct _can_be_string_view_t<wchar_t*> : std::true_type
    {
    };
    template <>
    struct _can_be_string_view_t<char*> : std::true_type
    {
    };
    template <>
    struct _can_be_string_view_t<char16_t*> : std::true_type
    {
    };
    template <>
    struct _can_be_string_view_t<const wchar_t*> : std::true_type
    {
    };
    template <>
    struct _can_be_string_view_t<const char*> : std::true_type
    {
    };
    template <>
    struct _can_be_string_view_t<const char16_t*> : std::true_type
    {
    };

    template <typename T>
    struct can_be_string_view_t : _can_be_string_view_t<std::decay_t<T>>
    {
    };

    template <typename T>
    struct is_string_like_t : std::false_type
    {
    };

    template <typename CharT>
    struct is_string_like_t<std::basic_string<CharT>> : std::true_type
    {
        // T is a string or string view of CharT
    };

    template <typename CharT>
    struct is_string_like_t<std::basic_string_view<CharT>> : std::true_type
    {
        // T is a string or string view of CharT
    };

    template <typename T, typename CharT>
    struct is_charT_string_type : std::disjunction<std::is_same<T, std::basic_string<CharT>>, std::is_same<T, std::basic_string_view<CharT>>>
    {
        // T is a string or string view of CharT
    };

    template <typename T, typename CharT>
    struct not_charT_string_like_t : std::conjunction<is_string_like_t<std::decay_t<T>>, std::negation<is_charT_string_type<std::decay_t<T>, CharT>>>
    {
        // 1. T is a string or string view
        // 2. T is not a string or string view of CharT
    };

    template <typename T>
    auto stringviewify(T&& tp)
    {
        // Convert a char pointer to a string view
        return std::basic_string_view<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>>{tp};
    }

    template <typename CharT, typename T>
    auto inline to_charT_string(T&& arg) -> std::basic_string<CharT> {
        // Dispatch to the correct conversion function based on the CharT type
        if constexpr (std::is_same_v<CharT, wchar_t>) {
            return to_wstring(std::forward<T>(arg));
        } else if constexpr (std::is_same_v<CharT, char16_t>) {
            return to_u16string(std::forward<T>(arg));
        } else if constexpr (std::is_same_v<CharT, char>) {
            return to_string(std::forward<T>(arg));
        }
    }

    template <typename CharT, typename T>
    auto inline to_charT_string_path(T&& arg) -> std::basic_string<CharT> {
        // Dispatch to the correct conversion function based on the CharT type
        if constexpr (std::is_same_v<CharT, wchar_t>) {
            return arg.wstring();
        } else if constexpr (std::is_same_v<CharT, char16_t>) {
            return arg.u16string();
        } else if constexpr (std::is_same_v<CharT, char>) {
            return arg.string();
        }
    }

    // Convert any string-like to a string of generic CharT
    // Or pass through if it's already a string(view) of CharType or if we can't convert it
    template<typename CharT, typename T>
    auto inline to_charT(T&& arg) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::filesystem::path> || std::is_same_v<std::decay_t<T>, const std::filesystem::path>) {
            // std::filesystem::path has its own conversion functions
            return to_charT_string_path<CharT>(std::forward<T>(arg));
        } else if constexpr (can_be_string_view_t<T>::value) {
            // If T is a char pointer, it can be treated as a string view
            return to_charT<CharT>(stringviewify(std::forward<T>(arg)));
        } else if constexpr (not_charT_string_like_t<T, CharT>::value) {
            // If T is a string or string view but not using CharT, convert it
            return to_charT_string<CharT>(std::forward<T>(arg));
        } else {
            // If T is already a string or string view using CharT, pass through
            // Or if we can't convert it, pass through
            return std::forward<T>(arg);
        }
    }

    // Convert any string-like to a string(view) of CharType that is used in UE
    // Or pass through if it's already a string(view) of CharType or if we can't convert it
    template<typename T>
    auto inline to_ue(T&& arg) {
        return to_charT<CharType>(std::forward<T>(arg));
    }

    // You can add more to_* function if needed

    // Auto Type Conversion Done

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
            return to_ue(input);
#endif
        }
    }
    auto inline to_const_ue(std::string_view input) -> const StringType&
    {
        static std::unordered_map<std::string_view, StringType> uestringpool;
        static std::shared_mutex uestringpool_lock;

        // Allow multiple readers that are stalled when any thread is writing.
        {
            std::shared_lock<std::shared_mutex> read_guard(uestringpool_lock);
            if (uestringpool.contains(input)) return uestringpool[input];
        }

        auto temp_input = std::string{input};
        auto new_str = to_ue(temp_input);

        // Stall the readers to insert a new string.
        {
            std::lock_guard<std::shared_mutex> write_guard(uestringpool_lock);
            const auto& [emplaced_iter, unused] = uestringpool.emplace(input, std::move(new_str));
            return emplaced_iter->second;
        }
    }
    
    namespace String
    {
        template <typename CharT>
        auto inline iequal(std::basic_string_view<CharT> a, std::basic_string_view<CharT> b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const CharT a_char, const CharT b_char) {
                       return std::towlower((wchar_t) a_char) == std::towlower((wchar_t) b_char);
                   });
        }

        template <typename CharT>
        auto inline iequal(std::basic_string<CharT>& a, const CharT* b)
        {
            return iequal(std::basic_string_view<CharT>{a}, std::basic_string_view<CharT>{b});
        }

        template <typename CharT>
        auto inline iequal(const CharT* a, std::basic_string<CharT>& b)
        {
            return iequal(std::basic_string_view<CharT>{a}, std::basic_string_view<CharT>{b});
        }

        template <typename CharT>
        auto inline str_cmp_insensitive(const CharT* a, std::basic_string<CharT>& b)
        {
            return iequal(std::basic_string_view<CharT>{a}, std::basic_string_view<CharT>{b});
        }
    } // namespace String
} // namespace RC
