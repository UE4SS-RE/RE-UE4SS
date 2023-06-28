#ifndef RC_UVTD_HPP
#define RC_UVTD_HPP

#include <filesystem>
#include <unordered_set>
#include <set>
#include <map>
#include <utility>

#include <File/File.hpp>
#include <Input/Handler.hpp>
#include <Function/Function.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

#include <dia2.h>
#include <atlbase.h>

namespace RC::UVTD
{
    extern bool processing_events;
    extern Input::Handler input_handler;

    enum class DumpMode { VTable, MemberVars, SolBindings };
    auto main(DumpMode) -> void;

    enum class ValidForVTable { Yes, No };
    enum class ValidForMemberVars { Yes, No };

    struct SymbolNameInfo
    {
        ValidForVTable valid_for_vtable{};
        ValidForMemberVars valid_for_member_vars{};

        explicit SymbolNameInfo(ValidForVTable valid_for_vtable, ValidForMemberVars valid_for_member_vars) :
                valid_for_vtable(valid_for_vtable),
                valid_for_member_vars(valid_for_member_vars)
        {
        }
    };

    class VTableDumper
    {
    private:
        std::filesystem::path pdb_file;
        CComPtr<IDiaDataSource> dia_source;
        CComPtr<IDiaSession> dia_session;
        CComPtr<IDiaSymbol> dia_global_symbol;
        CComPtr<IDiaEnumSymbols> dia_global_symbols_enum;
        bool is_425_plus;

        bool are_symbols_cached{};

    public:
        VTableDumper() = delete;
        explicit VTableDumper(std::filesystem::path pdb_file) : pdb_file(std::move(pdb_file))
        {
            auto version_string = this->pdb_file.filename().stem().string();
            auto major_version = std::atoi(version_string.substr(0, 1).c_str());
            auto minor_version = std::atoi(version_string.substr(2).c_str());
            is_425_plus = (major_version > 4) || (major_version == 4 && minor_version >= 25);
        }

    public:
        auto setup_symbol_loader() -> void;
        using EnumEntriesTypeAlias = struct EnumEntry*;
        auto dump_vtable_for_symbol(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo&, EnumEntriesTypeAlias = nullptr, struct Class* class_entry = nullptr) -> void;
        auto dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void;
        auto dump_member_variable_layouts(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo&, EnumEntriesTypeAlias = nullptr, struct Class* class_entry = nullptr) -> void;
        auto dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void;
        auto generate_code(DumpMode) -> void;
        auto experimental_generate_members() -> void;
    };

    struct MemberVariable
    {
        File::StringType type;
        File::StringType name;
        int32_t offset;
    };

    struct EnumEntry
    {
        File::StringType name;
        File::StringType name_clean;
        std::map<File::StringType, MemberVariable> variables;
    };

    struct EnumEntries
    {
        std::map<File::StringType, EnumEntry> entries;
    };
    extern std::unordered_map<File::StringType, EnumEntry> g_enum_entries;

    struct FunctionBody
    {
        File::StringType name;
        File::StringType signature;
        uint32_t offset;
        bool is_overload;
    };
    struct Class
    {
        File::StringType class_name;
        File::StringType class_name_clean;
        std::map<uint32_t, FunctionBody> functions;
        // Key: Variable name
        std::map<File::StringType, MemberVariable> variables;
        uint32_t last_virtual_offset;
        ValidForVTable valid_for_vtable{ValidForVTable::No};
        ValidForMemberVars valid_for_member_vars{ValidForMemberVars::No};
    };
    struct Classes
    {
        // Key: Class name clean
        std::unordered_map<File::StringType, Class> entries;
    };
    // Key: PDB name (e.g. 4_22)
    extern std::unordered_map<File::StringType, Classes> g_class_entries;
}

#endif //RC_UVTD_HPP
