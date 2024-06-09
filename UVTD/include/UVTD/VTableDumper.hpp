#ifndef UNREALVTABLEDUMPER_VTABLEDUMPER_HPP
#define UNREALVTABLEDUMPER_VTABLEDUMPER_HPP

#include <map>
#include <unordered_map>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>
#include <UVTD/TypeContainer.hpp>

#include <PDB_TPIStream.h>

namespace RC::UVTD
{
    class VTableDumper
    {
      private:
        Symbols symbols;
        TypeContainer type_container;

        bool are_symbols_cached{};

      public:
        VTableDumper() = delete;
        explicit VTableDumper(Symbols symbols) : symbols(std::move(symbols))
        {
        }

      public:
        auto generate_code() -> void;
        auto generate_files() -> void;

      public:
        auto get_type_container() const -> const TypeContainer&
        {
            return type_container;
        }

      private:
        auto process_class(const PDB::TPIStream& tpi_stream,
                           const PDB::CodeView::TPI::Record* class_record,
                           const UEStringType& class_name,
                           const SymbolNameInfo& name_info) -> void;

        auto process_method_overload_list(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* method_record, Class& class_entry) -> void;
        auto process_onemethod(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* onemethod_record, Class& class_entry) -> void;

      private:
        auto dump_vtable_for_symbol(std::unordered_map<UEStringType, SymbolNameInfo>& names) -> void;

      public:
        static auto output_cleanup() -> void;
    };
} // namespace RC::UVTD

#endif