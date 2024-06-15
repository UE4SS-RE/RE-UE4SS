#pragma once

#include <format>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

#include <File/Macros.hpp>
#include <Helpers/String.hpp>
#include <JSON/Common.hpp>
#include <JSON/KeyValuePair.hpp>
#include <JSON/Number.hpp>
#include <JSON/Value.hpp>

namespace RC::JSON
{
    class RC_JSON_API Object : public Value
    {
      public:
        constexpr static Type static_type = Type::Object;

      public:
        enum class IsGlobalObject
        {
            Yes,
            No
        };
    
      private:
        std::unordered_map<StringType, std::unique_ptr<JSON::Value>> m_members{};
        IsGlobalObject m_is_global_object{IsGlobalObject::No};

      public:
        Object() = default;
        explicit Object(IsGlobalObject is_global_object) : m_is_global_object(is_global_object){};
        ~Object() override = default;

        // Explicitly making the class non-copyable to enable dllexport with unique_ptr
        Object(const Object&) = delete;
        auto operator=(const Object&) -> Object& = delete;

      private:
        JSON_DEPRECATED auto find_value_by_key(const StringType& key) const -> Value*;
        JSON_DEPRECATED auto find_value_by_key(const StringType& key) -> Value*;

      public:
        JSON_DEPRECATED auto new_object(StringType name) -> Object&;
        JSON_DEPRECATED auto new_array(StringType name) -> class Array&;
        JSON_DEPRECATED auto new_string(StringType name, const StringType& value) -> void;
        JSON_DEPRECATED auto new_null(StringType name) -> void;
        JSON_DEPRECATED auto new_bool(StringType name, bool value) -> void;

        JSON_DEPRECATED auto add_object(StringType name, std::unique_ptr<Object>) -> void;

        JSON_DEPRECATED auto get() -> decltype(m_members)&
        {
            return m_members;
        };
        JSON_DEPRECATED auto get() const -> const decltype(m_members)&
        {
            return m_members;
        };

        template <typename ValueType>
        JSON_DEPRECATED auto get(const StringType& key) const -> ValueType&
        {
            auto value = find_value_by_key(key);
            if (!value)
            {
                throw std::runtime_error{to_string(std::format(STR("No key in JSON object with name {}"), key))};
            }
            return *static_cast<ValueType*>(value);
        }

        template <typename ValueType>
        JSON_DEPRECATED auto get(const StringType& key) -> ValueType&
        {
            auto value = find_value_by_key(key);
            if (!value)
            {
                throw std::runtime_error{to_string(std::format(STR("No key in JSON object with name {}"), key))};
            }
            return *static_cast<ValueType*>(value);
        }

        template <JSONNumber number_type>
        JSON_DEPRECATED auto new_number(StringType name, number_type value) -> void
        {
            m_members.emplace(std::move(name), std::make_unique<Number>(value));
        }

        JSON_DEPRECATED auto serialize(ShouldFormat should_format = ShouldFormat::No, int32_t* indent_level = nullptr) -> StringType override;
        JSON_DEPRECATED auto get_type() const -> Type override
        {
            return Type::Object;
        }
    };
} // namespace RC::JSON
