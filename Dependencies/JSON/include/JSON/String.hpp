#ifndef RC_JSON_STRING_HPP
#define RC_JSON_STRING_HPP

#include <format>

#include <JSON/Common.hpp>
#include <JSON/Value.hpp>
#include <File/Macros.hpp>

namespace RC::JSON
{
    class RC_JSON_API String : public Value
    {
    public:
        constexpr static Type static_type = Type::String;

    private:
        StringType m_data{};

    public:
        String() = default;
        explicit String(StringViewType string);
        ~String() override = default;

    public:
        auto get() -> StringType& { return m_data; }
        auto get_view() const -> StringViewType { return m_data; }

    public:
        auto serialize(ShouldFormat should_format = ShouldFormat::No, int32_t* indent_level = nullptr) -> StringType override;
        auto get_type() const -> Type override { return Type::String; }
    };
}

#endif //RC_JSON_STRING_HPP
