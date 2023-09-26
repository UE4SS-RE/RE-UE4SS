#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaXFieldClass.hpp>

namespace RC::LuaType
{
    XFieldClass::XFieldClass(Unreal::FFieldClassVariant object) : LocalObjectBase<Unreal::FFieldClassVariant, FFieldClassName>(std::move(object))
    {
    }

    auto XFieldClass::construct(const LuaMadeSimple::Lua& lua, Unreal::FFieldClassVariant unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XFieldClass lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XFieldClass>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XFieldClass::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XFieldClass::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XFieldClass has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XFieldClass::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("GetFName", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<XFieldClass>();
            FName::construct(lua, lua_object.get_local_cpp_object().GetFName());
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
