#include <LuaType/LuaMod.hpp>

namespace RC::LuaType
{
    Mod::Mod(const RC::Mod* object) : RemoteObjectBase<const RC::Mod, ModName>(object) {}

    auto Mod::construct(const LuaMadeSimple::Lua& lua, const RC::Mod* cpp_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::Mod lua_object{cpp_object};

        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);

        setup_metamethods(lua_object);

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), ClassName::ToString(), lua_object.get_metamethods());

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
            table.make_global(ClassName::ToString());
        }
    }
}
