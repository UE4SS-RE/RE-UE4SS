#pragma once
#include <concepts>
#include <type_traits>

#include <glaze/glaze.hpp>

#include <Constructs/CompileTimeString.hpp>

namespace RC
{
    /*
     * @brief A type which allows for annotating other types with strings
     *
     * A type which allows for annotating other types with strings
     * This type is most commonly used with json properties to annotate them with comments.
     * The type also allows for transparent access of the inner value for ease of use.
     *
     * @tparam C annotation strings
     * @tparam T inner value type
     *
     * Example:
     *
     * @code{.cpp}
     * Annotated<Strings<"Hi", "Bye">, int> annotated_integer(123);
     *
     * for (const auto& comment : comments) {
     *   Output::send(SYSSTR("{}\n"), comment);
     * }
     * @endcode
     */
    template <typename C, typename T>
    struct Annotated
    {
        decltype(C::strings) comments = C::strings;
        T value;

        /*
         * Create a new Annotated instance with a value
         */
        Annotated(T value) : value(std::move(value))
        {
        }

        template <typename OtherC>
        Annotated(const Annotated<OtherC, T>& other) noexcept
            requires std::copy_constructible<T>
            : value(other.value)
        {
        }

        template <typename OtherC>
        Annotated& operator=(const Annotated<OtherC, T>& other) noexcept
            requires std::is_copy_assignable_v<T>
        {
            value = other.value;
            return *this;
        }

        template <typename OtherC>
        Annotated(Annotated<OtherC, T>&& other) noexcept
            requires std::move_constructible<T>
            : value(std::move(other.value))
        {
        }

        template <typename OtherC>
        Annotated& operator=(Annotated<OtherC, T>&& other) noexcept
            requires std::is_move_assignable_v<T>
        {
            value = std::move(other.value);
            return *this;
        }

        T operator*() const noexcept
        {
            return value;
        }

        T& operator->() noexcept
        {
            return value;
        }

        struct glaze
        {
            using GLZ_T = Annotated;
            [[maybe_unused]] static constexpr std::string_view name = "Annotated";
            static constexpr auto value = glz::object("Comments", &GLZ_T::comments, "Value", &GLZ_T::value);
        };
    };

} // namespace RC
