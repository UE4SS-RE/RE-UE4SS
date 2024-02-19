#pragma once

#include <codecvt>
#include <cwctype>
#include <locale>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <type_traits>

#include <File/Macros.hpp>

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

    auto inline explode_by_occurrence(const std::wstring& in_str_wide, const wchar_t delimiter, ExplodeType start_or_end) -> std::wstring
    {
        size_t occurrence = (start_or_end == ExplodeType::FromStart ? in_str_wide.find_first_of(delimiter) : in_str_wide.find_last_of(delimiter));

        std::wstring return_value;
        if (occurrence != std::wstring::npos)
        {
            return_value = start_or_end == ExplodeType::FromEnd ? in_str_wide.substr(occurrence + 1, std::wstring::npos) : in_str_wide.substr(0, occurrence);
        }
        else
        {
            return_value = in_str_wide;
        }

        return return_value;
    }

    auto inline explode_by_occurrence(const std::u16string& in_str_wide, const char16_t delimiter, ExplodeType start_or_end) -> std::u16string
    {
        size_t occurrence = (start_or_end == ExplodeType::FromStart ? in_str_wide.find_first_of(delimiter) : in_str_wide.find_last_of(delimiter));

        std::u16string return_value;
        if (occurrence != std::wstring::npos)
        {
            return_value = start_or_end == ExplodeType::FromEnd ? in_str_wide.substr(occurrence + 1, std::u16string::npos) : in_str_wide.substr(0, occurrence);
        }
        else
        {
            return_value = in_str_wide;
        }

        return return_value;
    }

    auto inline explode_by_occurrence(const std::string& in_str, const char delimiter, ExplodeType start_or_end) -> std::string
    {
        size_t occurrence = (start_or_end == ExplodeType::FromStart ? in_str.find_first_of(delimiter) : in_str.find_last_of(delimiter));

        std::string return_value;
        if (occurrence != std::string::npos)
        {
            return_value = start_or_end == ExplodeType::FromEnd ? in_str.substr(occurrence + 1, std::string::npos) : in_str.substr(0, occurrence);
        }
        else
        {
            return_value = in_str;
        }

        return return_value;
    }

    auto inline explode_by_occurrence(const std::string& in_str, const char delimiter, const int32_t occurrence) -> std::string
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

    auto inline explode_by_occurrence(const std::string& in_str, const char delimiter) -> std::vector<std::string>
    {
        std::vector<std::string> result;

        size_t counter{};
        size_t start_offset{};

        for (const char* current_char = in_str.c_str(); *current_char; ++current_char)
        {
            if (*current_char == delimiter || counter == in_str.length() - 1)
            {
                std::string sub_str = in_str.substr(start_offset, counter - start_offset + (counter == in_str.length() - 1 ? 1 : 0));
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

    auto inline explode_by_occurrence(const std::wstring& in_str_wide, const wchar_t delimiter) -> std::vector<std::wstring>
    {
        std::vector<std::wstring> result;

        size_t counter{};
        size_t start_offset{};

        for (const wchar_t* current_char = in_str_wide.c_str(); *current_char; ++current_char)
        {
            if (*current_char == delimiter || counter == in_str_wide.length() - 1)
            {
                std::wstring sub_str = in_str_wide.substr(start_offset, counter - start_offset + (counter == in_str_wide.length() - 1 ? 1 : 0));
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

    auto inline explode_by_occurrence(const std::u16string& in_str_wide, const char16_t delimiter) -> std::vector<std::u16string>
    {
        std::vector<std::u16string> result;

        size_t counter{};
        size_t start_offset{};

        for (const char16_t* current_char = in_str_wide.c_str(); *current_char; ++current_char)
        {
            if (*current_char == delimiter || counter == in_str_wide.length() - 1)
            {
                std::u16string sub_str = in_str_wide.substr(start_offset, counter - start_offset + (counter == in_str_wide.length() - 1 ? 1 : 0));
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

    auto inline explode_by_occurrence(const std::wstring& in_str, const wchar_t delimiter, const int32_t occurrence) -> std::wstring
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

    auto inline explode_by_occurrence(const std::u16string& in_str, const char16_t delimiter, const int32_t occurrence) -> std::u16string
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
        #if WIN32
#pragma warning(disable : 4996)
            static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
            return converter.from_bytes(input);
#pragma warning(default : 4996)
        #else
            static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};
            return converter.from_bytes(input);
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

    auto inline to_wstring(std::string_view input) -> std::wstring
    {
        auto temp_input = std::string{input};
        return to_wstring(temp_input);
    }

    auto inline to_wstring(std::u16string& input) -> std::wstring
    {
#ifdef WIN32
        return {input.begin(), input.end()};
#else
        throw std::runtime_error{"There is no reason to use this function on non-Windows platforms"};
#endif
    }

    auto inline to_wstring(std::u16string_view input) -> std::wstring
    {
#ifdef WIN32
        auto temp_input = std::u16string{input};
        return to_wstring(temp_input);
#else
        return to_wstring(std::u16string{input});
#endif
    }

    auto inline to_wstring(std::filesystem::path& input) -> std::wstring
    {
        return input.wstring();
    }

    auto inline to_string(std::wstring& input) -> std::string
    {
#ifdef WIN32
#pragma warning(disable : 4996)
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter{};
        return converter.to_bytes(input);
#pragma warning(default : 4996)
#else
        static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};
        return converter.to_bytes(input);
#endif
    }

    auto inline to_string(std::u16string& input) -> std::string
    {
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(input);
    }

    auto inline to_string(std::wstring_view input) -> std::string
    {
        auto temp_input = std::wstring{input};
        return to_string(temp_input);
    }

    auto inline to_string(std::u16string_view input) -> std::string
    {
        auto temp_input = std::u16string{input};
        return to_string(temp_input);
    }

    auto inline to_string(std::string_view input) -> std::string
    {
        return std::string{input};
    }

    auto inline to_string(std::string& input) -> std::string
    {
        return std::string{input};
    }

    auto inline to_string(std::filesystem::path& input) -> std::string
    {
        return input.string();
    }

    auto inline to_u16string(std::wstring& input) -> std::u16string
    {
#ifdef WIN32
        return {input.begin(), input.end()};
#else
        throw std::runtime_error{"There is no reason to use this function on non-Windows platforms"};
#endif
    }

    auto inline to_u16string(std::wstring_view input) -> std::u16string
    {
        auto temp_input = std::wstring{input};
        return to_u16string(temp_input);
    }

    auto inline to_u16string(std::string& input) -> std::u16string
    {
        // codecvt_utf8_utf16<char16_t>
        #pragma warning(disable : 4996)
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(input);
        #pragma warning(default : 4996)
    }

    auto inline to_u16string(std::string_view input) -> std::u16string
    {
        auto temp_input = std::string{input};
        return to_u16string(temp_input);
    }

    auto inline to_u16string(std::u16string_view input) -> std::u16string
    {
        return std::u16string{input};
    }

    auto inline to_u16string(std::u16string& input) -> std::u16string
    {
        return std::u16string{input};
    }

    auto inline to_u16string(std::filesystem::path& input) -> std::u16string
    {
        return input.u16string();
    }

    // Type traits to check if T is a string type that needs conversion
    template<typename T>
    struct is_string_like_t : std::false_type {};

    // Specializations for the types that need conversion
    template<> struct is_string_like_t<std::string> : std::true_type {};
    template<> struct is_string_like_t<std::wstring> : std::true_type {};
    template<> struct is_string_like_t<std::u16string> : std::true_type {};
    template<> struct is_string_like_t<std::wstring_view> : std::true_type {};
    template<> struct is_string_like_t<std::u16string_view> : std::true_type {};
    template<> struct is_string_like_t<std::string_view> : std::true_type {};

    template <typename T>
    struct dependent_false : std::false_type {};

    template <typename T>
    auto stringviewify(T&& tp) {
        if constexpr (std::is_same_v<std::decay_t<T>, UECharType*> || std::is_same_v<std::decay_t<T>, const UECharType*>) {
            return UEStringViewType{tp};
        } else if constexpr  (std::is_same_v<std::decay_t<T>, SystemCharType*> || std::is_same_v<std::decay_t<T>, const SystemCharType*>) {
            return SystemStringViewType{tp};
        } else if constexpr (std::is_same_v<std::decay_t<T>, File::CharType*> || std::is_same_v<std::decay_t<T>, const File::CharType*>) {
            return File::StringViewType{tp};
        } else {
            return std::forward<T>(tp);
        }
    }

    template<typename T>
    struct _can_be_string_view_t : std::false_type {};
    template<> struct _can_be_string_view_t<wchar_t*> : std::true_type {};
    template<> struct _can_be_string_view_t<char*> : std::true_type {};
    template<> struct _can_be_string_view_t<char16_t*> : std::true_type {};
    template<> struct _can_be_string_view_t<const wchar_t*> : std::true_type {};
    template<> struct _can_be_string_view_t<const char*> : std::true_type {};
    template<> struct _can_be_string_view_t<const char16_t*> : std::true_type {};

    template<typename T>
    struct can_be_string_view_t : _can_be_string_view_t<std::decay_t<T>> {};

    template<typename T>
    struct is_file_string_type : std::disjunction<
        std::is_same<T, File::StringType>,
        std::is_same<T, File::StringViewType>
    > {};

    template<typename T>
    struct not_file_string_like_t : std::conjunction<is_string_like_t<std::decay_t<T>>, std::negation<is_file_string_type<std::decay_t<T>>>> {};

    template <typename T>
    auto inline to_file_string(T&& input) -> File::StringType {
        if constexpr (std::is_same_v<File::StringType, std::string>)
        {
            return to_string(input);
        }
        else if constexpr (std::is_same_v<File::StringType, std::wstring>)
        {
            return to_wstring(input);
        }
        else if constexpr (std::is_same_v<File::StringType, std::u16string>)
        {
            return to_u16string(input);
        } else {
            static_assert(dependent_false<T>::value, "Unsupported SystemStringType.");
        }
    }

    template<typename T>
    auto to_file(T&& arg) {
        if constexpr (can_be_string_view_t<T>::value) {
            return to_file_string(stringviewify(std::forward<T>(arg)));
        } 
        else if constexpr (not_file_string_like_t<std::decay_t<T>>::value) 
        {
            return to_file_string(std::forward<T>(arg));
        } 
        else 
        {
            return std::forward<T>(arg);
        }
    }

    template<typename T>
    struct is_system_string_type : std::disjunction<
        std::is_same<T, SystemStringType>,
        std::is_same<T, SystemStringViewType>
    > {};

    template<typename T>
    struct not_system_string_like_t : std::conjunction<is_string_like_t<std::decay_t<T>>,std::negation<is_system_string_type<std::decay_t<T>>>> {};

    template <typename T>
    auto inline to_system_string(T&& input) -> SystemStringType {
        if constexpr (std::is_same_v<SystemStringType, std::string>)
        {
            return to_string(input);
        }
        else if constexpr (std::is_same_v<SystemStringType, std::wstring>)
        {
            return to_wstring(input);
        }
        else if constexpr (std::is_same_v<SystemStringType, std::u16string>)
        {
            return to_u16string(input);
        }
        else 
        {
            static_assert(dependent_false<T>::value, "Unsupported SystemStringType.");
        }
    }

    template<typename T>
    auto to_system(T&& arg) {
        if constexpr (can_be_string_view_t<T>::value) {
            return to_system_string(stringviewify(std::forward<T>(arg)));
        } 
        else if constexpr (not_system_string_like_t<std::decay_t<T>>::value) 
        {
            return to_system_string(std::forward<T>(arg));
        } 
        else 
        {
            return std::forward<T>(arg);
        }
    }

    template<typename T>
    struct is_ue_string_type : std::disjunction<
        std::is_same<T, UEStringType>,
        std::is_same<T, UEStringViewType>
    > {};

    template<typename T>
    struct not_ue_string_like_t : std::conjunction<is_string_like_t<std::decay_t<T>>, std::negation<is_ue_string_type<std::decay_t<T>>>> {};

    template <typename T>
    auto inline to_ue_string(T&& input) -> UEStringType {
        if constexpr (std::is_same_v<UEStringType, std::string>)
        {
            return to_string(input);
        }
        else if constexpr (std::is_same_v<UEStringType, std::wstring>)
        {
            return to_wstring(input);
        }
        else if constexpr (std::is_same_v<UEStringType, std::u16string>)
        {
            return to_u16string(input);
        } 
        else 
        {
            static_assert(dependent_false<T>::value, "Unsupported SystemStringType.");
        }
    }

    template<typename T>
    auto to_ue(T&& arg) {
        if constexpr (can_be_string_view_t<T>::value) 
        {
            return to_ue_string(stringviewify(std::forward<T>(arg)));
        } 
        else if constexpr (not_ue_string_like_t<std::decay_t<T>>::value) 
        {
            return to_ue_string(std::forward<T>(arg));
        } 
        else 
        {
            return std::forward<T>(arg);
        }
    }

    template<typename T>
    struct is_lua_string_type : std::disjunction<
        std::is_same<T, std::string>,
        std::is_same<T, std::string>
    > {};

    template<typename T>
    struct not_lua_string_like_t : std::negation<is_lua_string_type<std::decay_t<T>>> {};

    template<typename T>
    auto to_lua(T&& arg) {
        if constexpr (can_be_string_view_t<T>::value || not_lua_string_like_t<std::decay_t<T>>::value) 
        {
            return to_string((arg));
        } 
        else 
        {
            return std::forward<T>(arg);
        }
    }

    #define csfor_lua(x) (to_lua((x)).c_str())

    namespace String
    {
        auto inline iequal(std::wstring_view a, std::wstring_view b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const wchar_t a_char, const wchar_t b_char) {
                       return std::towlower(a_char) == std::towlower(b_char);
                   });
        }

        auto inline iequal(std::u16string_view a, std::u16string_view b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const char16_t a_char, const char16_t b_char) {
                       return std::towlower(a_char) == std::towlower(b_char);
                   });
        }

        auto inline iequal(std::wstring& a, const wchar_t* b)
        {
            return iequal(a, std::wstring_view{b});
        }

        auto inline iequal(std::u16string& a, const char16_t* b)
        {
            return iequal(a, std::u16string_view{b});
        }

        auto inline iequal(const wchar_t* a, std::wstring& b)
        {
            return iequal(std::wstring_view{a}, b);
        }

        auto inline iequal(const char16_t* a, std::u16string& b)
        {
            return iequal(std::u16string_view{a}, b);
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
