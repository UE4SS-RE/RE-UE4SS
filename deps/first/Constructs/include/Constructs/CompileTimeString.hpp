#pragma once
#include <cstdint>
#include <algorithm>
#include <array>
#include <string_view>

namespace RC
{
    /*
     * A compile-time string
     *
     * @tparam N string size
     */
    template <size_t N>
    struct ConstString
    {
        constexpr ConstString(const char (&str)[N])
        {
            std::copy_n(str, N, value);
        }

        /*
         * String value
         */
        char value[N];
    };

    template <size_t N>
    ConstString(const char (&)[N]) -> ConstString<N>;

    /*
     * A compile-time collection of compile-time strings
     *
     * @tparam C compile-time strings
     *
     * Example:
     *
     * @code{.cpp}
     * Strings<"Hi", "Bye"> strings;
     * @endcode
     */
    template <ConstString... C>
    struct Strings
    {
        /*
         * Compile-time strings storage
         */
        constexpr static std::array strings = {std::string_view{C.value}...};
    };

} // namespace RC
