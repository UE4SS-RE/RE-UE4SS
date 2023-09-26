#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXInterfaceProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Property/FInterfaceProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    XInterfaceProperty::XInterfaceProperty(Unreal::FInterfaceProperty* object) : RemoteObjectBase<Unreal::FInterfaceProperty, FInterfacePropertyName>(object)
    {
    }

    auto XInterfaceProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FInterfaceProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XInterfaceProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FInterfaceProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XInterfaceProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XInterfaceProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FInterfaceProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XInterfaceProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XInterfaceProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XInterfaceProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("GetInterfaceClass", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XInterfaceProperty>();
            LuaType::UClass::construct(lua, lua_object.get_remote_cpp_object()->GetInterfaceClass());
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
            // table.make_global(ClassName::ToString());
        }
    }
} // namespace RC::LuaType
