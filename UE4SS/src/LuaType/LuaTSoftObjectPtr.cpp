#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <LuaType/LuaFSoftObjectPath.hpp>
#include <LuaType/LuaFWeakObjectPtr.hpp>
#include <LuaType/LuaTSoftObjectPtr.hpp>

namespace RC::LuaType
{
    TSoftObjectPtr::TSoftObjectPtr(Unreal::FSoftObjectPtr object) : LocalObjectBase<Unreal::FSoftObjectPtr, TSoftObjectPtrName>(std::move(object))
    {
    }

    auto TSoftObjectPtr::construct(const LuaMadeSimple::Lua& lua, Unreal::FSoftObjectPtr& unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::TSoftObjectPtr lua_object{unreal_object};

        auto metatable_name = "TSoftObjectPtrUserdata";

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table, metatable_name);
            lua.new_metatable<LuaType::TSoftObjectPtr>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto TSoftObjectPtr::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();
        ;

        auto metatable_name = "TSoftObjectPtrUserdata";

        setup_metamethods(construct_to);
        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table, metatable_name);

        return table;
    }

    auto TSoftObjectPtr::setup_metamethods(BaseObject& base_object) -> void
    {
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto TSoftObjectPtr::setup_member_functions(LuaMadeSimple::Lua::Table& table, std::string_view metatable_name) -> void
    {
        table.add_pair("GetWeakPtr", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TSoftObjectPtr>();
            FWeakObjectPtr::construct(lua, lua_object.get_local_cpp_object().WeakPtr);
            return 1;
        });

        table.add_pair("GetTagAtLastTest", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TSoftObjectPtr>();
            lua.set_integer(lua_object.get_local_cpp_object().TagAtLastTest);
            return 1;
        });

        table.add_pair("GetObjectID", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TSoftObjectPtr>();
            FSoftObjectPath::construct(lua, lua_object.get_local_cpp_object().ObjectID);
            return 1;
        });

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.set_string("TSoftObjectPtrUserdata");
                return 1;
            });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            // table.make_global(metatable_name);// , is_final == LuaMadeSimple::Type::IsFinal::No);
        }
    }
} // namespace RC::LuaType
