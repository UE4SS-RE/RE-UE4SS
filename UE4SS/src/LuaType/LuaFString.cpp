#include <Helpers/String.hpp>
#include <LuaType/LuaFString.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FString.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    FString::FString(Unreal::FString* object) : LocalObjectBase<Unreal::FString, FStringName>(std::move(Unreal::FString(*object)))
    {
    }

    auto FString::construct(const LuaMadeSimple::Lua& lua, Unreal::FString* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FString lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FString>(metatable_name, lua_object.get_metamethods());
        }

        // Transfer the object & its ownership fully to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FString::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FString>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FString::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // FString has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto FString::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("ToString", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<LuaType::FString>();

            const wchar_t* string_data = lua_object.get_local_cpp_object().GetCharArray();
            if (string_data)
            {
                lua.set_string(to_string(UEStringType(string_data)));
            }
            else
            {
                lua.set_string("");
            }

            return 1;
        });

        table.add_pair("Clear", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<LuaType::FString>();

            lua_object.get_local_cpp_object().Clear();

            return 0;
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
