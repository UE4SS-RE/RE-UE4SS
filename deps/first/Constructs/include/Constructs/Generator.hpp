#pragma once

#include <algorithm>
#include <coroutine>
#include <numeric>
// #include <ranges>
#include <vector>

namespace RC
{
    template <typename T>
    struct Generator
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T const*;
        using reference = T const&;

        struct promise_type
        {
            T value;

            auto get_return_object() -> Generator
            {
                return Generator(handle_type::from_promise(*this));
            }

            auto initial_suspend() -> std::suspend_always
            {
                return {};
            }
            auto final_suspend() noexcept -> std::suspend_always
            {
                return {};
            }
            auto unhandled_exception() -> void
            {
            }

            template <std::convertible_to<T> From>
            auto yield_value(From&& from) -> std::suspend_always
            {
                value = std::forward<From>(from);
                return {};
            }

            auto return_void() -> void
            {
            }
        };

        struct iterator
        {
            using iterator_category = std::input_iterator_tag;
            using value_type = T;
            using difference_type = ptrdiff_t;
            using pointer = T const*;
            using reference = T const&;

            iterator() = default;
            constexpr explicit iterator(handle_type handle) : handle(handle)
            {
            }

            constexpr auto operator==(const iterator& other) const -> bool
            {
                return handle == other.handle;
            }

            constexpr auto operator!=(const iterator& other) const -> bool
            {
                return !(*this == other);
            }

            auto operator++() -> iterator&
            {
                handle.resume();

                if (handle.done())
                {
                    handle = nullptr;
                }

                return *this;
            }

            auto operator++(int) -> iterator&
            {
                iterator temp = *this;
                *++this;
                return temp;
            }

            auto operator*() const -> T const&
            {
                return handle.promise().value;
            }

            auto operator->() const -> T const*
            {
                return std::addressof(operator*());
            }

          private:
            handle_type handle;
        };

        Generator(handle_type handle) : handle(handle)
        {
        }
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        Generator(Generator&& other) noexcept : handle(other.handle)
        {
            other.handle = {};
        }

        Generator& operator=(const Generator&& other) noexcept
        {
            if (this != &other)
            {
                if (handle) handle.destroy();
                handle = other.handle;
                other.handle = {};
            }
            return *this;
        }

        ~Generator()
        {
            if (handle) handle.destroy();
        }

        explicit operator bool()
        {
            handle();
            return !handle.done();
        }

        T operator()()
        {
            handle();
            return std::move(handle.promise().value);
        }

        auto begin() const -> iterator
        {
            if (!handle) return {};

            handle.resume();

            if (handle.done()) return {};

            return iterator(handle);
        }

        auto end() const -> iterator
        {
            return {};
        }

      private:
        handle_type handle;
    };
} // namespace RC
