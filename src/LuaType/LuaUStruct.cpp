#include <LuaType/LuaUStruct.hpp>
#include <LuaType/LuaUFunction.hpp>
#include <LuaType/LuaXProperty.hpp>
#include <Unreal/UStruct.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FProperty.hpp>
#include <DynamicOutput/Output.hpp>

namespace RC::LuaType
{
    UStruct::UStruct(Unreal::UStruct* object) : UObjectBase<Unreal::UStruct, UStructName>(object) {}

    auto UStruct::construct(const LuaMadeSimple::Lua& lua, Unreal::UStruct* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object);

        LuaType::UStruct lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UStruct>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UStruct::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UStruct::setup_metamethods([[maybe_unused]]BaseObject& base_object) -> void
    {
        // UClass has no metamethods
    }

    template<LuaMadeSimple::Type::IsFinal is_final>
    auto UStruct::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("GetSuperStruct", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<UStruct>();

            LuaType::UStruct::construct(lua, static_cast<Unreal::UClass*>(lua_object.get_remote_cpp_object()->GetSuperStruct()));

            return 1;
        });

        table.add_pair("ForEachFunction", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<UStruct>();

            lua_object.get_remote_cpp_object()->ForEachFunction([&](Unreal::UFunction* function) {
                // Duplicate the Lua function so that we can use it in subsequent iterations of this loop (call_function pops the function from the stack)
                lua_pushvalue(lua.get_lua_state(), 1);

                // Set the 'Function' parameter for the Lua function (P1)
                LuaType::UFunction::construct(lua, lua_object.get_remote_cpp_object(), function);

                lua.call_function(1, 1);

                // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                if (lua.is_bool(2) && lua.get_bool(2))
                {
                    return LoopAction::Break;
                }
                else
                {
                    // There's a 'nil' on the stack because we told Lua that we expect a return value.
                    // Lua will put 'nil' on the stack if the Lua function doesn't explicitly return anything.
                    // We discard the 'nil' here, otherwise the Lua stack is corrupted on the next iteration of the 'ForEachFunction' loop.
                    // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                    lua.discard_value(2);
                    return LoopAction::Continue;
                }
            });

            return 1;
        });

        table.add_pair("ForEachProperty", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<UStruct>();

            lua_object.get_remote_cpp_object()->ForEachProperty([&](Unreal::FProperty* property) {
                // Duplicate the Lua function so that we can use it in subsequent iterations of this loop (call_function pops the function from the stack)
                lua_pushvalue(lua.get_lua_state(), 1);

                // Set the 'Property' parameter for the Lua function (P1)
                auto_construct_property(lua, property);

                lua.call_function(1, 1);

                // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                if (lua.is_bool(2) && lua.get_bool(2))
                {
                    return LoopAction::Break;
                }
                else
                {
                    // There's a 'nil' on the stack because we told Lua that we expect a return value.
                    // Lua will put 'nil' on the stack if the Lua function doesn't explicitly return anything.
                    // We discard the 'nil' here, otherwise the Lua stack is corrupted on the next iteration of the 'ForEachFunction' loop.
                    // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                    lua.discard_value(2);
                    return LoopAction::Continue;
                }
            });

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
            //table.make_global(ClassName::ToString());
        }
    }
}
