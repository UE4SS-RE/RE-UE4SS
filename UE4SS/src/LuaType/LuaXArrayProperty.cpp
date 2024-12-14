#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXArrayProperty.hpp>
#include <LuaType/LuaXProperty.hpp>

#include <Unreal/Property/FArrayProperty.hpp>

namespace RC::LuaType
{
    XArrayProperty::XArrayProperty(Unreal::FArrayProperty* object) : RemoteObjectBase<Unreal::FArrayProperty, FArrayPropertyName>(object)
    {
    }

    auto XArrayProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FArrayProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XArrayProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FArrayProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XArrayProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XArrayProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FArrayProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XArrayProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XArrayProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XArrayProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("GetInner", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<XArrayProperty>();
            LuaType::auto_construct_property(lua, lua_object.get_remote_cpp_object()->GetInner());
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
