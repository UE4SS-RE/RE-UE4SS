#include <LuaType/LuaUInt64.hpp>

namespace RC::LuaType
{
    UInt64::UInt64(uint64_t number) : LocalObjectBase<uint64_t, UInt64Name>(std::move(number))
    {
    }

    auto UInt64::construct(const LuaMadeSimple::Lua& lua, uint64_t number) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::UInt64 lua_object{number};

        static constexpr auto metatable_name = ClassName::ToString();

        auto table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            lua.new_metatable<LuaType::UInt64>(metatable_name, lua_object.get_metamethods());
        }

        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());
        return LuaMadeSimple::Lua::Table{lua};
    }

    auto UInt64::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::BinaryAnd, [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<LuaType::UInt64>();
            auto& lua_object_other = lua.get_userdata<LuaType::UInt64>();
            LuaType::UInt64::construct(lua, lua_object.get_local_cpp_object() & lua_object_other.get_local_cpp_object());
            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::BinaryOr, [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<LuaType::UInt64>();
            auto& lua_object_other = lua.get_userdata<LuaType::UInt64>();
            LuaType::UInt64::construct(lua, lua_object.get_local_cpp_object() | lua_object_other.get_local_cpp_object());
            return 1;
        });
    }
} // namespace RC::LuaType
