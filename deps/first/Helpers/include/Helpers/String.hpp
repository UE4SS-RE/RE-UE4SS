#pragma once

#include <codecvt>
#include <cwctype>
#include <locale>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <cassert>

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
            return_value = start_or_end == ExplodeType::FromEnd ? in_str_wide.substr(occurrence + 1, std::basic_string<CharT>::npos)
                                                                : in_str_wide.substr(0, occurrence);
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

    /**
     * Breaks an input string into a vector of substrings based on a given delimiter.
     * <br>It treats sections that start with a delimiter character and enclosed in double quotes (`"`) as a single substring, ignoring any delimiters inside
     * the quotes. <br>It supports an escape character (default `\`) to capture double quotes as part of a string.
     *
     * @tparam CharT The character type.
     * @param in_str  The input string to be split.
     * @param delimiter (optional) The character used to split the string. Default: ` `.
     * @param escape_character (optional) The character used for escaping quotes. Default: `\`.
     * @return A vector of substrings, split by the delimiter, with quoted substrings preserved.
     */
    template <typename CharT>
    auto inline explode_by_occurrence_with_quotes(const std::basic_string<CharT>& in_str,
                                                  const CharT delimiter = STR(' '),
                                                  const CharT escape_character = STR('\\')) -> std::vector<std::basic_string<CharT>>
    {
        constexpr auto quotation_symbol = STR('"');
        assert(delimiter != quotation_symbol && "Double quote (\") can't be used as delimiter");
        assert(delimiter != escape_character && "Delimiter can't be the same as the escape character");
        assert(escape_character != quotation_symbol && "Double quote (\") can't be used as escape character");

        std::vector<std::basic_string<CharT>> result;
        std::basic_string<CharT> current;
        auto in_quotes = false;

        auto add_current_to_vector = [&result, &current]() {
            if (!current.empty())
            {
                result.push_back(current);
                current.clear();
            }
        };

        auto is_valid_quote_symbol = [&in_quotes, &in_str, &delimiter, &escape_character](int position) {
            if (position >= 0 && position < in_str.size() && in_str[position] == quotation_symbol && (position == 0 || in_str[position - 1] != escape_character))
            {
                return in_quotes ? position + 1 == in_str.size() || in_str[position + 1] == delimiter : position == 0 || in_str[position - 1] == delimiter;
            }
            return false;
        };

        for (size_t i = 0; i < in_str.size(); i++)
        {
            if (is_valid_quote_symbol(i))
            {
                in_quotes = !in_quotes;
                continue;
            }
            const auto current_char = in_str[i];
            if ((!in_quotes && current_char == delimiter))
            {
                add_current_to_vector();
                continue;
            }
            if (current_char != escape_character || in_str[i + 1] != quotation_symbol)
            {
                current.push_back(current_char);
            }
        }
        add_current_to_vector();

        return result;
    }
    /* explode_by_occurrence -> END */

    auto inline to_wstring(const std::string& input) -> std::wstring
    {
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
        return converter.from_bytes(input);
#pragma warning(default : 4996)
    }

    auto inline to_wstring(const char* pInput)
    {
        auto temp_input = std::string{pInput};
        return to_wstring(temp_input);
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

    auto inline to_wstring(const std::wstring& input) -> std::wstring
    {
        return std::wstring{input};
    }

    auto inline to_wstring(const std::u16string& input) -> std::wstring
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

    auto inline to_string(const std::wstring& input) -> std::string
    {
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
        return converter.to_bytes(input);
#pragma warning(default : 4996)
    }

    auto inline to_string(const wchar_t* pInput)
    {
        auto temp_input = std::wstring{pInput};
        return to_string(temp_input);
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

    auto inline to_u16string(const std::wstring& input) -> std::u16string
    {
        return {input.begin(), input.end()};
    }

    auto inline to_u16string(std::wstring_view input) -> std::u16string
    {
        auto temp_input = std::wstring{input};
        return to_u16string(temp_input);
    }

    auto inline to_u16string(const std::string& input) -> std::u16string
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
    auto inline to_charT_string(T&& arg) -> std::basic_string<CharT>
    {
        // Dispatch to the correct conversion function based on the CharT type
        if constexpr (std::is_same_v<CharT, wchar_t>)
        {
            return to_wstring(std::forward<T>(arg));
        }
        else if constexpr (std::is_same_v<CharT, char16_t>)
        {
            return to_u16string(std::forward<T>(arg));
        }
        else if constexpr (std::is_same_v<CharT, char>)
        {
            return to_string(std::forward<T>(arg));
        }
    }

    template <typename CharT, typename T_Path>
    auto inline to_charT_string_path(T_Path&& arg) -> std::basic_string<CharT>
    {
        static_assert(std::is_same_v<std::decay_t<T_Path>, std::filesystem::path>, "Input must be std::filesystem::path");

        if constexpr (std::is_same_v<CharT, wchar_t>) // Covers WIDECHAR and Windows TCHAR
        {
            return arg.wstring();
        }
        else if constexpr (std::is_same_v<CharT, char>) // Covers ANSICHAR, for UTF-8 output
        {
            // For UTF-8 std::string output from path
            std::u8string u8_s = arg.u8string(); // path.u8string() IS UTF-8
            return std::basic_string<CharT>(reinterpret_cast<const CharT*>(u8_s.c_str()), u8_s.length());
        }
        else if constexpr (std::is_same_v<CharT, uint16_t> || std::is_same_v<CharT, char16_t>) // Covers CHAR16 and standard char16_t
        {
            // Goal: Convert path to std::basic_string<uint16_t> or std::basic_string<char16_t> (UTF-16)

            // Option 1: If wchar_t is 16-bit (like on Windows) and represents UTF-16
            if constexpr (sizeof(wchar_t) == sizeof(CharT))
            {
                std::wstring temp_ws = arg.wstring(); // Get UTF-16 as std::wstring
                // If CharT is uint16_t and wchar_t is also 16-bit, this reinterpret_cast is common.
                // If CharT is char16_t and wchar_t is also 16-bit, also common.
                return std::basic_string<CharT>(reinterpret_cast<const CharT*>(temp_ws.c_str()), temp_ws.length());
            }
            // Option 2: Convert from path's u8string (UTF-8) to UTF-16 (CharT)
            // This is more portable if wchar_t size varies or isn't guaranteed to be UTF-16.
            else
            {
                std::u8string u8_s = arg.u8string();
                if (u8_s.empty())
                {
                    return std::basic_string<CharT>();
                }

                try
                {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

                    // Ensure CharT for the converter is char16_t if that's what codecvt expects
                    // If CharT is uint16_t, we might need to cast the result or use a char16_t intermediate.
                    // For simplicity, let's assume we convert to std::u16string (char16_t based)
                    // and then construct std::basic_string<CharT> from it.

                    using IntermediateChar16Type = char16_t; // Standard type for codecvt
                    std::wstring_convert<std::codecvt_utf8_utf16<IntermediateChar16Type, 0x10FFFF, std::little_endian>, IntermediateChar16Type> converter;
                    std::basic_string<IntermediateChar16Type> u16_intermediate_s =
                            converter.from_bytes(reinterpret_cast<const char*>(u8_s.data()), reinterpret_cast<const char*>(u8_s.data() + u8_s.length()));

                    // Now construct the final std::basic_string<CharT>
                    // This assumes CharT (e.g., uint16_t) and IntermediateChar16Type (char16_t)
                    // have compatible representations for UTF-16 code units.
                    return std::basic_string<CharT>(reinterpret_cast<const CharT*>(u16_intermediate_s.data()), u16_intermediate_s.length());

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
                }
                catch (const std::exception&)
                {
                    // Catching std::exception for broader compatibility
                    throw std::runtime_error("Failed to convert path from UTF-8 to UTF-16");
                }
            }
        }
        else
        {
            // This static_assert will provide a compile-time error for unsupported CharT types.
            static_assert(std::is_same_v<CharT, wchar_t> || std::is_same_v<CharT, char> || std::is_same_v<CharT, uint16_t> || std::is_same_v<CharT, char16_t>,
                          "to_charT_string_path: Unsupported target CharT for path conversion");
            return std::basic_string<CharT>(); // Should be unreachable due to static_assert
        }
    }

    // Convert any string-like to a string of generic CharT
    // Or pass through if it's already a string(view) of CharType or if we can't convert it
    template <typename CharT, typename T>
    auto inline to_charT(T&& arg)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, std::filesystem::path> || std::is_same_v<std::decay_t<T>, const std::filesystem::path>)
        {
            // std::filesystem::path has its own conversion functions
            return to_charT_string_path<CharT>(std::forward<T>(arg));
        }
        else if constexpr (can_be_string_view_t<T>::value)
        {
            // If T is a char pointer, it can be treated as a string view
            return to_charT<CharT>(stringviewify(std::forward<T>(arg)));
        }
        else if constexpr (not_charT_string_like_t<T, CharT>::value)
        {
            // If T is a string or string view but not using CharT, convert it
            return to_charT_string<CharT>(std::forward<T>(arg));
        }
        else
        {
            // If T is already a string or string view using CharT, pass through
            // Or if we can't convert it, pass through
            return std::forward<T>(arg);
        }
    }

    // Ensure that a string is compatible with UE4SS, converting it if neccessary
    template <typename T>
    auto inline ensure_str(T&& arg) /* -> StringType */
    {
        return ensure_str_as<CharType>(std::forward<T>(arg)); // CharType is the project's native char type
    }

    template <typename TargetCharT, typename T>
    auto inline ensure_str_as(T&& arg) -> std::basic_string<TargetCharT>
    {
        return to_charT<TargetCharT>(std::forward<T>(arg));
    }

    template <typename T>
    auto inline to_utf8_string(T&& arg) -> std::string
    {
        return ensure_str_as<char>(std::forward<T>(arg));
    }

    // You can add more to_* function if needed

    // Auto Type Conversion Done

    /**
     * Normalizes a path for use in Lua, ensuring:
     * 1. UTF-8 encoding for proper Unicode handling
     * 2. Forward slashes for consistency across platforms
     * 
     * @param path The path to normalize
     * @return A UTF-8 encoded string with forward slashes
     * @throws std::runtime_error if conversion fails
     */
    auto inline normalize_path_for_lua(const std::filesystem::path& path) -> std::string
    {
        std::string utf8_path = to_utf8_string(path);
        
        // Replace backslashes with forward slashes for Lua
        std::replace(utf8_path.begin(), utf8_path.end(), '\\', '/');
        
        return utf8_path;
    }
    
    /**
     * Creates a Windows-compatible wide string from a UTF-8 path string
     * This is useful when opening files with Windows APIs that expect UTF-16
     * 
     * @param utf8_path UTF-8 encoded path string
     * @return Wide string (UTF-16) for Windows APIs
     * @throws std::runtime_error if conversion fails
     */
    auto inline utf8_to_wpath(const std::string& utf8_path) -> std::wstring
    {
        // No fallbacks - if this fails, it should throw since it's a critical error
        // that indicates invalid UTF-8 input
        return to_wstring(utf8_path);
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
            return ensure_str(input);
#endif
        }
    }
    auto inline ensure_str_const(std::string_view input) -> const StringType&
    {
        static std::unordered_map<std::string_view, StringType> uestringpool;
        static std::shared_mutex uestringpool_lock;

        // Allow multiple readers that are stalled when any thread is writing.
        {
            std::shared_lock<std::shared_mutex> read_guard(uestringpool_lock);
            if (uestringpool.contains(input)) return uestringpool[input];
        }

        auto temp_input = std::string{input};
        auto new_str = ensure_str(temp_input);

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
                       return std::towlower((wchar_t)a_char) == std::towlower((wchar_t)b_char);
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
