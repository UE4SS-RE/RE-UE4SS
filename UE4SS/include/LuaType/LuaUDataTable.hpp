#pragma once
#include <LuaType/LuaUObject.hpp>


namespace RC::Unreal
{
    class UDataTable;
}

namespace RC::LuaType
{
    struct UDataTableName
    {
        constexpr static const char* ToString()
        {
            return "UDataTable";
        }
    };

    class UDataTable : public RemoteObjectBase<Unreal::UDataTable, UDataTableName>
    {
    private:
        explicit UDataTable(const PusherParams&);

    public:
        UDataTable() = delete;

        auto static construct(const PusherParams&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua& lua, Unreal::UDataTable* unreal_object) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;

        enum class DataTableOperation
        {
            FindRow,
            AddRow,
            RemoveRow,
            EmptyTable,
            GetRowNames,
            GetAllRows,
        };

        auto static prepare_to_handle(DataTableOperation, const LuaMadeSimple::Lua&) -> void;
    };

    struct FDataTableInfo
    {
        Unreal::UDataTable* data_table{};
        Unreal::UScriptStruct* row_struct{};
        Unreal::FName row_struct_fname{};
        size_t row_size{};

        FDataTableInfo(Unreal::UDataTable* table);

        /**
        * Validates that the DataTable has a valid row struct.
        * Throws if row struct is not found
        *
        * @param lua Lua state to throw against.
        */
        void validate_row_struct(const LuaMadeSimple::Lua& lua);
    };
}