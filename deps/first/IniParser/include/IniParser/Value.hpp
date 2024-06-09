#pragma once

#include <array>
#include <cstdint>

#include <File/File.hpp>
#include <IniParser/Common.hpp>

namespace RC::Ini
{
    class Value
    {
      public:
        enum class Type
        {
            NoType,
            String,
            Int64,
            Float,
            Bool,

            Max,
        };

      private:
        // Always use "m_ref->value" instead of "value"
        // That way it returns properly if this value is a reference to another variable
        // The "m_ref->value" member is set to self if it doesn't refer to another variable
        SystemStringType m_string_value;
        int64_t m_int64_value{};
        float m_float_value{};
        bool m_bool_value{};

        std::array<bool, static_cast<size_t>(Type::Max)> m_valid_types{false};
        size_t m_num_valid_types{0};

        const Value* m_ref{nullptr};

      public:
        RC_INI_PARSER_API auto get_ref() const -> const Value*;
        RC_INI_PARSER_API auto set_ref(const Value* new_ref) -> void;

        RC_INI_PARSER_API auto is_valid_string() const -> bool
        {
            return is_valid_type<Type::String>();
        }
        RC_INI_PARSER_API auto is_valid_int64() const -> bool
        {
            return is_valid_type<Type::Int64>();
        }
        RC_INI_PARSER_API auto is_valid_float() const -> bool
        {
            return is_valid_type<Type::Float>();
        }
        RC_INI_PARSER_API auto is_valid_bool() const -> bool
        {
            return is_valid_type<Type::Bool>();
        }

        RC_INI_PARSER_API auto get_string_value() const -> const SystemStringType&;
        RC_INI_PARSER_API auto get_int64_value() const -> int64_t;
        RC_INI_PARSER_API auto get_float_value() const -> float;
        RC_INI_PARSER_API auto get_bool_value() const -> bool;

        RC_INI_PARSER_API auto add_string_value(SystemStringViewType data) -> void;
        RC_INI_PARSER_API auto add_int64_value(const SystemStringType& data, int base = 10) -> void;
        RC_INI_PARSER_API auto add_float_value(const SystemStringType& data) -> void;
        RC_INI_PARSER_API auto add_bool_value(bool data) -> void;

      private:
        template <Type ValueType>
        auto add_type() -> void
        {
            m_valid_types[static_cast<int32_t>(ValueType)] = true;
            ++m_num_valid_types;
        }

        template <Type ValueType>
        [[nodiscard]] auto is_valid_type() const -> bool
        {
            return m_valid_types[static_cast<int32_t>(ValueType)];
        }
    };
} // namespace RC::Ini
