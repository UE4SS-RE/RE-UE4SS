#include <LuaType/LuaMod.hpp>

namespace RC::LuaType
{
    Mod::Mod(RC::Mod* object) : RemoteObjectBase<RC::Mod, ModName>(object) {}

    auto Mod::construct(const LuaMadeSimple::Lua& lua, RC::Mod* cpp_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::Mod lua_object{cpp_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::Mod>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto Mod::setup_metamethods(BaseObject&) -> void
    {
        // Mod has no metamethods
    }

    template<LuaMadeSimple::Type::IsFinal is_final>
    auto Mod::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.set_string(ClassName::ToString());
                return 1;
            });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            //table.make_global(ClassName::ToString());
        }
    }
}
