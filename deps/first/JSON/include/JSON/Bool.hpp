#pragma once

#include <JSON/Value.hpp>

namespace RC::JSON
{
    class RC_JSON_API Bool : public Value
    {
      public:
        constexpr static Type static_type = Type::Bool;

      private:
        bool m_underlying_value{};

      public:
        explicit Bool(bool value);
        ~Bool() override = default;

      public:
        auto get() -> bool
        {
            return m_underlying_value;
        }

      public:
        auto serialize([[maybe_unused]] ShouldFormat should_format = ShouldFormat::No, [[maybe_unused]] int32_t* indent_level = nullptr) -> UEStringType override;
        auto get_type() const -> Type override
        {
            return Type::Bool;
        }
    };
} // namespace RC::JSON
