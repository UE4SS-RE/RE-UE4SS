#pragma once

#include <locale>
#include <codecvt>
#include <string>

// Set this to 1 to use ANSI (char*) instead of wide strings (wchar_t*)
#ifndef RC_IS_ANSI
#define RC_IS_ANSI 0
#endif

#if RC_IS_ANSI == 1
#define STR(str) u##str
#define SYSSTR(str) str
#else
#ifdef LINUX
#define STR(str) u##str
#define SYSSTR(str) str
#else
#define STR(str) L##str
#define SYSSTR(str) L##str
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
#if RC_IS_ANSI == 1
    using StringType = std::string;
    using StringViewType = std::string_view;
    using CharType = char;
    using StreamType = std::ifstream;
    // using ToString = std::tostring;

    constexpr auto ToString = [](auto&& numeric_value) constexpr -> decltype(auto) {
        return std::to_string(std::forward<decltype(numeric_value)>(numeric_value));
    };
#else
#ifdef WIN32
    using StringType = std::wstring;
    using StringViewType = std::wstring_view;
    using CharType = wchar_t;
    using StreamType = std::wifstream;
    constexpr auto ToString = [](auto&& numeric_value) constexpr -> decltype(auto) {
        return std::to_wstring(std::forward<decltype(numeric_value)>(numeric_value));
    };
#else
// on linux, use utf8
    using StringType = std::string;
    using StringViewType = std::string_view;
    using CharType = char;
    using StreamType = std::ifstream;

    constexpr auto ToString = [](auto&& numeric_value) constexpr -> decltype(auto) {
        return std::to_string(std::forward<decltype(numeric_value)>(numeric_value));
    };
#endif
#endif
} // namespace RC::File

namespace RC
{
    using SystemStringType = File::StringType;
    using SystemStringViewType = File::StringViewType;
    using SystemCharType = File::CharType;
    using SystemStreamType = File::StreamType;

#ifdef WIN32
    using UEStringType = File::StringType;
    using UEViewType = File::StringViewType;
    using UECharType = File::CharType;
    #define SystemStringPrint "%S"
    #define UEStringPrint "%S"
    static auto SystemStringToUEString = [](const SystemStringType& str) -> UEStringType { return str; };
    static auto UEStringToSystemString = [](const UEStringType& str) -> SystemStringType { return str; };
#else
    using UEStringType = std::u16string;
    using UEViewType = std::u16string_view;
    using UECharType = char16_t;
    #define SystemStringPrint "%s"
    #define UEStringPrint "%S"
    static auto SystemStringToUEString = [](const SystemStringType& str) -> UEStringType {
        static std::wstring_convert<typename std::codecvt_utf8_utf16<char16_t>, char16_t> converter{};
        return converter.from_bytes((std::string)str);
    };
    static auto UEStringToSystemString = [](const UEStringType& str) -> SystemStringType {
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes((std::u16string)str);
    };
#endif

    constexpr auto ToString = File::ToString;
} // namespace RC
