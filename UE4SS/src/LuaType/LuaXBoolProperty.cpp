#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXBoolProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Property/FBoolProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    XBoolProperty::XBoolProperty(Unreal::FBoolProperty* object) : RemoteObjectBase<Unreal::FBoolProperty, FBoolPropertyName>(object)
    {
    }

    auto XBoolProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FBoolProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XBoolProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FBoolProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XBoolProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XBoolProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FBoolProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XBoolProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XBoolProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XBoolProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("GetByteMask", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XBoolProperty>();
            lua.set_integer(lua_object.get_remote_cpp_object()->GetByteMask());
            return 1;
        });

        table.add_pair("GetByteOffset", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XBoolProperty>();
            lua.set_integer(lua_object.get_remote_cpp_object()->GetByteOffset());
            return 1;
        });

        table.add_pair("GetFieldMask", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XBoolProperty>();
            lua.set_integer(lua_object.get_remote_cpp_object()->GetFieldMask());
            return 1;
        });

        table.add_pair("GetFieldSize", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XBoolProperty>();
            lua.set_integer(lua_object.get_remote_cpp_object()->GetFieldSize());
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
