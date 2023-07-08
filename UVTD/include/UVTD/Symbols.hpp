#ifndef UNREALVTABLEDUMPER_SYMBOLS_HPP
#define UNREALVTABLEDUMPER_SYMBOLS_HPP

#include <unordered_map>
#include <map>
#include <vector>
#include <format>

#include <File/File.hpp>

#include <PDB_RawFile.h>
#include <PDB_DBIStream.h>
#include <PDB_TPIStream.h>

namespace RC::UVTD
{
    enum class DumpMode { VTable, MemberVars, SolBindings };

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

    struct MemberVariable
    {
        File::StringType type;
        File::StringType name;
        int32_t offset;
    };

    struct FunctionParam
    {
        File::StringType type;

        auto to_string() const -> File::StringType
        {
            return std::format(STR("{}"), type);
        }
    };

    struct MethodSignature
    {
        File::StringType return_type;
        File::StringType name;
        std::vector<FunctionParam> params;
        bool const_qualifier;

        auto to_string() const -> File::StringType
        {
            File::StringType params_string{};

            for (size_t i = 0; i < params.size(); i++)
            {
                bool should_add_comma = i < params.size() - 1;
                params_string.append(std::format(STR("{}{}"), params[i].to_string(), should_add_comma ? STR(", ") : STR("")));
            }

            return std::format(STR("{} {}({}){};"), return_type, name, params_string, const_qualifier ? STR("const") : STR(""));
        }
    };

    struct MethodBody
    {
        File::StringType name;
        MethodSignature signature;
        uint32_t offset;
        bool is_overload;
    };

    struct Class
    {
        File::StringType class_name;
        File::StringType class_name_clean;
        std::map<uint32_t, MethodBody> functions;
        // Key: Variable name
        std::map<File::StringType, MemberVariable> variables;
        uint32_t last_virtual_offset{};
        ValidForVTable valid_for_vtable{ ValidForVTable::No };
        ValidForMemberVars valid_for_member_vars{ ValidForMemberVars::No };
    };

    class Symbols {
    public:
        struct MemberVariable
        {
            File::StringType type;
            int32_t offset;
        };

        struct EnumEntry
        {
            File::StringType name;
            File::StringType name_clean;
            std::map<File::StringType, MemberVariable> variables;
        };		
        
        struct Class
        {
            File::StringType class_name;
            File::StringType class_name_clean;
            std::map<uint32_t, MethodBody> functions;
            // Key: Variable name
            std::map<File::StringType, MemberVariable> variables;
            uint32_t last_virtual_offset;
            ValidForVTable valid_for_vtable{ ValidForVTable::No };
            ValidForMemberVars valid_for_member_vars{ ValidForMemberVars::No };
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
        auto get_or_create_enum_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean) -> EnumEntry&;
        auto get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&;

        auto generate_method_signature(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::Record* function_record, File::StringType method_name) -> MethodSignature;

    public:
        auto static get_type_name(const PDB::TPIStream& tpi_stream, uint32_t record_index) -> File::StringType;
        auto static get_method_name(const PDB::CodeView::TPI::FieldList* method_record) -> File::StringType;
        auto static get_leaf_name(const char* data, PDB::CodeView::TPI::TypeRecordKind kind) -> File::StringType;

        auto static clean_name(const File::StringType& name) -> File::StringType;

        auto static is_virtual(PDB::CodeView::TPI::MemberAttributes attributes) -> bool;

    private:
        auto setup_symbol_loader() -> void;
    };
}

#endif