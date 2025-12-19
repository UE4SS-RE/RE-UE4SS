#include <limits>

#include <fmt/std.h>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <LuaType/LuaThreadId.hpp>

namespace RC::LuaType
{
    ThreadId::ThreadId(std::thread::id object) : LocalObjectBase<std::thread::id, ThreadIdName>(std::move(object))
    {
    }

    auto ThreadId::construct(const LuaMadeSimple::Lua& lua, std::thread::id unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::ThreadId lua_object{unreal_object};

        auto metatable_name = "ThreadIdUserdata";

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::ThreadId>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto ThreadId::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto ThreadId::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Equal, []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
            if (!lua.is_userdata(1) || !lua.is_userdata(2))
            {
                lua.throw_error("ThreadId __eq metamethod called but there was not two userdata to compare");
            }

            auto a = lua.get_userdata<LuaType::ThreadId>();
            auto b = lua.get_userdata<LuaType::ThreadId>();

            return a.get_local_cpp_object() == b.get_local_cpp_object();
        });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto ThreadId::setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("ToString", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<ThreadId>();

            lua.set_string(fmt::format("{}", lua_object.get_local_cpp_object()));

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
            // table.make_global("ThreadIdUserdata");
        }
    }
} // namespace RC::LuaType
