#include <DynamicOutput/DynamicOutput.hpp>
#include <LuaType/LuaFText.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/FText.hpp>

namespace RC::LuaType
{
    FText::FText(Unreal::FText object) : LocalObjectBase<Unreal::FText, FTextName>(std::move(object))
    {
    }

    auto FText::construct(const LuaMadeSimple::Lua& lua, Unreal::FText unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FText lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FText>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FText::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FText::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Call, []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
            // Acts as a "constructor" for FText
            std::string error_overload_not_found{R"(
No overload found for function 'FText'.
Overloads:
#1: FText(string Text)
)"};

            if (lua.is_userdata())
            {
                // Discard the userdata since it's unwanted data
                lua.discard_value();
            }

            SystemStringType text_string{};
            if (lua.is_string())
            {
                text_string = to_system(lua.get_string());
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            LuaType::FText::construct(lua, Unreal::FText(to_ue(text_string)));

            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Equal, []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
            if (!lua.is_userdata(1) || !lua.is_userdata(2))
            {
                lua.throw_error("FText __eq metamethod called but there was not two userdata to compare");
            }

            auto text_a = lua.get_userdata<LuaType::FText>();
            auto text_b = lua.get_userdata<LuaType::FText>();
            // FText objects are equal if their string representations are equal
            return text_a.get_local_cpp_object().ToString() == text_b.get_local_cpp_object().ToString();
        });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto FText::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("ToString", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<FText>();

            const auto& fstring = lua_object.get_local_cpp_object().ToFString();
            lua.set_string(to_string(fstring.GetCharArray()));

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
