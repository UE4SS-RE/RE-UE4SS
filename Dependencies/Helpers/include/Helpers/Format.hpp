#ifndef RC_FORMAT_HPP
#define RC_FORMAT_HPP

#include <string>

namespace RC
{
    template<typename ...Args>
    auto static fmt(const char* fmt, Args... args) -> std::string
    {
        constexpr size_t out_string_length = 1000;
        char out_string[out_string_length];

        size_t msg_len = strlen(fmt);

        // Attempt to give a hint if the buffer is too small
        if (msg_len > out_string_length)
        {
            fmt = "An error occurred but the message was too long for the buffer.";
            msg_len = strlen(fmt);
        }

        // If the buffer is too small for the hint message then I guess we do nothing
        // The default message will be used which can't be too small since it's calculated at compile-time
        if (msg_len < out_string_length)
        {
            sprintf_s(out_string, out_string_length, fmt, args...);
        }

        return out_string;
    }

    template<typename ...Args>
    auto static fmt(const wchar_t* fmt, Args... args) -> std::wstring
    {
        constexpr size_t out_string_length = 1000;
        wchar_t out_string[out_string_length];

        size_t msg_len = wcslen(fmt);

        // Attempt to give a hint if the buffer is too small
        if (msg_len > out_string_length)
        {
            fmt = L"An error occurred but the message was too long for the buffer.";
            msg_len = wcslen(fmt);
        }

        // If the buffer is too small for the hint message then I guess we do nothing
        // The default message will be used which can't be too small since it's calculated at compile-time
        if (msg_len < out_string_length)
        {
            swprintf_s(out_string, out_string_length, fmt, args...);
        }

        return out_string;
    }
}

#endif //RC_FORMAT_HPP
