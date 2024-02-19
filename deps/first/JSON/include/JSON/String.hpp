#pragma once

#include <format>

#include <File/Macros.hpp>
#include <JSON/Common.hpp>
#include <JSON/Value.hpp>

namespace RC::JSON
{
    class String : public Value
    {
      public:
        RC_JSON_API constexpr static Type static_type = Type::String;

      private:
        UEStringType m_data{};

      public:
        RC_JSON_API String() = default;
        RC_JSON_API explicit String(UEStringViewType string);
        RC_JSON_API ~String() override = default;

      public:
        RC_JSON_API auto get() -> UEStringType&
        {
            return m_data;
        }
        RC_JSON_API auto get_view() const -> UEStringViewType
        {
            return m_data;
        }

      public:
        RC_JSON_API auto serialize(ShouldFormat should_format = ShouldFormat::No, int32_t* indent_level = nullptr) -> UEStringType override;
        RC_JSON_API auto get_type() const -> Type override
        {
            return Type::String;
        }
    };
} // namespace RC::JSON
