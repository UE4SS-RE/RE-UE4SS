#ifndef UNREALVTABLEDUMPER_MEMBERVARS_HPP
#define UNREALVTABLEDUMPER_MEMBERVARS_HPP

#include <unordered_map>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>
#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    class MemberVarsDumper
    {
      private:
        Symbols symbols;
        TypeContainer type_container;

      public:
        MemberVarsDumper() = delete;
        explicit MemberVarsDumper(Symbols symbols) : symbols(std::move(symbols))
        {
        }

      private:
        auto process_class(const PDB::TPIStream& tpi_stream,
                           const PDB::CodeView::TPI::Record* class_record,
                           const UEStringType& class_name,
                           const SymbolNameInfo& name_info) -> void;
        auto process_member(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* field_record, Class& class_entry) -> void;

      private:
        auto dump_member_variable_layouts(std::unordered_map<UEStringType, SymbolNameInfo>& names) -> void;

      public:
        auto generate_code() -> void;
        auto generate_files() -> void;

      public:
        auto get_type_container() const -> const TypeContainer&
        {
            return type_container;
        }

      public:
        static auto output_cleanup() -> void;
    };
} // namespace RC::UVTD

#endif