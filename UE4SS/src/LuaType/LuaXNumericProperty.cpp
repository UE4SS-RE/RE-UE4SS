#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXNumericProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Property/FNumericProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    XNumericProperty::XNumericProperty(Unreal::FNumericProperty* object) : RemoteObjectBase<Unreal::FNumericProperty, FNumericPropertyName>(object)
    {
    }

    auto XNumericProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FNumericProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XNumericProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FNumericProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XNumericProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XNumericProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FNumericProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XNumericProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XNumericProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XNumericProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("IsFloatingPoint", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XNumericProperty>();
            lua.set_bool(lua_object.get_remote_cpp_object()->IsFloatingPoint());
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
