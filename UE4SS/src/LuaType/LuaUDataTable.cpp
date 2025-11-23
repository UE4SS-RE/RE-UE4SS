#include <LuaType/LuaUDataTable.hpp>
#include <LuaType/LuaUScriptStruct.hpp>
#include <Unreal/Engine/UDataTable.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>

namespace RC::LuaType
{
    UDataTable::UDataTable(Unreal::UDataTable* object)
        : UObjectBase<Unreal::UDataTable, UDataTableName>(object)
    {
    }

    auto UDataTable::construct(const LuaMadeSimple::Lua& lua, Unreal::UDataTable* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object);

        LuaType::UDataTable lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UDataTable>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UDataTable::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UDataTable::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Length,
                                             [](const LuaMadeSimple::Lua& lua) {
                                                 auto& lua_object = lua.get_userdata<UDataTable>();
                                                 auto data_table = lua_object.get_remote_cpp_object();
                                                 lua.set_integer(data_table->GetRowMap().Num());
                                                 return 1;
                                             });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto UDataTable::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("IsValid",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();
                           lua.set_bool(lua_object.get_remote_cpp_object() != nullptr);
                           return 1;
                       });

        table.add_pair("GetRowStruct",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();
                           auto data_table = lua_object.get_remote_cpp_object();

                           if (!data_table)
                           {
                               lua.set_nil();
                               return 1;
                           }

                           auto row_struct = data_table->GetRowStruct();
                           if (!row_struct)
                           {
                               lua.set_nil();
                               return 1;
                           }

                           LuaType::UObject::construct(lua, row_struct);
                           return 1;
                       });

        table.add_pair("GetRowMap",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();
                           auto data_table = lua_object.get_remote_cpp_object();

                           if (!data_table)
                           {
                               lua.set_nil();
                               return 1;
                           }

                           // Get the row struct for conversion
                           Unreal::UScriptStruct* row_struct = data_table->GetRowStruct();
                           if (!row_struct)
                           {
                               lua.set_nil();
                               return 1;
                           }

                           const Unreal::TMap<Unreal::FName, unsigned char*>& row_map = data_table->GetRowMap();

                           // Create Lua table to hold the map
                           auto lua_table = lua.prepare_new_table();

                           for (const auto& Pair : row_map)
                           {
                               // Use row name as key
                               lua_table.add_key(to_string(Pair.Key.ToString()).c_str());

                               // Create UScriptStruct wrapper for reference-based access
                               ScriptStructWrapper row_wrapper{
                                       .script_struct = row_struct,
                                       .start_of_struct = Pair.Value,
                                       .property = nullptr
                               };
                               UScriptStruct::construct(lua, row_wrapper);

                               lua_table.fuse_pair();
                           }

                           lua_table.make_local();
                           return 1;
                       });

        table.add_pair("FindRow",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(DataTableOperation::FindRow, lua);
                           return 1;
                       });

        table.add_pair("AddRow",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(DataTableOperation::AddRow, lua);
                           return 1;
                       });

        table.add_pair("RemoveRow",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(DataTableOperation::RemoveRow, lua);
                           return 1;
                       });

        table.add_pair("EmptyTable",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(DataTableOperation::EmptyTable, lua);
                           return 1;
                       });

        table.add_pair("GetRowNames",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(DataTableOperation::GetRowNames, lua);
                           return 1;
                       });

        table.add_pair("GetAllRows",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(DataTableOperation::GetAllRows, lua);
                           return 1;
                       });

        table.add_pair("ForEachRow",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           UDataTable& lua_object = lua.get_userdata<UDataTable>();
                           auto data_table = lua_object.get_remote_cpp_object();

                           if (!data_table)
                           {
                               lua.throw_error("DataTable is null");
                           }

                           FDataTableInfo info(data_table);
                           info.validate_row_struct(lua);

                           const auto& row_map = data_table->GetRowMap();

                           for (const auto& Pair : row_map)
                           {
                               // Duplicate the Lua function for next iteration
                               lua_pushvalue(lua.get_lua_state(), 1);

                               // Push row name as first parameter
                               lua.set_string(to_string(Pair.Key.ToString()));

                               // Push row data as second parameter (as UScriptStruct for reference-based access)
                               ScriptStructWrapper row_wrapper{
                                       .script_struct = info.row_struct,
                                       .start_of_struct = Pair.Value,
                                       .property = nullptr
                               };
                               UScriptStruct::construct(lua, row_wrapper);

                               // Call function with row name and row data, expecting 1 return value
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
                                   // We discard the 'nil' here, otherwise the Lua stack is corrupted on the next iteration of the 'ForEachRow' loop.
                                   // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                                   lua.discard_value(2);
                               }
                           }

                           return 0;
                       });

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type",
                           [](const LuaMadeSimple::Lua& lua) -> int {
                               lua.set_string(ClassName::ToString());
                               return 1;
                           });
        }
    }

    auto UDataTable::prepare_to_handle(const DataTableOperation operation, const LuaMadeSimple::Lua& lua) -> void
    {
        UDataTable& lua_object = lua.get_userdata<UDataTable>();
        auto data_table = lua_object.get_remote_cpp_object();

        if (!data_table)
        {
            lua.throw_error("DataTable is null");
        }

        FDataTableInfo info(data_table);

        switch (operation)
        {
        case DataTableOperation::FindRow: {
            info.validate_row_struct(lua);

            // Get row name from parameter
            Unreal::FName row_name(ensure_str(lua.get_string(1)).c_str(), Unreal::FNAME_Add);

            Unreal::uint8* row_data = data_table->FindRowUnchecked(row_name);
            if (!row_data)
            {
                lua.set_nil();
                return;
            }

            // Return a UScriptStruct wrapper that provides reference-based access to the row data
            // This matches C++ behavior where FindRow returns a pointer to the actual row memory
            ScriptStructWrapper row_wrapper{
                    .script_struct = info.row_struct,
                    .start_of_struct = row_data,
                    .property = nullptr
            };

            UScriptStruct::construct(lua, row_wrapper);
            break;
        }
        case DataTableOperation::AddRow: {
            info.validate_row_struct(lua);

            // Get row name from parameter 1 (this removes it from stack)
            if (!lua.is_string(1))
            {
                lua.throw_error("AddRow expects a string as the first parameter");
            }
            Unreal::FName row_name(ensure_str(lua.get_string(1)).c_str(), Unreal::FNAME_Add);

            // After get_string(1), the second parameter is now at position 1
            // Check if it's a UScriptStruct (reference) or a table (copy)
            bool is_struct = lua.is_userdata(1);
            bool is_table = lua.is_table(1);

            if (!is_struct && !is_table)
            {
                lua.throw_error("AddRow expects a table or UScriptStruct as the second parameter");
            }

            Unreal::uint8* new_row_data = nullptr;

            if (is_struct)
            {
                // Handle UScriptStruct - copy data from the struct
                auto& struct_wrapper = lua.get_userdata<UScriptStruct>();
                auto& wrapper_data = struct_wrapper.get_local_cpp_object();

                // Allocate new memory and copy from source
                new_row_data = (Unreal::uint8*)Unreal::FMemory::Malloc(info.row_size);
                info.row_struct->InitializeStruct(new_row_data);
                info.row_struct->CopyScriptStruct(new_row_data, wrapper_data.start_of_struct);
            }
            else  // is_table
            {
                // Handle Lua table - convert table to struct
                new_row_data = (Unreal::uint8*)Unreal::FMemory::Malloc(info.row_size);
                info.row_struct->InitializeStruct(new_row_data);

                // Get row data from Lua table at position 1
                Unreal::int32 comparison_index = static_cast<Unreal::int32>(info.row_struct_fname.GetComparisonIndex());
                if (StaticState::m_property_value_pushers.contains(comparison_index))
                {
                    // Use specific pusher if available
                    PusherParams pusher_params{
                            .operation = LuaMadeSimple::Type::Operation::Set,
                            .lua = lua,
                            .base = nullptr,
                            .data = new_row_data,
                            .property = nullptr
                    };

                    lua_pushvalue(lua.get_lua_state(), 1); // Table is at position 1 now
                    StaticState::m_property_value_pushers[comparison_index](pusher_params);
                    lua.discard_value(-1);
                }
                else
                {
                    // Use the helper function - table is at position 1
                    convert_lua_table_to_struct(lua, info.row_struct, new_row_data, 1, nullptr);
                }
            }

            // Add row to table (this copies the data into DataTable's internal storage)
            data_table->AddRow(row_name, new_row_data, info.row_struct);

            // Free our temporary allocation since AddRow copied the data
            info.row_struct->DestroyStruct(new_row_data);
            Unreal::FMemory::Free(new_row_data);

            break;
        }
        case DataTableOperation::RemoveRow: {
            // Get row name
            Unreal::FName row_name(ensure_str(lua.get_string(1)).c_str(), Unreal::FNAME_Add);
            data_table->RemoveRow(row_name);
            break;
        }
        case DataTableOperation::EmptyTable: {
            data_table->EmptyTable();
            break;
        }
        case DataTableOperation::GetRowNames: {
            auto row_names = data_table->GetRowNames();

            auto lua_table = lua.prepare_new_table();
            for (Unreal::int32 i = 0; i < row_names.Num(); i++)
            {
                lua_table.add_key(i + 1);
                lua.set_string(to_string(row_names[i].ToString()));
                lua_table.fuse_pair();
            }
            lua_table.make_local();
            break;
        }
        case DataTableOperation::GetAllRows: {
            info.validate_row_struct(lua);

            const auto& row_map = data_table->GetRowMap();

            auto lua_table = lua.prepare_new_table();
            int index = 1;
            for (const auto& Pair : row_map)
            {
                // Add index as key
                lua_table.add_key(index++);

                // Create row entry table
                auto row_table = lua.prepare_new_table();

                // Add row name
                row_table.add_key("Name");
                lua.set_string(to_string(Pair.Key.ToString()));
                row_table.fuse_pair();

                // Add row data
                row_table.add_key("Data");

                Unreal::int32 comparison_index = static_cast<Unreal::int32>(info.row_struct_fname.GetComparisonIndex());
                // Create UScriptStruct wrapper for reference-based access
                ScriptStructWrapper row_wrapper{
                        .script_struct = info.row_struct,
                        .start_of_struct = Pair.Value,
                        .property = nullptr
                };
                UScriptStruct::construct(lua, row_wrapper);
                row_table.fuse_pair();

                // Now make row_table local and fuse it into main table
                row_table.make_local();
                lua_table.fuse_pair();
            }

            lua_table.make_local();
            break;
        }
        }
    }

    FDataTableInfo::FDataTableInfo(Unreal::UDataTable* table) : data_table(table)
    {
        if (table)
        {
            row_struct = table->GetRowStruct();
            if (row_struct)
            {
                row_struct_fname = row_struct->GetClassPrivate()->GetNamePrivate();
                row_size = row_struct->GetSize();
            }
        }
    }

    void FDataTableInfo::validate_row_struct(const LuaMadeSimple::Lua& lua)
    {
        if (!row_struct)
        {
            lua.throw_error("DataTable has no RowStruct specified");
        }
    }
}