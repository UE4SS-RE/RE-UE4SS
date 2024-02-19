#pragma once

#include <locale>
#include <codecvt>
#include <string>

// Set this to 1 to use ANSI (char*) instead of wide strings (wchar_t*)
#ifndef RC_IS_ANSI
#define RC_IS_ANSI 0
#endif

// always use u16string
#define STR(str) u##str

#ifdef LINUX
    #define SYSSTR(str) str
    #define IOSTR(str) str
#else
    #if RC_IS_ANSI == 0
        #define SYSSTR(str) u##str
        #define IOSTR(str) u##str
    #else
        #define SYSSTR(str) str
        #define IOSTR(str) str
    #endif
#endif

#ifdef S
static_assert(false, "UE4SS define 'S' is already defined, please solve this");
#else
//#define S(str) STR(str)
#endif

#define THROW_INTERNAL_FILE_ERROR(msg)                                                                                                                         \
    RC::File::Internal::StaticStorage::internal_error = true;                                                                                                  \
    throw std::runtime_error{msg};

// This macro needs to be moved into the DynamicOutput system
/*
#define ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(param_device) \
                if (DefaultTargets::get_default_devices_ref().empty() || !param_device){                       \
                THROW_INTERNAL_OUTPUT_ERROR("[Output::send] Attempted to send but there were no default devices. Please specify at least one default device or
construct a Targets object and supply your own devices.") \
                }
//*/

#include <string>

namespace RC::File
{
    typedef std::basic_ofstream<std::u16string::value_type> u16ofstream;
    typedef std::basic_ifstream<std::u16string::value_type> u16ifstream;
    
#if RC_IS_ANSI == 1
    using StringType = std::string;
    using StringViewType = std::string_view;
    using CharType = char;
    using StreamType = std::ifstream;
#else
    // System String Types
    #ifdef WIN32
        using StringType = std::u16string;
        using StringViewType = std::u16string_view;
        using CharType = std::u16string::value_type;
        using StreamType = u16ifstream;
    #else
    // on linux, use utf8
        using StringType = std::string;
        using StringViewType = std::string_view;
        using CharType = char;
        using StreamType = std::ifstream;
    #endif // WIN32
#endif // RC_IS_ANSI

} // namespace RC::File

namespace RC
{
    // Should find a better place for these definitions
    // System = C++ String Types
#if RC_IS_ANSI == 1
    using SystemStringType = std::string;
    using SystemStringViewType = std::string_view;
    using SystemCharType = char;
    using SystemStreamType = std::ifstream;
    constexpr auto ToString = [](auto&& numeric_value) constexpr -> decltype(auto) {
        return std::to_string(std::forward<decltype(numeric_value)>(numeric_value));
    };
    #define SystemStringPrint "%s"
#else
    // System String Types
    #ifdef WIN32
        using SystemStringType = std::u16string;
        using SystemStringViewType = std::u16string_view;
        using SystemCharType = std::u16string::value_type;
        using SystemStreamType = u16ifstream;
        constexpr auto ToString = [](auto&& numeric_value) constexpr -> decltype(auto) {
            return std::to_u16string(std::forward<decltype(numeric_value)>(numeric_value));
        };
        #define SystemStringPrint "%S"
    #else
    // on linux, use utf8
        using SystemStringType = std::string;
        using SystemStringViewType = std::string_view;
        using SystemCharType = char;
        using SystemStreamType = std::ifstream;
        constexpr auto ToString = [](auto&& numeric_value) constexpr -> decltype(auto) {
            return std::to_string(std::forward<decltype(numeric_value)>(numeric_value));
        };
        #define SystemStringPrint "%s"
    #endif // WIN32
#endif

#if RC_IS_ANSI == 1
    using UEStringType = std::string;
    using UEStringViewType = std::string_view;
    using UECharType = std::string::value_type;
    #define UEStringPrint "%s"
#else
    using UEStringType = std::u16string;
    using UEStringViewType = std::u16string_view;
    using UECharType = std::u16string::value_type;
    #define UEStringPrint "%S"
#endif // RC_IS_ANSI

} // namespace RC
