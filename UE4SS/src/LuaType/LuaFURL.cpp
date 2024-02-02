#include <LuaType/LuaFURL.hpp>

namespace RC::LuaType
{
    FURL::FURL(Unreal::FURL object) : LocalObjectBase<Unreal::FURL, FURLName>(std::move(object))
    {
    }

    auto FURL::construct(const LuaMadeSimple::Lua& lua, Unreal::FURL unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FURL lua_object{unreal_object};

        auto metatable_name = "FURLUserdata";

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FURL>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FURL::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FURL::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Call, [](const LuaMadeSimple::Lua& lua) -> int {
            // todo: add metamethods

            return 1;
        });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto FURL::setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void
    {
        // todo: setup member functions
    }
} // namespace RC::LuaType
