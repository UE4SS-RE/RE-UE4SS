#include <LuaType/LuaUEnum.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXEnumProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Property/FEnumProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    XEnumProperty::XEnumProperty(Unreal::FEnumProperty* object) : RemoteObjectBase<Unreal::FEnumProperty, FEnumPropertyName>(object)
    {
    }

    auto XEnumProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FEnumProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XEnumProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FEnumProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XEnumProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XEnumProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FEnumProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XEnumProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XEnumProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XEnumProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("GetEnum", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<XEnumProperty>();
            LuaType::UEnum::construct(lua, lua_object.get_remote_cpp_object()->GetEnum());
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
