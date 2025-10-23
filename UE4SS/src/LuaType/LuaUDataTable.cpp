#include <LuaType/LuaUDataTable.hpp>
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

                               // Convert row data to Lua table
                               Unreal::int32 comparison_index = static_cast<Unreal::int32>(
                                   row_struct->GetNamePrivate().GetComparisonIndex());

                               if (StaticState::m_property_value_pushers.contains(comparison_index))
                               {
                                   PusherParams pusher_params{
                                           .operation = LuaMadeSimple::Type::Operation::GetParam,
                                           .lua = lua,
                                           .base = nullptr,
                                           .data = Pair.Value,
                                           .property = nullptr
                                   };
                                   StaticState::m_property_value_pushers[comparison_index](pusher_params);
                               }
                               else
                               {
                                   // Use the helper function for struct conversion
                                   convert_struct_to_lua_table(lua, row_struct, Pair.Value, true, nullptr);
                               }

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

                               // Push row data as second parameter
                               PusherParams pusher_params{
                                       .operation = LuaMadeSimple::Type::Operation::GetParam,
                                       .lua = lua,
                                       .base = nullptr,
                                       .data = Pair.Value,
                                       .property = nullptr
                               };

                               // Check if we have a pusher for the row struct type
                               Unreal::int32 comparison_index = static_cast<Unreal::int32>(info.row_struct_fname.GetComparisonIndex());
                               if (StaticState::m_property_value_pushers.contains(comparison_index))
                               {
                                   StaticState::m_property_value_pushers[comparison_index](pusher_params);
                               }
                               else
                               {
                                   // Use the helper function for struct conversion
                                   convert_struct_to_lua_table(lua, info.row_struct, Pair.Value, true, nullptr);
                               }

                               // Call function with row name and row data
                               lua.call_function(2, 0);
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

            // Push row data
            PusherParams pusher_params{
                    .operation = LuaMadeSimple::Type::Operation::GetParam,
                    .lua = lua,
                    .base = nullptr,
                    .data = row_data,
                    .property = nullptr
            };

            Unreal::int32 comparison_index = static_cast<Unreal::int32>(info.row_struct_fname.GetComparisonIndex());
            if (StaticState::m_property_value_pushers.contains(comparison_index))
            {
                StaticState::m_property_value_pushers[comparison_index](pusher_params);
            }
            else
            {
                // Use the helper function for struct conversion
                convert_struct_to_lua_table(lua, info.row_struct, row_data, true, nullptr);
            }
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

            // After get_string(1), the table is now at position 1 (not 2!)
            if (!lua.is_table(1))
            {
                lua.throw_error("AddRow expects a table as the second parameter");
            }

            // Allocate memory for new row
            Unreal::uint8* new_row_data = (Unreal::uint8*)Unreal::FMemory::Malloc(info.row_size);
            info.row_struct->InitializeStruct(new_row_data);

            // Get row data from Lua - now at position 1
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

            // Add row to table
            data_table->AddRow(row_name, new_row_data, info.row_struct);
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
                if (StaticState::m_property_value_pushers.contains(comparison_index))
                {
                    PusherParams pusher_params{
                            .operation = LuaMadeSimple::Type::Operation::GetParam,
                            .lua = lua,
                            .base = nullptr,
                            .data = Pair.Value,
                            .property = nullptr
                    };
                    StaticState::m_property_value_pushers[comparison_index](pusher_params);
                }
                else
                {
                    // Use the helper function for struct conversion
                    convert_struct_to_lua_table(lua, info.row_struct, Pair.Value, true, nullptr);
                }
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