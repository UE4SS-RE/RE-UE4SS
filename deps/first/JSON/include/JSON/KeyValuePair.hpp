#pragma once

namespace RC::JSON
{
    template <typename T>
    struct TypedKeyValuePair
    {
        const UEStringType& key;
        T* value{};
    };

    struct KeyValuePair
    {
        UEStringType key{};
        std::unique_ptr<JSON::Value> value{};

        template <typename JSONElementType>
        auto is() const -> bool
        {
            return value->get_type() == JSONElementType::static_type;
        }

        template <typename JSONElementType>
        auto as() -> TypedKeyValuePair<JSONElementType>
        {
            if (value->get_type() != JSONElementType::static_type)
            {
                throw std::runtime_error{std::format("JSONKeyValuePair::as<JSONElementType>: actual type '{}' did not match requested type '{}'",
                                                     type_to_string(value->get_type()),
                                                     type_to_string(JSONElementType::static_type))};
            }
            else
            {
                return TypedKeyValuePair<JSONElementType>{key, value->as<JSONElementType>()};
            }
        }

        template <typename JSONElementType>
        auto as() const -> TypedKeyValuePair<JSONElementType>
        {
            if (value->get_type() != JSONElementType::static_type)
            {
                throw std::runtime_error{std::format("JSONKeyValuePair::as<JSONElementType>: actual type '{}' did not match requested type '{}'",
                                                     type_to_string(value->get_type()),
                                                     type_to_string(JSONElementType::static_type))};
            }
            else
            {
                return TypedKeyValuePair<JSONElementType>{key, value->as<JSONElementType>()};
            }
        }
    };
} // namespace RC::JSON

/*
namespace RC::JSON::Parser
{
    enum class JSONElementType
    {
        Base,
        Object,
        Array,
        String,
        Integer,
    };

    inline auto type_to_string(JSONElementType element_type) -> std::string
    {
        switch (element_type)
        {
            case JSONElementType::Base:
                return "Base";
            case JSONElementType::Object:
                return "Object";
            case JSONElementType::Array:
                return "Array";
            case JSONElementType::String:
                return "String";
            case JSONElementType::Integer:
                return "Integer";
        }

        return "Unhandled Type";
    }

    struct JSONElementBase
    {
        constexpr static auto static_type() -> JSONElementType { return JSONElementType::Base; }
        virtual auto get_type() const -> JSONElementType = 0;

        template<typename JSONElementType>
        auto is_a() const -> bool
        {
            return get_type() == JSONElementType::static_type();
        }

        template<typename JSONElementType>
        auto as() -> JSONElementType*
        {
            if (get_type() != JSONElementType::static_type())
            {
                throw std::runtime_error{std::format("JSONKeyValuePair::as<JSONElementType>: actual type '{}' did not match requested type '{}'",
type_to_string(get_type()), type_to_string(JSONElementType::static_type()))};
            }
            else
            {
                return static_cast<JSONElementType*>(this);
            }
        }

        template<typename JSONElementType>
        auto as() const -> const JSONElementType*
        {
            if (get_type() != JSONElementType::static_type())
            {
                throw std::runtime_error{std::format("JSONKeyValuePair::as<JSONElementType>: actual type '{}' did not match requested type '{}'",
type_to_string(get_type()), type_to_string(JSONElementType::static_type()))};
            }
            else
            {
                return static_cast<const JSONElementType*>(this);
            }
        }
    };

    template<typename T>
    struct JSONTypedKeyValuePair
    {
        const SystemStringType& key;
        T* value{};
    };

    struct JSONKeyValuePair
    {
        SystemStringType key{};
        std::unique_ptr<JSONElementBase> value{};

        template<typename JSONElementType>
        auto is_a() const -> bool
        {
            return value->get_type() == JSONElementType::static_type();
        }

        template<typename JSONElementType>
        auto as() -> JSONTypedKeyValuePair<JSONElementType>
        {
            return JSONTypedKeyValuePair<JSONElementType>{key, value->as<JSONElementType>()};
        }

        template<typename JSONElementType>
        auto as() const -> JSONTypedKeyValuePair<JSONElementType>
        {
            return JSONTypedKeyValuePair<JSONElementType>{key, value->as<JSONElementType>()};
        }
    };

    struct JSONObject : JSONElementBase
    {
    private:
        std::vector<JSONKeyValuePair> elements{};

    public:
        JSONObject() = default;

    private:
        auto find_value_by_key(StringViewType look_for_key) const -> JSONElementBase*
        {
            for (const auto&[key, value] : elements)
            {
                if (key == look_for_key) { return value.get(); }
            }
            return nullptr;
        }

        auto find_value_by_key(StringViewType look_for_key) -> JSONElementBase*
        {
            for (auto&[key, value] : elements)
            {
                if (key == look_for_key) { return value.get(); }
            }
            return nullptr;
        }

    public:
        constexpr static auto static_type() -> JSONElementType { return JSONElementType::Object; }
        auto get() -> std::vector<JSONKeyValuePair>& { return elements; }
        auto get() const -> const std::vector<JSONKeyValuePair>& { return elements; }
        auto get_type() const -> JSONElementType override { return JSONElementType::Object; }

        template<typename ValueType>
        auto get(StringViewType key) const -> ValueType&
        {
            auto value = find_value_by_key(key);
            if (!value) { throw std::runtime_error{to_string(std::format(SYSSTR("No key in JSON object with name {}"), key))}; }
            return *static_cast<ValueType*>(value);
        }

        template<typename ValueType>
        auto get(StringViewType key) -> ValueType&
        {
            auto value = find_value_by_key(key);
            if (!value) { throw std::runtime_error{to_string(std::format(SYSSTR("No key in JSON object with name {}"), key))}; }
            return *static_cast<ValueType*>(value);
        }
    };

    struct JSONArray : JSONElementBase
    {
    private:
        std::vector<std::unique_ptr<JSONElementBase>> elements{};

    public:
        JSONArray() = default;

    public:
        constexpr static auto static_type() -> JSONElementType { return JSONElementType::Array; }
        auto get() -> std::vector<std::unique_ptr<JSONElementBase>>& { return elements; }
        auto get() const -> const std::vector<std::unique_ptr<JSONElementBase>>& { return elements; }
        auto get_type() const -> JSONElementType override { return JSONElementType::Array; }
    };

    struct JSONString : JSONElementBase
    {
    private:
        SystemStringType value{};

    public:
        JSONString() = default;
        JSONString(SystemStringType value) : value(std::move(value)) { }

    public:
        constexpr static auto static_type() -> JSONElementType { return JSONElementType::String; }
        auto get() -> SystemStringType& { return value; }
        auto get() const -> const SystemStringType& { return value; }
        auto get_view() -> StringViewType { return value; }
        auto get_view() const -> StringViewType { return value; }
        auto get_type() const -> JSONElementType override { return JSONElementType::String; }
    };

    struct JSONInteger : JSONElementBase
    {
    private:
        int64_t value{};

    public:
        JSONInteger() = default;
        JSONInteger(int64_t value) : value(value) { }

    public:
        constexpr static auto static_type() -> JSONElementType { return JSONElementType::Integer; }
        auto get() -> int64_t { return value; }
        auto get() const -> int64_t { return value; }
        auto get_type() const -> JSONElementType override { return JSONElementType::Integer; }
    };
}
//*/
