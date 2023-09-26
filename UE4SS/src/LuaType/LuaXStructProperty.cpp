#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaUScriptStruct.hpp>
#include <LuaType/LuaXStructProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Property/FStructProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    XStructProperty::XStructProperty(Unreal::FStructProperty* object) : RemoteObjectBase<Unreal::FStructProperty, FStructPropertyName>(object)
    {
    }

    auto XStructProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FStructProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XStructProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FStructProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XStructProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;

        return table;
    }

    auto XStructProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FStructProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XStructProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XStructProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XStructProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("GetStruct", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<XStructProperty>();
            auto script_struct_wrapper = ScriptStructWrapper{lua_object.get_remote_cpp_object()->GetStruct(), nullptr, lua_object.get_remote_cpp_object()};
            LuaType::UScriptStruct::construct(lua, script_struct_wrapper);
            return 1;
        });

        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.set_string(ClassName::ToString());
                return 1;
            });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            // table.make_global(ClassName::ToString());
        }
    }
} // namespace RC::LuaType
