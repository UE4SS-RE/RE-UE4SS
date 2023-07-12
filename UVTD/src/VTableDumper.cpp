#define NOMINMAX

#include <iostream>
#include <filesystem>
#include <format>
#include <unordered_set>

#include <UVTD/VTableDumper.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/Symbols.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

#include <Windows.h>

#include <PDB_GlobalSymbolStream.h>
#include <PDB_CoalescedMSFStream.h>
#include <PDB_PublicSymbolStream.h>
#include <PDB_ModuleInfoStream.h>
#include <PDB_ModuleSymbolStream.h>
#include <PDB_TPIStream.h>
#include <PDB_IPIStream.h>

namespace RC::UVTD 
{
    auto VTableDumper::process_class(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::Record* class_record, const File::StringType& name, const SymbolNameInfo& name_info) -> void
    {
        auto changed = change_prefix(name, symbols.is_425_plus);
        if (!changed.has_value()) return;

        File::StringType class_name = *changed;
        File::StringType class_name_clean = Symbols::clean_name(class_name);

        auto& class_entry = type_container.get_or_create_class_entry(class_name, class_name_clean, name_info);

        auto fields = tpi_stream.GetTypeRecord(class_record->data.LF_CLASS.field);

        auto list_size = fields->header.size - sizeof(uint16_t);
        for (size_t i = 0; i < list_size; i++)
        {
            auto field_record = (PDB::CodeView::TPI::FieldList*)((uint8_t*)&fields->data.LF_FIELD.list + i);

            switch (field_record->kind)
            {
                case PDB::CodeView::TPI::TypeRecordKind::LF_METHOD:
                    process_method_overload_list(tpi_stream, field_record, class_entry);
                    break;
                case PDB::CodeView::TPI::TypeRecordKind::LF_ONEMETHOD:
                    process_onemethod(tpi_stream, field_record, class_entry);
                    break;
            }
        }
    }

    struct MethodListEntry
    {
        uint32_t index;
        uint32_t vftable_offset;
    };

    auto VTableDumper::process_method_overload_list(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* method_record, Class& class_entry) -> void
    {
        auto list = tpi_stream.GetTypeRecord(method_record->data.LF_METHOD.mList);

        File::StringType method_name = Symbols::get_method_name(method_record);
        File::StringType method_name_clean = Symbols::clean_name(method_name);

        // this is required because METHOD struct size is not constant :)
        size_t next_offset = 0;
        uint32_t overload_index = 0;

        for (size_t i = 0; i < method_record->data.LF_METHOD.count; i++)
        {
            PDB::CodeView::TPI::Record::Data* overload_record = (PDB::CodeView::TPI::Record::Data*)((uint8_t*)&list->data.LF_METHODLIST.mList + next_offset);

            next_offset += sizeof(PDB::CodeView::TPI::Record::Data::METHOD);
            if (Symbols::is_virtual(overload_record->METHOD.attributes))
            {
                next_offset += sizeof(uint32_t);
            }

            int32_t vtable_offset = overload_record->METHOD.vbaseoff[0];
            auto function_record = tpi_stream.GetTypeRecord(overload_record->METHOD.index);
            if (!Symbols::is_virtual(overload_record->METHOD.attributes)) continue;
            if (!function_record || function_record->header.kind != PDB::CodeView::TPI::TypeRecordKind::LF_MFUNCTION) continue;

            File::StringType overload_name = method_name_clean;
            if (overload_index != 0)
            {
                overload_name += std::format(STR("_{}"), overload_index);
            }
            overload_index++;

            auto& function = class_entry.functions[vtable_offset];
            function.name = overload_name;
            function.signature = symbols.generate_method_signature(tpi_stream, function_record, overload_name);
            function.offset = vtable_offset;
            function.is_overload = true;
        }
    }

    auto VTableDumper::process_onemethod(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* method_record, Class& class_entry) -> void
    {
        static std::unordered_map<File::StringType, std::unordered_map<File::StringType, uint32_t>> functions_already_dumped{};

        const auto is_virtual = method_record->data.LF_ONEMETHOD.attributes.mprop == (uint16_t)PDB::CodeView::TPI::MethodProperty::Intro ||
            method_record->data.LF_ONEMETHOD.attributes.mprop == (uint16_t)PDB::CodeView::TPI::MethodProperty::PureIntro;
        if (!is_virtual) return;

        File::StringType method_name = Symbols::get_method_name(method_record);
        int32_t vtable_offset = method_record->data.LF_ONEMETHOD.vbaseoff[0];
        auto function_record = tpi_stream.GetTypeRecord(method_record->data.LF_ONEMETHOD.index);

        bool is_overload{};
        
        if (auto it = functions_already_dumped.find(class_entry.class_name); it != functions_already_dumped.end())
        {
            if (auto it2 = it->second.find(method_name); it2 != it->second.end())
            {
                method_name.append(std::format(STR("_{}"), ++it2->second));
                is_overload = true;
            }
        }

        Output::send(STR("  method {} offset {}\n"), method_name, vtable_offset);

        File::StringType method_name_clean = Symbols::clean_name(method_name);

        auto& function = class_entry.functions[vtable_offset];
        function.name = method_name_clean;
        function.signature = symbols.generate_method_signature(tpi_stream, function_record, method_name);
        function.offset = vtable_offset;
        function.is_overload = is_overload;
        functions_already_dumped.emplace(method_name, 1);
    }

    auto VTableDumper::dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
    {
        Output::send(STR("Dumping {} struct symbols for {}\n"), names.size(), symbols.pdb_file_path.filename().stem().wstring());

        const PDB::TPIStream tpi_stream = PDB::CreateTPIStream(symbols.pdb_file);

        for (const PDB::CodeView::TPI::Record* type_record : tpi_stream.GetTypeRecords())
        {
            if (type_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_CLASS || type_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE)
            {
                if (type_record->data.LF_CLASS.property.fwdref) continue;

                const File::StringType class_name = Symbols::get_leaf_name(type_record->data.LF_CLASS.data, type_record->data.LF_CLASS.lfEasy.kind);
                if (!names.contains(class_name)) continue;

                const auto name_info = names.find(class_name);
                if (name_info == names.end()) continue;

                process_class(tpi_stream, type_record, class_name, name_info->second);
            }
        }
        return;
    }

    auto VTableDumper::generate_code() -> void
    {
        std::unordered_map<File::StringType, SymbolNameInfo> vtable_names;
        for (const auto& object_item : s_object_items)
        {
            if (object_item.valid_for_vtable != ValidForVTable::Yes) continue;
            
            vtable_names.emplace(object_item.name, SymbolNameInfo{ object_item.valid_for_vtable, object_item.valid_for_member_vars });
        }

        dump_vtable_for_symbol(vtable_names);
    }

    auto VTableDumper::generate_files() -> void
    {
        File::StringType pdb_name = symbols.pdb_file_path.filename().stem();

        for (const auto& [class_name, class_entry] : type_container.get_class_entries())
        {
            Output::send(STR("Generating file '{}_VTableOffsets_{}_FunctionBody.cpp'\n"), pdb_name, class_entry.class_name_clean);
            Output::Targets<Output::NewFileDevice> function_body_dumper;
            auto& function_body_file_device = function_body_dumper.get_device<Output::NewFileDevice>();
            function_body_file_device.set_file_name_and_path(vtable_gen_output_function_bodies_path / std::format(STR("{}_VTableOffsets_{}_FunctionBody.cpp"), pdb_name, class_name));
            function_body_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{ string };
            });

            for (const auto& [function_index, function_entry] : class_entry.functions)
            {
                auto local_class_name = class_entry.class_name;
                if (auto pos = local_class_name.find(STR("Property")); pos != local_class_name.npos)
                {
                    local_class_name.replace(0, 1, STR("F"));
                }

                function_body_dumper.send(STR("if (auto it = {}::VTableLayoutMap.find(STR(\"{}\")); it == {}::VTableLayoutMap.end())\n"), local_class_name, function_entry.name, local_class_name);
                function_body_dumper.send(STR("{\n"));
                function_body_dumper.send(STR("    {}::VTableLayoutMap.emplace(STR(\"{}\"), 0x{:X});\n"), local_class_name, function_entry.name, function_entry.offset);
                function_body_dumper.send(STR("}\n\n"));
            }
        }

        auto template_file = std::format(STR("VTableLayout_{}_Template.ini"), pdb_name);
        Output::send(STR("Generating file '{}'\n"), template_file);
        Output::Targets<Output::NewFileDevice> ini_dumper;
        auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
        ini_file_device.set_file_name_and_path(vtable_templates_output_path / template_file);
        ini_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{ string };
        });

        for (const auto& [class_name, class_entry] : type_container.get_class_entries())
        {
            ini_dumper.send(STR("[{}]\n"), class_entry.class_name);

            for (const auto& [function_index, function_entry] : class_entry.functions)
            {
                if (function_entry.is_overload)
                {
                    ini_dumper.send(STR("; {}\n"), function_entry.signature.to_string());
                }
                ini_dumper.send(STR("{}\n"), function_entry.name);
            }

            ini_dumper.send(STR("\n"));
        }
    }

    auto VTableDumper::output_cleanup() -> void
    {
        if (std::filesystem::exists(vtable_gen_output_include_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_include_path))
            {
                if (item.is_directory()) { continue; }
                if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp")) { continue; }

                File::delete_file(item.path());
            }
        }

        if (std::filesystem::exists(vtable_gen_output_function_bodies_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_function_bodies_path))
            {
                if (item.is_directory()) { continue; }
                if (item.path().extension() != STR(".cpp")) { continue; }

                File::delete_file(item.path());
            }
        }

        if (std::filesystem::exists(vtable_gen_output_src_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_src_path))
            {
                if (item.is_directory()) { continue; }
                if (item.path().extension() != STR(".cpp")) { continue; }

                File::delete_file(item.path());
            }
        }
    }
}