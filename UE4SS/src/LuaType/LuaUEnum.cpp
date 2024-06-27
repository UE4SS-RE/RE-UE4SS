#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUEnum.hpp>
#include <Unreal/UEnum.hpp>

namespace RC::LuaType
{
    UEnum::UEnum(Unreal::UEnum* object) : RemoteObjectBase<Unreal::UEnum, UEnumName>(object)
    {
    }

    auto UEnum::construct(const LuaMadeSimple::Lua& lua, Unreal::UEnum* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object);

        LuaType::UEnum lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UEnum>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UEnum::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UEnum::setup_metamethods(BaseObject&) -> void
    {
        // UEnum has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto UEnum::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("GetNameByValue", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.GetNameByValue'.
Overloads:
#1: GetNameByValue(integer Value))"};

            auto& lua_object = lua.get_userdata<UEnum>();

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto value = lua.get_integer();
            LuaType::FName::construct(lua, lua_object.get_remote_cpp_object()->GetNameByValue(value));
            return 1;
        });

        table.add_pair("ForEachName", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.ForEachName'.
Overloads:
#1: ForEachName(LuaFunction Callback))"};

            auto& lua_object = lua.get_userdata<UEnum>();

            for (auto& [name, value] : lua_object.get_remote_cpp_object()->ForEachName())
            {
                // Duplicate the Lua function so that we can use it in subsequent iterations of this loop (call_function pops the function from the stack)
                lua_pushvalue(lua.get_lua_state(), 1);

                // Set the 'Name' parameter for the Lua function (P1)
                LuaType::FName::construct(lua, name);

                // Set the 'Value' parameter for the Lua function (P2)
                lua.set_integer(value);

                lua.call_function(2, 1);

                // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                if (lua.is_bool(2) && lua.get_bool(2))
                {
                    break;
                }
                else
                {
                    // There's a 'nil' on the stack because we told Lua that we expect a return value.
                    // Lua will put 'nil' on the stack if the Lua function doesn't explicitly return anything.
                    // We discard the 'nil' here, otherwise the Lua stack is corrupted on the next iteration of the 'ForEachFunction' loop.
                    // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                    lua.discard_value(2);
                }
            }

            return 0;
        });

        table.add_pair("GetEnumNameByIndex", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.GetEnumNameByIndex'.
Overloads:
#1: GetNameByValue(integer Value))"};

            auto& lua_object = lua.get_userdata<UEnum>();

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto value = lua.get_integer();
            auto enum_pair = lua_object.get_remote_cpp_object()->GetEnumNameByIndex(value);

            LuaType::FName::construct(lua, enum_pair.Key);
            lua.set_integer(enum_pair.Value);

            return 2;
        });

        table.add_pair("InsertIntoNames", [](const LuaMadeSimple::Lua& lua) -> int {
            // Function: InsertIntoNames
            // Param #1: Enum key name
            // Param #2: Enum value
            // Param #3: Enum index
            // (Optional) Param #4: Shift values
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.InsertIntoNames'.
Overloads:
#1: InsertIntoNames(string Name, integer Value, integer Index) // shiftValues = false
#1: InsertIntoNames(string Name, integer Value, integer Index, bool shiftValues)"};

            auto& lua_object = lua.get_userdata<UEnum>();

            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'UEnum.InsertIntoNames' cannot be called with 0 parameters.");
            }

            StringType param_name{};
            int64_t param_value = 0;
            int32_t param_index = 0;
            bool param_shift = false;

            // P1 (Name), string
            if (lua.is_string())
            {
                param_name = to_ue(lua.get_string());
            }
            else
            {
                lua.throw_error("'UEnum.InsertIntoNames' could not load parameter for \"Name\"");
            }

            // P2 (Value), integer
            if (lua.is_integer())
            {
                param_value = lua.get_integer();
            }
            else
            {
                lua.throw_error("'UEnum.InsertIntoNames' could not load parameter for \"Value\"");
            }

            // P3 (Index), integer
            if (lua.is_integer())
            {
                param_index = lua.get_integer();
            }
            else
            {
                lua.throw_error("'UEnum.InsertIntoNames' could not load parameter for \"Index\"");
            }

            // (Optional) P4 (ShiftValues), boolean
            if (lua.is_bool())
            {
                param_shift = lua.get_bool();
            }

            const Unreal::FName key{param_name, Unreal::FNAME_Add};
            const auto pair = Unreal::TPair<Unreal::FName, int64_t>{key, param_value};

            lua_object.get_remote_cpp_object()->InsertIntoNames(pair, param_index, param_shift);
            return 1;
        });

        table.add_pair("EditNameAt", [](const LuaMadeSimple::Lua& lua) -> int {
            // Function: EditNameAt
            // Param #1: Enum key index
            // Param #2: New name for index
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.EditNameAt'.
Overloads:
#1: EditNameAt(integer Index, string NewName))"};

            auto& lua_object = lua.get_userdata<UEnum>();

            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'UEnum.EditValueAt' cannot be called with 0 parameters.");
            }

            int32_t param_index = 0;
            std::string param_new_name{};

            // P1 (Index), integer
            if (lua.is_integer())
            {
                param_index = lua.get_integer();
            }
            else
            {
                lua.throw_error("'UEnum.EditNameAt' could not load parameter for \"Index\"");
            }

            // P2 (NewName), string
            if (lua.is_string())
            {
                param_new_name = lua.get_string();
            }
            else
            {
                lua.throw_error("'UEnum.EditNameAt' could not load parameter for \"NewName\"");
            }

            Unreal::FName new_key = Unreal::FName(to_ue(param_new_name), Unreal::FNAME_Add);
            lua_object.get_remote_cpp_object()->EditNameAt(param_index, new_key);

            return 0;
        });

        table.add_pair("EditValueAt", [](const LuaMadeSimple::Lua& lua) -> int {
            // Function: EditValueAt
            // Param #1: Enum key Index
            // Param #2: New value for Index
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.EditValueAt'.
Overloads:
#1: EditValueAt(integer Index, integer NewValue))"};

            auto& lua_object = lua.get_userdata<UEnum>();

            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'UEnum.EditValueAt' cannot be called with 0 parameters.");
            }

            int32_t param_index = 0;
            int64_t param_new_value = 0;

            // P1 (Index), integer
            if (lua.is_integer())
            {
                param_index = lua.get_integer();
            }
            else
            {
                lua.throw_error("'UEnum.EditValueAt' Could not load parameter for \"Index\"");
            }

            // P2 (NewValue), integer
            if (lua.is_integer())
            {
                param_new_value = lua.get_integer();
            }
            else
            {
                lua.throw_error("'UEnum.EditValueAt' Could not load parameter for \"NewValue\"");
            }

            lua_object.get_remote_cpp_object()->EditValueAt(param_index, param_new_value);
            return 0;
        });

        table.add_pair("RemoveFromNamesAt", [](const LuaMadeSimple::Lua& lua) -> int {
            // Function: InsertIntoNames
            // Param #1: Enum index
            // (Optional) Param #2: Enum entry count
            // (Optional) Param #3: Allow shrinking after removal
            std::string error_overload_not_found{R"(
No overload found for function 'UEnum.RemoveFromNamesAt'.
Overloads:
#1: RemoveFromNamesAt(integer Index) // Count = 1, AllowShrinking = true
#2: RemoveFromNamesAt(integer Index, integer Count) // AllowShrinking = true
#3: RemoveFromNamesAt(integer Index, integer Count, bool AllowShrinking))"};

            auto& lua_object = lua.get_userdata<UEnum>();

            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'UEnum.RemoveFromNamesAt' cannot be called with 0 parameters.");
            }

            int32_t param_index = 0;
            int32_t param_count = 1;
            bool param_allow_shrinking = true;

            // P1 (Index), integer
            if (lua.is_integer())
            {
                param_index = lua.get_integer();
            }
            else
            {
                lua.throw_error("'UEnum.RemoveFromNamesAt' Could not load parameter for \"Index\"");
            }

            // P2 (Count), integer
            if (lua.is_integer())
            {
                param_count = lua.get_integer();
            }

            // P3 (AllowShrinking), boolean
            if (lua.is_bool())
            {
                param_allow_shrinking = lua.get_bool();
            }

            lua_object.get_remote_cpp_object()->RemoveFromNamesAt(param_index, param_count, param_allow_shrinking);
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
