#pragma once

#ifndef __clang__

#include <ranges>
#include <type_traits>

namespace RC
{
    template <std::ranges::input_range R>
        requires std::ranges::view<R>
    class EnumerateView : public std::ranges::view_interface<EnumerateView<R>>
    {
      public:
        struct iterator;

        struct iterator
        {
            using iterator_concept =
                    std::conditional_t<std::ranges::random_access_range<R>,
                                       std::random_access_iterator_tag,
                                       std::conditional_t<std::ranges::bidirectional_range<R>,
                                                          std::bidirectional_iterator_tag,
                                                          std::conditional_t<std::ranges::forward_range<R>, std::forward_iterator_tag, std::input_iterator_tag>>>;
            using value_type = std::remove_cvref_t<std::ranges::range_reference_t<R>>;
            using difference_type = std::ranges::range_difference_t<R>;

            iterator() = default;
            constexpr explicit iterator(std::ranges::iterator_t<R> base) : base_(base)
            {
            }

            constexpr auto operator++() -> iterator&
            {
                ++base_;
                ++index;
                return *this;
            }

            constexpr auto operator++(int) -> iterator
            {
                iterator tmp = *this;
                ++*this;
                return tmp;
            }

            constexpr auto operator--() -> iterator&
                requires std::ranges::bidirectional_range<R>
            {
                --base_;
                --index;
                return *this;
            }

            constexpr auto operator--(int) -> iterator
                requires std::ranges::bidirectional_range<R>
            {
                auto tmp = *this;
                --*this;
                return tmp;
            }

            constexpr auto operator+=(const difference_type offset) -> iterator&
                requires std::ranges::random_access_range<R>
            {
                base_ += offset;
                index += offset;
                return *this;
            }

            constexpr auto operator-=(const difference_type offset) -> iterator&
                requires std::ranges::random_access_range<R>
            {
                base_ -= offset;
                index -= offset;
                return *this;
            }

            constexpr auto operator==(const iterator& other) const -> bool
            {
                return base_ == other.base_;
            }

            constexpr auto operator!=(const iterator& other) const -> bool
            {
                return !(*this == other);
            }

            constexpr auto operator*() const -> std::pair<value_type, size_t>
            {
                return {*base_, index};
            }

          private:
            std::ranges::iterator_t<R> base_{};
            size_t index = 0;
        };

        EnumerateView()
            requires std::default_initializable<R>
        = default;
        constexpr EnumerateView(R base) : base_(std::move(base)), iter_(std::begin(base_))
        {
        }

        constexpr R base() const&
        {
            return base_;
        }
        constexpr R base() &&
        {
            return std::move(base_);
        }

        constexpr auto begin()
        {
            return iterator(iter_);
        }
        constexpr auto end()
        {
            return iterator(std::ranges::end(base_));
        }

        constexpr auto size() const
            requires std::ranges::sized_range<const R>
        {
            return std::ranges::size(base_);
        }

      private:
        R base_{};
        std::ranges::iterator_t<R> iter_{};
    };

    template <class R>
    EnumerateView(R&& base) -> EnumerateView<std::views::all_t<R>>;

    namespace details
    {
        struct EnumerateViewClosure
        {
            EnumerateViewClosure() = default;

            template <std::ranges::viewable_range R>
            constexpr auto operator()(R&& range) const noexcept
            {
                return EnumerateView(std::forward<R>(range));
            }
        };

        template <std::ranges::viewable_range R>
        constexpr auto operator|(R&& r, EnumerateViewClosure const& closure)
        {
            return closure(std::forward<R>(r));
        }
    } // namespace details

    namespace views
    {
        inline constexpr details::EnumerateViewClosure enumerate;
    }
} // namespace RC

#else

#include <iterator>

// A simple enumerate impl for clang to avoid https://github.com/llvm/llvm-project/issues/60704
namespace RC
{

    namespace views
    {
        template <typename T>
        struct internal_enumerate
        {
            T& iterable;
            size_t index;

            template <typename It>
            struct iterator : std::iterator<std::input_iterator_tag, std::pair<typename It::value_type, size_t>>
            {
                using iterator_category = std::input_iterator_tag;
                using value_type = std::pair<typename It::value_type, size_t>;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type*;
                using reference = value_type&;

                size_t index;
                It iter;

                explicit iterator(size_t index, It iter) : index(index), iter(iter)
                {
                }

                iterator& operator++()
                {
                    ++index;
                    ++iter;
                    return *this;
                }

                bool operator!=(const iterator& other) const
                {
                    return iter != other.iter;
                }

                value_type operator*() const
                {
                    return {*iter, index};
                }
            };

            auto begin()
            {
                return iterator<std::decay_t<decltype(iterable.begin())>>(0, iterable.begin());
            }

            auto end()
            {
                return iterator<std::decay_t<decltype(iterable.begin())>>(0, iterable.end());
            }

            explicit internal_enumerate(T& iterable) : iterable(iterable), index(0)
            {
            }

            internal_enumerate operator|(T& iterable)
            {
                return internal_enumerate(iterable);
            }
        };

        struct enumerate_op
        {
        };
        // operator |
        template <typename T>
        internal_enumerate<T> operator|(T& iterable, enumerate_op)
        {
            return internal_enumerate<T>(iterable);
        }

        // T&& version
        template <typename T>
        internal_enumerate<T> operator|(T&& iterable, enumerate_op)
        {
            return internal_enumerate<T>(iterable);
        }

        inline constexpr enumerate_op enumerate;
    }; // namespace views
} // namespace RC
#endif // __clang__
