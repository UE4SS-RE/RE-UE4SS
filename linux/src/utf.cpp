#include "ue4ss/utf.hpp"

#include <cstdint>
#include <stdexcept>

namespace
{
    constexpr std::uint32_t k_replacement_limit = 0x10ffffu;

    [[nodiscard]] bool is_continuation(unsigned char value)
    {
        return (value & 0xc0u) == 0x80u;
    }

    void append_utf16(std::u16string& output, std::uint32_t codepoint)
    {
        if (codepoint <= 0xffffu)
        {
            if (codepoint >= 0xd800u && codepoint <= 0xdfffu)
            {
                throw std::invalid_argument("UTF-8 decodes to a surrogate code point");
            }
            output.push_back(static_cast<char16_t>(codepoint));
            return;
        }
        if (codepoint > k_replacement_limit)
        {
            throw std::invalid_argument("UTF-8 code point is outside Unicode");
        }
        codepoint -= 0x10000u;
        output.push_back(static_cast<char16_t>(0xd800u + (codepoint >> 10u)));
        output.push_back(static_cast<char16_t>(0xdc00u + (codepoint & 0x3ffu)));
    }

    void append_utf8(std::string& output, std::uint32_t codepoint)
    {
        if (codepoint <= 0x7fu)
        {
            output.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7ffu)
        {
            output.push_back(static_cast<char>(0xc0u | (codepoint >> 6u)));
            output.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
        }
        else if (codepoint <= 0xffffu)
        {
            output.push_back(static_cast<char>(0xe0u | (codepoint >> 12u)));
            output.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
        }
        else
        {
            output.push_back(static_cast<char>(0xf0u | (codepoint >> 18u)));
            output.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
        }
    }
} // namespace

namespace ue4ss::linux
{
    std::u16string utf8_to_unreal(std::string_view input)
    {
        std::u16string output;
        output.reserve(input.size());
        for (std::size_t index = 0; index < input.size();)
        {
            const auto first = static_cast<unsigned char>(input[index]);
            std::uint32_t codepoint{};
            std::size_t continuation_count{};
            std::uint32_t minimum{};
            if (first <= 0x7fu)
            {
                codepoint = first;
            }
            else if ((first & 0xe0u) == 0xc0u)
            {
                codepoint = first & 0x1fu;
                continuation_count = 1u;
                minimum = 0x80u;
            }
            else if ((first & 0xf0u) == 0xe0u)
            {
                codepoint = first & 0x0fu;
                continuation_count = 2u;
                minimum = 0x800u;
            }
            else if ((first & 0xf8u) == 0xf0u)
            {
                codepoint = first & 0x07u;
                continuation_count = 3u;
                minimum = 0x10000u;
            }
            else
            {
                throw std::invalid_argument("invalid UTF-8 leading byte");
            }

            if (index + continuation_count >= input.size())
            {
                throw std::invalid_argument("truncated UTF-8 sequence");
            }
            for (std::size_t continuation = 0; continuation < continuation_count; ++continuation)
            {
                const auto byte = static_cast<unsigned char>(input[index + continuation + 1u]);
                if (!is_continuation(byte))
                {
                    throw std::invalid_argument("invalid UTF-8 continuation byte");
                }
                codepoint = (codepoint << 6u) | (byte & 0x3fu);
            }
            if (continuation_count > 0u && codepoint < minimum)
            {
                throw std::invalid_argument("overlong UTF-8 sequence");
            }
            append_utf16(output, codepoint);
            index += continuation_count + 1u;
        }
        return output;
    }

    std::string unreal_to_utf8(std::u16string_view input)
    {
        std::string output;
        output.reserve(input.size());
        for (std::size_t index = 0; index < input.size(); ++index)
        {
            std::uint32_t codepoint = input[index];
            if (codepoint >= 0xd800u && codepoint <= 0xdbffu)
            {
                if (++index >= input.size())
                {
                    throw std::invalid_argument("truncated UTF-16 surrogate pair");
                }
                const std::uint32_t low = input[index];
                if (low < 0xdc00u || low > 0xdfffu)
                {
                    throw std::invalid_argument("invalid UTF-16 surrogate pair");
                }
                codepoint = 0x10000u + ((codepoint - 0xd800u) << 10u) + (low - 0xdc00u);
            }
            else if (codepoint >= 0xdc00u && codepoint <= 0xdfffu)
            {
                throw std::invalid_argument("unpaired UTF-16 low surrogate");
            }
            append_utf8(output, codepoint);
        }
        return output;
    }
} // namespace ue4ss::linux
