#include <DynamicOutput/Output.hpp>
#include <LuaType/LuaFWeakObjectPtr.hpp>
#include <LuaType/LuaUObject.hpp>

namespace RC::LuaType
{
    FWeakObjectPtr::FWeakObjectPtr(Unreal::FWeakObjectPtr object) : LocalObjectBase<Unreal::FWeakObjectPtr, FWeakObjectPtrName>(std::move(object))
    {
    }

    auto FWeakObjectPtr::construct(const LuaMadeSimple::Lua& lua, Unreal::FWeakObjectPtr unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FWeakObjectPtr lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FWeakObjectPtr>(metatable_name, lua_object.get_metamethods());
        }

        // Transfer the object & its ownership fully to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FWeakObjectPtr::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FWeakObjectPtr::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // FWeakObjectPtr has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto FWeakObjectPtr::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("Get", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<FWeakObjectPtr>();
            LuaType::auto_construct_object(lua, lua_object.get_local_cpp_object().Get());
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
