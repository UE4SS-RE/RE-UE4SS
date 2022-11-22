#include <LuaType/LuaFName.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

namespace RC::LuaType
{
    FName::FName(Unreal::FName object) : LocalObjectBase<Unreal::FName, FNameName>(std::move(object)) {}

    auto FName::construct(const LuaMadeSimple::Lua& lua, Unreal::FName unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FName lua_object{unreal_object};

        auto metatable_name = "FNameUserdata";

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FName>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FName::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FName::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Call, []([[maybe_unused]]const LuaMadeSimple::Lua& lua) -> int {
            // Acts as a "constructor" for FName
            std::string error_overload_not_found{R"(
No overload found for function 'FName'.
Overloads:
#1: FName(string Name)
#2: FName(integer ComparisonIndex))"};

            Unreal::FName fname;

            if (lua.is_userdata())
            {
                // Discard the userdata since it's unwanted data
                lua.discard_value();
            }

            if (lua.is_string())
            {
                std::wstring name = to_wstring(lua.get_string());

                fname = Unreal::FName(name);
            }
            else if (lua.is_integer())
            {
                int64_t comparison_index = lua.get_integer();
                fname = Unreal::FName(comparison_index);
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            LuaType::FName::construct(lua, fname);

            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Equal, []([[maybe_unused]]const LuaMadeSimple::Lua& lua) -> int {
            if (!lua.is_userdata(1) || !lua.is_userdata(2))
            {
                lua.throw_error("FName __eq metamethod called but there was not two userdata to compare");
            }

            auto name_a = lua.get_userdata<LuaType::FName>();
            auto name_b = lua.get_userdata<LuaType::FName>();

            return name_a.get_local_cpp_object() == name_b.get_local_cpp_object();
        });
    }

    template<LuaMadeSimple::Type::IsFinal is_final>
    auto FName::setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("ToString", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<FName>();

            lua.set_string(to_string(lua_object.get_local_cpp_object().ToString()));

            return 1;
        });

        table.add_pair("GetComparisonIndex", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<FName>();

            lua.set_integer(lua_object.get_local_cpp_object().GetComparisonIndex());

            return 1;
        });

        table.add_pair("Equals", [](const LuaMadeSimple::Lua& lua) -> int {
            if (!lua.is_userdata(1) || !lua.is_userdata(2))
            {
                lua.throw_error("FName.Equals called but there was not two userdata to compare (use ':' to call, not '.')");
            }

            auto name_a = lua.get_userdata<LuaType::FName>();
            auto name_b = lua.get_userdata<LuaType::FName>();

            return name_a.get_local_cpp_object().Equals(name_b.get_local_cpp_object());
        });

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.set_string(ClassName::ToString());
                return 1;
            });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            //table.make_global("FNameUserdata");
        }
    }
}
