#ifndef RC_JSON_OBJECT_HPP
#define RC_JSON_OBJECT_HPP

#include <variant>
#include <vector>
#include <format>
#include <stdexcept>
#include <memory>
#include <unordered_map>

#include <File/Macros.hpp>
#include <Helpers/String.hpp>
#include <JSON/Common.hpp>
#include <JSON/Value.hpp>
#include <JSON/Number.hpp>
#include <JSON/KeyValuePair.hpp>

namespace RC::JSON
{
    class RC_JSON_API Object : public Value
    {
    public:
        constexpr static Type static_type = Type::Object;

    public:
        enum class IsGlobalObject { Yes, No };

#pragma warning(disable: 4251)
    private:
        std::unordered_map<StringType, std::unique_ptr<JSON::Value>> m_members{};
        IsGlobalObject m_is_global_object{IsGlobalObject::No};
#pragma warning(default: 4251)

    public:
        Object() = default;
        explicit Object(IsGlobalObject is_global_object) : m_is_global_object(is_global_object) {};
        ~Object() override = default;

        // Explicitly making the class non-copyable to enable dllexport with unique_ptr
        Object(const Object&) = delete;
        auto operator=(const Object&) -> Object& = delete;


    private:
        auto find_value_by_key(const StringType& key) const -> Value*;
        auto find_value_by_key(const StringType& key) -> Value*;

    public:
        auto new_object(StringType name) -> Object&;
        auto new_array(StringType name) -> class Array&;
        auto new_string(StringType name, const StringType& value) -> void;
        auto new_null(StringType name) -> void;
        auto new_bool(StringType name, bool value) -> void;

        auto add_object(StringType name, std::unique_ptr<Object>) -> void;

        auto get() -> decltype(m_members)& { return m_members; };
        auto get() const -> const decltype(m_members)& { return m_members; };

        template<typename ValueType>
        auto get(const StringType& key) const -> ValueType&
        {
            auto value = find_value_by_key(key);
            if (!value) { throw std::runtime_error{to_string(std::format(STR("No key in JSON object with name {}"), key))}; }
            return *static_cast<ValueType*>(value);
        }

        template<typename ValueType>
        auto get(const StringType& key) -> ValueType&
        {
            auto value = find_value_by_key(key);
            if (!value) { throw std::runtime_error{to_string(std::format(STR("No key in JSON object with name {}"), key))}; }
            return *static_cast<ValueType*>(value);
        }

        template<JSONNumber number_type>
        auto new_number(StringType name, number_type value) -> void
        {
            m_members.emplace(std::move(name), std::make_unique<Number>(value));
        }

        auto serialize(ShouldFormat should_format = ShouldFormat::No, int32_t* indent_level = nullptr) -> StringType override;
        auto get_type() const -> Type override { return Type::Object; }
    };
}

#endif //RC_JSON_OBJECT_HPP
