#include <LuaType/LuaFText.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/FString.hpp>

namespace RC::LuaType
{
    FText::FText(Unreal::FText* object) : RemoteObjectBase<Unreal::FText, FTextName>(object) {}

    auto FText::construct(const LuaMadeSimple::Lua& lua, Unreal::FText* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FText lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FText>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FText::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FText::setup_metamethods(BaseObject&) -> void
    {
        // AActor has no metamethods
    }

    template<LuaMadeSimple::Type::IsFinal is_final>
    auto FText::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("ToString", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<FText>();

            lua.set_string(to_string(lua_object.get_remote_cpp_object()->ToString()));

            return 1;
        });

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
