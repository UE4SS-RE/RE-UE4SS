#pragma once

#include <format>
#include <map>
#include <unordered_map>
#include <vector>

#include <File/File.hpp>

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
        No = 0,
        Yes = 1
    };
    enum class ValidForMemberVars
    {
        No = 0,
        Yes = 1
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
        File::StringType type;
        File::StringType name;
        int32_t offset;
        uint32_t type_index{};
        uint32_t size{};
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
        std::vector<MemberVariable> variables;
        uint32_t total_size{};  // Track total class size
        uint32_t last_virtual_offset{};
        ValidForVTable valid_for_vtable{ValidForVTable::No};
        ValidForMemberVars valid_for_member_vars{ValidForMemberVars::No};
    };

    class Symbols
    {
    public:
        struct EnumEntry
        {
            File::StringType name;
            File::StringType name_clean;
            std::map<File::StringType, MemberVariable> variables;
        };

        enum class ClassInheritanceModel
        {
            Single,
            Multiple,
            Virtual
        };

      public:
        std::filesystem::path pdb_file_path;
        File::Handle pdb_file_handle;
        std::span<uint8_t> pdb_file_map;

        PDB::RawFile pdb_file;
        PDB::DBIStream dbi_stream;
        bool is_425_plus;

    private:
        PDB::CodeView::DBI::CPUType m_machine_type{PDB::CodeView::DBI::CPUType::X64};

    public:
        std::unordered_map<File::StringType, EnumEntry> enum_entries;
        std::unordered_map<File::StringType, Class> class_entries;
        static inline std::unordered_map<uint32_t, uint32_t> type_size_cache;


        Symbols() = delete;

        explicit Symbols(std::filesystem::path pdb_file_path);

        Symbols(const Symbols& other);

        Symbols& operator=(const Symbols& other);

      public:
        auto get_or_create_enum_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean) -> EnumEntry&;

        auto generate_method_signature(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::Record* function_record, File::StringType method_name)
                -> MethodSignature;

      public:
        auto static get_type_name(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool check_valid = false, bool is_64bit = true) -> File::StringType;
        auto static read_numeric(const uint8_t*& data) -> uint64_t;
        auto static get_numeric_leaf_size(const uint8_t* data) -> uint32_t;
        auto static get_field_record_size(const PDB::CodeView::TPI::FieldList* field) -> uint32_t;
        auto static get_type_size_impl(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool is_64bit = true) -> uint32_t;
        auto static get_type_size(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool is_64bit = true) -> uint32_t;
        auto static get_method_name(const PDB::CodeView::TPI::FieldList* method_record) -> File::StringType;
        auto static get_class_inheritance_model(const PDB::TPIStream& tpi_stream, uint32_t class_type_index) -> ClassInheritanceModel;
        // Existing method for backward compatibility
        auto static get_leaf_name(const char* data, PDB::CodeView::TPI::TypeRecordKind kind) -> File::StringType;

        // New overload that takes class name for context-aware name mapping
        auto static get_leaf_name(const File::StringType& class_name, const char* data, PDB::CodeView::TPI::TypeRecordKind kind) -> File::StringType;

        auto static clean_name(File::StringType name) -> File::StringType;

        auto static is_virtual(PDB::CodeView::TPI::MemberAttributes attributes) -> bool;
        auto is_x64() const -> bool;
        auto is_x86() const -> bool;

      private:
        auto setup_symbol_loader() -> void;
    };
} // namespace RC::UVTD