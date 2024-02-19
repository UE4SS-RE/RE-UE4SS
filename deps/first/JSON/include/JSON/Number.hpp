#pragma once

#include <bit>

#include <JSON/Value.hpp>

namespace RC::JSON
{
    class RC_JSON_API Number : public Value
    {
      public:
        constexpr static Type static_type = JSON::Type::Number;

      public:
        enum class Type
        {
            UInt32,
            UInt64,
            Int32,
            Int64,
            Float,
            Double,
        };

        union StoredValue {
            uint8_t as_integer[4];
            float as_float;
            double as_double;
        };
        StoredValue m_value{};
        Type m_stored_type{};
        bool m_is_signed{};

      public:
        explicit Number(uint64_t value);
        explicit Number(uint32_t value);
        explicit Number(int64_t value);
        explicit Number(int32_t value);
        explicit Number(float value);
        explicit Number(double value);
        ~Number() override = default;

      private:
        template <typename NumberType, typename ValueType, typename StoredValueType>
        auto static get_internal(ValueType stored_type, StoredValueType& value) -> NumberType&
        {
            if constexpr (std::is_same_v<NumberType, uint32_t>)
            {
                if (stored_type == Type::UInt32)
                {
                    return *std::bit_cast<NumberType*>(&value.as_integer);
                }
            }
            else if constexpr (std::is_same_v<NumberType, uint64_t>)
            {
                if (stored_type == Type::UInt64)
                {
                    return *std::bit_cast<NumberType*>(&value.as_integer);
                }
            }
            else if constexpr (std::is_same_v<NumberType, int32_t>)
            {
                if (stored_type == Type::Int32)
                {
                    return *std::bit_cast<NumberType*>(&value.as_integer);
                }
            }
            else if constexpr (std::is_same_v<NumberType, int64_t>)
            {
                if (stored_type == Type::Int64)
                {
                    return *std::bit_cast<NumberType*>(&value.as_integer);
                }
            }
            else if constexpr (std::is_same_v<NumberType, float>)
            {
                if (stored_type == Type::Float)
                {
                    return value.as_float;
                }
            }
            else if constexpr (std::is_same_v<NumberType, double>)
            {
                if (stored_type == Type::Double)
                {
                    return value.as_double;
                }
            }
            throw std::runtime_error{"JSON::Number::get<NumberType>(): Tried to get number of type that doesn't match the stored type"};
        }

        template <typename NumberType, typename ValueType>
        auto static is_internal(ValueType stored_type) -> bool
        {
            // TODO: Can we do more checking here to actually check whether the value is within the range of 'NumberType' ?
            if constexpr (std::is_same_v<NumberType, uint32_t>)
            {
                if (stored_type == Type::UInt32)
                {
                    return true;
                }
            }
            else if constexpr (std::is_same_v<NumberType, uint64_t>)
            {
                if (stored_type == Type::UInt64)
                {
                    return true;
                }
            }
            else if constexpr (std::is_same_v<NumberType, int32_t>)
            {
                if (stored_type == Type::Int32)
                {
                    return true;
                }
            }
            else if constexpr (std::is_same_v<NumberType, int64_t>)
            {
                if (stored_type == Type::Int64)
                {
                    return true;
                }
            }
            else if constexpr (std::is_same_v<NumberType, float>)
            {
                if (stored_type == Type::Float)
                {
                    return true;
                }
            }
            else if constexpr (std::is_same_v<NumberType, double>)
            {
                if (stored_type == Type::Double)
                {
                    return true;
                }
            }
            else
            {
                return false;
            }
        }

      public:
        template <typename NumberType>
        auto get() -> NumberType&
        {
            return get_internal<NumberType>(m_stored_type, m_value);
        }
        template <typename NumberType>
        auto get() const -> NumberType&
        {
            return get_internal<NumberType>(m_stored_type, m_value);
        }

        // Intentionally hides the definition of 'is' inside the Value base class.
        template <typename NumberType>
        auto is() -> bool
        {
            return is_internal<NumberType>(m_stored_type);
        }
        // Intentionally hides the definition of 'is' inside the Value base class.
        template <typename NumberType>
        auto is() const -> bool
        {
            return is_internal<NumberType>(m_stored_type);
        }

      public:
        auto serialize([[maybe_unused]] ShouldFormat should_format = ShouldFormat::No, [[maybe_unused]] int32_t* indent_level = nullptr) -> UEStringType override;
        auto get_type() const -> JSON::Type override
        {
            return JSON::Type::Number;
        }
    };
} // namespace RC::JSON
