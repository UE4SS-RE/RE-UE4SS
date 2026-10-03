#include <LuaType/LuaValue.hpp>

#include <lua.hpp>
#include <stdexcept>
#include <unordered_set>

#include <glaze/glaze.hpp>

namespace RC::LuaType
{
    auto LuaValue::operator==(const LuaValue& other) const -> bool
    {
        if (type != other.type) return false;

        switch (type)
        {
        case Type::Nil:
            return true;
        case Type::Boolean:
            return std::get<bool>(data) == std::get<bool>(other.data);
        case Type::Integer:
            return std::get<int64_t>(data) == std::get<int64_t>(other.data);
        case Type::Number:
            return std::get<double>(data) == std::get<double>(other.data);
        case Type::String:
            return std::get<std::string>(data) == std::get<std::string>(other.data);
        case Type::Table:
            // Table comparison by content (expensive but needed for completeness)
            return std::get<std::vector<std::pair<LuaValue, LuaValue>>>(data) ==
                   std::get<std::vector<std::pair<LuaValue, LuaValue>>>(other.data);
        }
        return false;
    }

    auto LuaValue::operator<(const LuaValue& other) const -> bool
    {
        // First compare by type
        if (type != other.type) return static_cast<uint8_t>(type) < static_cast<uint8_t>(other.type);

        switch (type)
        {
        case Type::Nil:
            return false; // All nils are equal
        case Type::Boolean:
            return std::get<bool>(data) < std::get<bool>(other.data);
        case Type::Integer:
            return std::get<int64_t>(data) < std::get<int64_t>(other.data);
        case Type::Number:
            return std::get<double>(data) < std::get<double>(other.data);
        case Type::String:
            return std::get<std::string>(data) < std::get<std::string>(other.data);
        case Type::Table:
            // Lexicographic comparison of table entries
            return std::get<std::vector<std::pair<LuaValue, LuaValue>>>(data) <
                   std::get<std::vector<std::pair<LuaValue, LuaValue>>>(other.data);
        }
        return false;
    }

    auto lua_value_type_name(LuaValue::Type type) -> const char*
    {
        switch (type)
        {
        case LuaValue::Type::Nil: return "nil";
        case LuaValue::Type::Boolean: return "boolean";
        case LuaValue::Type::Integer: return "integer";
        case LuaValue::Type::Number: return "number";
        case LuaValue::Type::String: return "string";
        case LuaValue::Type::Table: return "table";
        }
        return "unknown";
    }

    // Internal helper to serialize with cycle detection
    static auto serialize_lua_value_internal(
        lua_State* L,
        int idx,
        const LuaSerializeOptions& options,
        int current_depth,
        std::unordered_set<const void*>& visited_tables
    ) -> LuaValue
    {
        // Normalize index to absolute
        if (idx < 0 && idx > LUA_REGISTRYINDEX)
        {
            idx = lua_gettop(L) + idx + 1;
        }

        int ltype = lua_type(L, idx);

        switch (ltype)
        {
        case LUA_TNIL:
            return LuaValue{};

        case LUA_TBOOLEAN:
            return LuaValue{static_cast<bool>(lua_toboolean(L, idx))};

        case LUA_TNUMBER:
            if (lua_isinteger(L, idx))
            {
                return LuaValue{static_cast<int64_t>(lua_tointeger(L, idx))};
            }
            else
            {
                return LuaValue{lua_tonumber(L, idx)};
            }

        case LUA_TSTRING:
        {
            size_t len = 0;
            const char* str = lua_tolstring(L, idx, &len);
            return LuaValue{std::string(str, len)};
        }

        case LUA_TTABLE:
        {
            // Check depth limit
            if (current_depth >= options.max_depth)
            {
                if (options.error_on_unsupported)
                {
                    throw std::runtime_error("Table nesting depth exceeds maximum (" + std::to_string(options.max_depth) + ")");
                }
                return LuaValue{};
            }

            // Check for cycles
            const void* table_ptr = lua_topointer(L, idx);
            if (visited_tables.find(table_ptr) != visited_tables.end())
            {
                if (options.error_on_unsupported)
                {
                    throw std::runtime_error("Circular table reference detected");
                }
                return LuaValue{};
            }
            visited_tables.insert(table_ptr);

            std::vector<std::pair<LuaValue, LuaValue>> entries;

            // Iterate over table
            lua_pushnil(L);  // First key
            while (lua_next(L, idx) != 0)
            {
                // Key is at -2, value at -1
                try
                {
                    LuaValue key = serialize_lua_value_internal(L, -2, options, current_depth + 1, visited_tables);
                    LuaValue value = serialize_lua_value_internal(L, -1, options, current_depth + 1, visited_tables);
                    entries.emplace_back(std::move(key), std::move(value));
                }
                catch (...)
                {
                    lua_pop(L, 2);  // Pop value and key before rethrowing
                    visited_tables.erase(table_ptr);
                    throw;
                }

                lua_pop(L, 1);  // Pop value, keep key for next iteration
            }

            visited_tables.erase(table_ptr);
            return LuaValue{std::move(entries)};
        }

        case LUA_TFUNCTION:
            if (options.error_on_unsupported)
            {
                throw std::runtime_error("Cannot serialize Lua function (use a C++ task handler for async work)");
            }
            return LuaValue{};

        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
            if (options.error_on_unsupported)
            {
                throw std::runtime_error("Cannot serialize userdata (UObjects, etc. cannot cross thread boundaries)");
            }
            return LuaValue{};

        case LUA_TTHREAD:
            if (options.error_on_unsupported)
            {
                throw std::runtime_error("Cannot serialize Lua thread/coroutine");
            }
            return LuaValue{};

        default:
            if (options.error_on_unsupported)
            {
                throw std::runtime_error("Unknown Lua type: " + std::to_string(ltype));
            }
            return LuaValue{};
        }
    }

    auto serialize_lua_value(lua_State* L, int idx, const LuaSerializeOptions& options) -> LuaValue
    {
        std::unordered_set<const void*> visited_tables;
        return serialize_lua_value_internal(L, idx, options, 0, visited_tables);
    }

    auto push_lua_value(lua_State* L, const LuaValue& value) -> void
    {
        switch (value.type)
        {
        case LuaValue::Type::Nil:
            lua_pushnil(L);
            break;

        case LuaValue::Type::Boolean:
            lua_pushboolean(L, std::get<bool>(value.data) ? 1 : 0);
            break;

        case LuaValue::Type::Integer:
            lua_pushinteger(L, static_cast<lua_Integer>(std::get<int64_t>(value.data)));
            break;

        case LuaValue::Type::Number:
            lua_pushnumber(L, std::get<double>(value.data));
            break;

        case LuaValue::Type::String:
        {
            const auto& str = std::get<std::string>(value.data);
            lua_pushlstring(L, str.c_str(), str.size());
            break;
        }

        case LuaValue::Type::Table:
        {
            const auto& entries = std::get<std::vector<std::pair<LuaValue, LuaValue>>>(value.data);

            // Create table with hint for size
            lua_createtable(L, 0, static_cast<int>(entries.size()));

            for (const auto& [key, val] : entries)
            {
                push_lua_value(L, key);    // Push key
                push_lua_value(L, val);    // Push value
                lua_settable(L, -3);       // t[key] = value, pops key and value
            }
            break;
        }
        }
    }

    // Helper to convert LuaValue to glz::generic
    static auto lua_value_to_glz(const LuaValue& value) -> glz::generic
    {
        glz::generic result;

        switch (value.type)
        {
        case LuaValue::Type::Nil:
            result = nullptr;
            return result;

        case LuaValue::Type::Boolean:
            result = std::get<bool>(value.data);
            return result;

        case LuaValue::Type::Integer:
            // JSON doesn't distinguish int vs float, use double
            result = static_cast<double>(std::get<int64_t>(value.data));
            return result;

        case LuaValue::Type::Number:
            result = std::get<double>(value.data);
            return result;

        case LuaValue::Type::String:
            result = std::get<std::string>(value.data);
            return result;

        case LuaValue::Type::Table:
        {
            const auto& entries = std::get<std::vector<std::pair<LuaValue, LuaValue>>>(value.data);

            // Check if this is an array (sequential integer keys starting at 1)
            bool is_array = true;
            int64_t expected_index = 1;
            for (const auto& [key, val] : entries)
            {
                if (!key.is_integer() || key.as_integer() != expected_index)
                {
                    is_array = false;
                    break;
                }
                expected_index++;
            }

            if (is_array && !entries.empty())
            {
                // Convert to JSON array
                glz::generic::array_t arr;
                arr.reserve(entries.size());
                for (const auto& [key, val] : entries)
                {
                    arr.push_back(lua_value_to_glz(val));
                }
                result = std::move(arr);
                return result;
            }
            else
            {
                // Convert to JSON object (keys must be strings)
                glz::generic::object_t obj;
                for (const auto& [key, val] : entries)
                {
                    std::string key_str;
                    if (key.is_string())
                    {
                        key_str = key.as_string();
                    }
                    else if (key.is_integer())
                    {
                        key_str = std::to_string(key.as_integer());
                    }
                    else if (key.is_number())
                    {
                        key_str = std::to_string(key.as_number());
                    }
                    else if (key.is_boolean())
                    {
                        key_str = key.as_boolean() ? "true" : "false";
                    }
                    else
                    {
                        key_str = "null";
                    }
                    obj[key_str] = lua_value_to_glz(val);
                }
                result = std::move(obj);
                return result;
            }
        }
        }

        result = nullptr;
        return result;
    }

    // Helper to convert glz::generic to LuaValue
    static auto glz_to_lua_value(const glz::generic& json) -> LuaValue
    {
        if (json.is_null())
        {
            return LuaValue{};
        }
        else if (json.is_boolean())
        {
            return LuaValue{json.get<bool>()};
        }
        else if (json.is_number())
        {
            double num = json.get<double>();
            // Check if it's an integer
            if (num == static_cast<double>(static_cast<int64_t>(num)) &&
                num >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
                num <= static_cast<double>(std::numeric_limits<int64_t>::max()))
            {
                return LuaValue{static_cast<int64_t>(num)};
            }
            return LuaValue{num};
        }
        else if (json.is_string())
        {
            return LuaValue{json.get<std::string>()};
        }
        else if (json.is_array())
        {
            const auto& arr = json.get<glz::generic::array_t>();
            std::vector<std::pair<LuaValue, LuaValue>> entries;
            entries.reserve(arr.size());
            int64_t index = 1;  // Lua arrays are 1-indexed
            for (const auto& item : arr)
            {
                entries.emplace_back(LuaValue{index}, glz_to_lua_value(item));
                index++;
            }
            return LuaValue{std::move(entries)};
        }
        else if (json.is_object())
        {
            const auto& obj = json.get<glz::generic::object_t>();
            std::vector<std::pair<LuaValue, LuaValue>> entries;
            entries.reserve(obj.size());
            for (const auto& [key, val] : obj)
            {
                entries.emplace_back(LuaValue{key}, glz_to_lua_value(val));
            }
            return LuaValue{std::move(entries)};
        }
        return LuaValue{};
    }

    auto lua_value_to_json(const LuaValue& value, bool pretty) -> std::string
    {
        glz::generic json = lua_value_to_glz(value);

        // Use dump() to serialize generic to string
        auto result = json.dump();
        if (!result)
        {
            throw std::runtime_error("Failed to serialize to JSON");
        }

        if (pretty)
        {
            // Prettify the JSON string
            return glz::prettify_json(result.value());
        }

        return result.value();
    }

    auto json_to_lua_value(const std::string& json_str) -> LuaValue
    {
        glz::generic json;
        auto ec = glz::read_json(json, json_str);
        if (ec)
        {
            throw std::runtime_error("Failed to parse JSON: " + std::string(glz::format_error(ec, json_str)));
        }
        return glz_to_lua_value(json);
    }

} // namespace RC::LuaType
