#pragma once

#include <JSON/Common.hpp>

namespace RC::JSON
{
    template <typename SupposedJSONNumber>
    concept JSONNumber = std::is_same_v<SupposedJSONNumber, int32_t> || std::is_same_v<SupposedJSONNumber, float> || std::is_same_v<SupposedJSONNumber, double>;

    enum class Type
    {
        Null,
        Object,
        Array,
        String,
        Number,
        Bool,
    };

    JSON_DEPRECATED inline auto type_to_string(Type type) -> std::string
    {
        switch (type)
        {
        case Type::Null:
            return "Null";
        case Type::Object:
            return "Object";
        case Type::Array:
            return "Array";
        case Type::String:
            return "String";
        case Type::Number:
            return "Number";
        case Type::Bool:
            return "Bool";
        }

        return "UnhandledType";
    }

    enum class ShouldFormat
    {
        Yes,
        No
    };

    class RC_JSON_API Value
    {
      public:
        virtual ~Value() = default;

      public:
        virtual auto serialize(ShouldFormat, int32_t* indent_level) -> StringType = 0;
        virtual auto get_type() const -> Type = 0;

      public:
        template <typename T>
        auto is() -> bool
        {
            return get_type() == T::static_type;
        }
        template <typename T>
        auto is() const -> bool
        {
            return get_type() == T::static_type;
        }

        template <typename T>
        auto as() const -> const T*
        {
            return static_cast<const T*>(this);
        }
        template <typename T>
        auto as() -> T*
        {
            return static_cast<T*>(this);
        }
    };
} // namespace RC::JSON
