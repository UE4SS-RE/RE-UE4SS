#pragma once

#include <JSON/Value.hpp>

namespace RC::JSON
{
    class RC_JSON_API Null : public Value
    {
      public:
        constexpr static Type static_type = Type::Null;

      public:
        ~Null() override = default;

      public:
        auto serialize([[maybe_unused]] ShouldFormat should_format = ShouldFormat::No, [[maybe_unused]] int32_t* indent_level = nullptr) -> UEStringType override;
        auto get_type() const -> Type override
        {
            return Type::Null;
        }
    };
} // namespace RC::JSON
