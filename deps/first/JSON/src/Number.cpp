#include <JSON/Number.hpp>

namespace RC::JSON
{
    Number::Number(uint32_t value)
    {
        *std::bit_cast<uint32_t*>(&m_value.as_integer) = value;
        m_stored_type = Type::UInt32;
        m_is_signed = false;
    }

    Number::Number(uint64_t value)
    {
        *std::bit_cast<uint64_t*>(&m_value.as_integer) = value;
        m_stored_type = Type::UInt64;
        m_is_signed = false;
    }

    Number::Number(int32_t value)
    {
        *std::bit_cast<int32_t*>(&m_value.as_integer) = value;
        m_stored_type = Type::Int32;
        m_is_signed = true;
    }

    Number::Number(int64_t value)
    {
        *std::bit_cast<int64_t*>(&m_value.as_integer) = value;
        m_stored_type = Type::Int64;
        m_is_signed = true;
    }

    Number::Number(float value)
    {
        m_value.as_float = value;
        m_stored_type = Type::Float;
    }

    Number::Number(double value)
    {
        m_value.as_double = value;
        m_stored_type = Type::Double;
    }

    auto Number::serialize(ShouldFormat should_format, int32_t* indent_level) -> SystemStringType
    {
        (void)should_format;
        (void)indent_level;

        switch (m_stored_type)
        {
        case Type::UInt32:
            return ToString(get<uint32_t>());
        case Type::UInt64:
            return ToString(get<uint64_t>());
        case Type::Int32:
            return ToString(get<int32_t>());
        case Type::Int64:
            return ToString(get<int64_t>());
        case Type::Float:
            return ToString(get<float>());
        case Type::Double:
            return ToString(get<double>());
        }

        throw std::runtime_error{"JSON number was not valid"};
    }
} // namespace RC::JSON
