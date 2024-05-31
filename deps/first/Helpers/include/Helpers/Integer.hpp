#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace RC::Helper::Integer
{
    template <std::integral IntTo, std::integral IntFrom>
    auto to(IntFrom from) -> IntTo
    {
        static constexpr const char* error_message{"Tried converting integer to another integral type but it was too big or too small for the resulting type"};

        if constexpr (std::numeric_limits<IntFrom>::is_signed)
        {
            auto as_signed = static_cast<int64_t>(from);
            if (as_signed < std::numeric_limits<IntTo>::min() || as_signed > std::numeric_limits<IntTo>::max())
            {
                throw std::runtime_error{error_message};
            }
            else
            {
                return static_cast<IntTo>(from);
            }
        }
        else
        {
            auto as_unsigned = static_cast<uint64_t>(from);
            if (as_unsigned > std::numeric_limits<IntTo>::max())
            {
                throw std::runtime_error{error_message};
            }
            else
            {
                return static_cast<IntTo>(from);
            }
        }
    }
} // namespace RC::Helper::Integer
