#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include <Common.hpp>

struct lua_State;

namespace RC::LuaType
{
    /**
     * @brief Represents a serialized Lua value that can be safely passed between threads.
     *
     * This struct is used for the AsyncCompute API to serialize Lua values from one
     * Lua state and deserialize them into another (potentially isolated) state.
     *
     * Phase 1 supports: nil, boolean, number (integer/float), string, and tables
     * containing only these types. Tables can be nested.
     *
     * Unsupported types (function, userdata, thread, lightuserdata) will throw
     * during serialization.
     */
    struct RC_UE4SS_API LuaValue
    {
        enum class Type : uint8_t
        {
            Nil,
            Boolean,
            Integer,
            Number,
            String,
            Table
        };

        Type type{Type::Nil};

        // Storage for the value
        // Using variant for type-safe storage
        std::variant<
            std::monostate,                                   // Nil
            bool,                                             // Boolean
            int64_t,                                          // Integer
            double,                                           // Number (float)
            std::string,                                      // String
            std::vector<std::pair<LuaValue, LuaValue>>        // Table (key-value pairs)
        > data;

        // Constructors for convenience
        LuaValue() : type(Type::Nil), data(std::monostate{}) {}
        explicit LuaValue(bool b) : type(Type::Boolean), data(b) {}
        explicit LuaValue(int64_t i) : type(Type::Integer), data(i) {}
        explicit LuaValue(double d) : type(Type::Number), data(d) {}
        explicit LuaValue(std::string s) : type(Type::String), data(std::move(s)) {}
        explicit LuaValue(std::vector<std::pair<LuaValue, LuaValue>> t) : type(Type::Table), data(std::move(t)) {}

        // Accessors
        [[nodiscard]] auto is_nil() const -> bool { return type == Type::Nil; }
        [[nodiscard]] auto is_boolean() const -> bool { return type == Type::Boolean; }
        [[nodiscard]] auto is_integer() const -> bool { return type == Type::Integer; }
        [[nodiscard]] auto is_number() const -> bool { return type == Type::Number; }
        [[nodiscard]] auto is_string() const -> bool { return type == Type::String; }
        [[nodiscard]] auto is_table() const -> bool { return type == Type::Table; }

        [[nodiscard]] auto as_boolean() const -> bool
        {
            if (type != Type::Boolean) throw std::runtime_error("LuaValue is not a boolean");
            return std::get<bool>(data);
        }

        [[nodiscard]] auto as_integer() const -> int64_t
        {
            if (type != Type::Integer) throw std::runtime_error("LuaValue is not an integer");
            return std::get<int64_t>(data);
        }

        [[nodiscard]] auto as_number() const -> double
        {
            if (type == Type::Integer) return static_cast<double>(std::get<int64_t>(data));
            if (type != Type::Number) throw std::runtime_error("LuaValue is not a number");
            return std::get<double>(data);
        }

        [[nodiscard]] auto as_string() const -> const std::string&
        {
            if (type != Type::String) throw std::runtime_error("LuaValue is not a string");
            return std::get<std::string>(data);
        }

        [[nodiscard]] auto as_table() const -> const std::vector<std::pair<LuaValue, LuaValue>>&
        {
            if (type != Type::Table) throw std::runtime_error("LuaValue is not a table");
            return std::get<std::vector<std::pair<LuaValue, LuaValue>>>(data);
        }

        [[nodiscard]] auto as_table() -> std::vector<std::pair<LuaValue, LuaValue>>&
        {
            if (type != Type::Table) throw std::runtime_error("LuaValue is not a table");
            return std::get<std::vector<std::pair<LuaValue, LuaValue>>>(data);
        }

        // Comparison for use as map key (needed for table serialization)
        auto operator==(const LuaValue& other) const -> bool;
        auto operator<(const LuaValue& other) const -> bool;
    };

    /**
     * @brief Configuration options for serialization
     */
    struct RC_UE4SS_API LuaSerializeOptions
    {
        int max_depth{100};           // Maximum table nesting depth
        bool error_on_unsupported{true};  // Throw on unsupported types (vs returning nil)
    };

    /**
     * @brief Serialize a Lua value from the stack into a LuaValue
     *
     * @param L The Lua state
     * @param idx Stack index of the value to serialize
     * @param options Serialization options
     * @return The serialized LuaValue
     * @throws std::runtime_error if the value contains unsupported types (and error_on_unsupported is true)
     */
    RC_UE4SS_API auto serialize_lua_value(lua_State* L, int idx, const LuaSerializeOptions& options = {}) -> LuaValue;

    /**
     * @brief Push a LuaValue onto the Lua stack
     *
     * @param L The Lua state
     * @param value The value to push
     */
    RC_UE4SS_API auto push_lua_value(lua_State* L, const LuaValue& value) -> void;

    /**
     * @brief Get a human-readable type name for a LuaValue type
     */
    RC_UE4SS_API auto lua_value_type_name(LuaValue::Type type) -> const char*;

    /**
     * @brief Convert a LuaValue to a JSON string
     *
     * @param value The LuaValue to convert
     * @param pretty If true, format with indentation
     * @return JSON string representation
     * @throws std::runtime_error on serialization failure
     */
    RC_UE4SS_API auto lua_value_to_json(const LuaValue& value, bool pretty = false) -> std::string;

    /**
     * @brief Parse a JSON string into a LuaValue
     *
     * @param json_str The JSON string to parse
     * @return Parsed LuaValue
     * @throws std::runtime_error on parse failure
     */
    RC_UE4SS_API auto json_to_lua_value(const std::string& json_str) -> LuaValue;

} // namespace RC::LuaType
