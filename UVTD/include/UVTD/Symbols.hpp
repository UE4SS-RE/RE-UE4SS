#ifndef UNREALVTABLEDUMPER_SYMBOLS_HPP
#define UNREALVTABLEDUMPER_SYMBOLS_HPP

#include <format>
#include <map>
#include <unordered_map>
#include <vector>

#include <File/File.hpp>

#include <Helpers/String.hpp>
#include <Helpers/Format.hpp>

#include <PDB_DBIStream.h>
#include <PDB_RawFile.h>
#include <PDB_TPIStream.h>

namespace RC::UVTD
{
    struct DumpSettings
    {
        bool should_dump_vtable{};
        bool should_dump_member_vars{};
        bool should_dump_sol_bindings{};
    };

    enum class ValidForVTable
    {
        Yes,
        No
    };
    enum class ValidForMemberVars
    {
        Yes,
        No
    };

    struct SymbolNameInfo
    {
        ValidForVTable valid_for_vtable{};
        ValidForMemberVars valid_for_member_vars{};

        explicit SymbolNameInfo(ValidForVTable valid_for_vtable, ValidForMemberVars valid_for_member_vars)
            : valid_for_vtable(valid_for_vtable), valid_for_member_vars(valid_for_member_vars)
        {
        }
    };

    struct MemberVariable
    {
        UEStringType type;
        UEStringType name;
        int32_t offset;
    };

    struct FunctionParam
    {
        UEStringType type;

        auto to_string() const -> File::StringType
        {
            return std::format(IOSTR("{}"), to_file(type));
        }
    };

    struct MethodSignature
    {
        UEStringType return_type;
        UEStringType name;
        std::vector<FunctionParam> params;
        bool const_qualifier;

        auto to_string() const -> File::StringType
        {
            File::StringType params_string{};

            for (size_t i = 0; i < params.size(); i++)
            {
                bool should_add_comma = i < params.size() - 1;
                params_string.append(fmtfile(IOSTR("{}{}"), params[i].to_string(), should_add_comma ? IOSTR(", ") : IOSTR("")));
            }

            return fmtfile(IOSTR("{} {}({}){};"), return_type, name, params_string, const_qualifier ? IOSTR("const") : IOSTR(""));
        }
    };

    struct MethodBody
    {
        UEStringType name;
        MethodSignature signature;
        uint32_t offset;
        bool is_overload;
    };

    struct Class
    {
        UEStringType class_name;
        UEStringType class_name_clean;
        std::map<uint32_t, MethodBody> functions;
        // Key: Variable name
        std::map<UEStringType, MemberVariable> variables;
        uint32_t last_virtual_offset{};
        ValidForVTable valid_for_vtable{ValidForVTable::No};
        ValidForMemberVars valid_for_member_vars{ValidForMemberVars::No};
    };

    class Symbols
    {
      public:
        struct MemberVariable
        {
            UEStringType type;
            int32_t offset;
        };

        struct EnumEntry
        {
            UEStringType name;
            UEStringType name_clean;
            std::map<UEStringType, MemberVariable> variables;
        };

        struct Class
        {
            UEStringType class_name;
            UEStringType class_name_clean;
            std::map<uint32_t, MethodBody> functions;
            // Key: Variable name
            std::map<UEStringType, MemberVariable> variables;
            uint32_t last_virtual_offset;
            ValidForVTable valid_for_vtable{ValidForVTable::No};
            ValidForMemberVars valid_for_member_vars{ValidForMemberVars::No};
        };

      public:
        std::filesystem::path pdb_file_path;
        File::Handle pdb_file_handle;
        std::span<uint8_t> pdb_file_map;

        PDB::RawFile pdb_file;
        PDB::DBIStream dbi_stream;
        bool is_425_plus;

        std::unordered_map<File::StringType, EnumEntry> enum_entries;
        std::unordered_map<File::StringType, Class> class_entries;

      public:
        Symbols() = delete;

        explicit Symbols(std::filesystem::path pdb_file_path);

        Symbols(const Symbols& other);

        Symbols& operator=(const Symbols& other);

      public:
        auto get_or_create_enum_entry(const UEStringType& symbol_name, const UEStringType& symbol_name_clean) -> EnumEntry&;
        auto get_or_create_class_entry(const UEStringType& symbol_name, const UEStringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&;

        auto generate_method_signature(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::Record* function_record, UEStringType method_name)
                -> MethodSignature;

      public:
        auto static get_type_name(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool check_valid = false) -> UEStringType;
        auto static get_method_name(const PDB::CodeView::TPI::FieldList* method_record) -> UEStringType;
        auto static get_leaf_name(const char* data, PDB::CodeView::TPI::TypeRecordKind kind) -> UEStringType;

        auto static clean_name(UEStringType name) -> UEStringType;

        auto static is_virtual(PDB::CodeView::TPI::MemberAttributes attributes) -> bool;

      private:
        auto setup_symbol_loader() -> void;
    };
} // namespace RC::UVTD

#endif