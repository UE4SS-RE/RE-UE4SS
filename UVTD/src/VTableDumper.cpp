#define NOMINMAX

#include <filesystem>
#include <format>
#include <iostream>
#include <unordered_set>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <UVTD/ConfigUtil.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/Symbols.hpp>
#include <UVTD/VTableDumper.hpp>

#include <Windows.h>

#include <PDB_CoalescedMSFStream.h>
#include <PDB_GlobalSymbolStream.h>
#include <PDB_IPIStream.h>
#include <PDB_ModuleInfoStream.h>
#include <PDB_ModuleSymbolStream.h>
#include <PDB_PublicSymbolStream.h>
#include <PDB_TPIStream.h>

namespace RC::UVTD
{

    File::StringType sanitize_type_for_identifier(File::StringType type)
    {
        File::StringType result = type;

        // Handle volatile prefix
        if (result.starts_with(STR("volatile ")))
        {
            result = result.substr(9);
            result = STR("V_") + result;
        }

        // Handle const volatile prefix
        if (result.starts_with(STR("const volatile ")) || result.starts_with(STR("volatile const ")))
        {
            result = result.substr(15);
            result = STR("CV_") + result;
        }

        // Handle const prefix
        if (result.starts_with(STR("const ")))
        {
            result = result.substr(6);
            result = STR("C_") + result;
        }
        
        // Handle references and pointers
        while (!result.empty() && (result.back() == '&' || result.back() == '*'))
        {
            if (result.back() == '&')
            {
                result.pop_back();
                result = STR("Ref_") + result;
            }
            else if (result.back() == '*')
            {
                result.pop_back();
                result = STR("Ptr_") + result;
            }
        }

        // Replace characters that are invalid in identifiers
        std::replace(result.begin(), result.end(), '<', '_');
        std::replace(result.begin(), result.end(), '>', '_');
        std::replace(result.begin(), result.end(), ',', '_');
        std::replace(result.begin(), result.end(), ' ', '_');
        std::replace(result.begin(), result.end(), ':', '_');
        std::replace(result.begin(), result.end(), '[', '_');
        std::replace(result.begin(), result.end(), ']', '_');
        std::replace(result.begin(), result.end(), '(', '_');
        std::replace(result.begin(), result.end(), ')', '_');

        // Remove any multiple underscores (not just doubles)
        File::StringType cleaned;
        bool last_was_underscore = false;
        for (auto c : result)
        {
            if (c == '_')
            {
                if (!last_was_underscore)
                {
                    cleaned += c;
                    last_was_underscore = true;
                }
            }
            else
            {
                cleaned += c;
                last_was_underscore = false;
            }
        }

        return cleaned;
    }

    File::StringType generate_mangled_suffix(const MethodSignature& signature)
    {
        File::StringType suffix = STR("");
        auto quals = signature.qualifiers;
        // Add qualifiers if any exist
        bool has_quals = quals.is_const || quals.is_volatile || quals.is_lvalue_ref || quals.is_rvalue_ref;

        if (has_quals)
        {
            suffix += STR("_");

            // cv-qualifiers come before ref-qualifiers
            if (quals.is_const)
            {
                suffix += STR("C"); // const
            }
            if (quals.is_volatile)
            {
                suffix += STR("V"); // volatile
            }

            // Ref-qualifiers
            if (quals.is_lvalue_ref)
            {
                suffix += STR("L"); // & (lvalue ref)
            }
            else if (quals.is_rvalue_ref)
            {
                suffix += STR("R"); // && (rvalue ref)
            }
        }

        auto params = signature.params;
        // Add parameters if any
        if (!params.empty())
        {
            suffix += STR("__");
            for (size_t i = 0; i < params.size(); ++i)
            {
                if (i > 0) suffix += STR("__");
                suffix += sanitize_type_for_identifier(params[i].type);
            }
        }

        return suffix;
    }
    
    auto VTableDumper::process_class(const PDB::TPIStream& tpi_stream,
                                     const PDB::CodeView::TPI::Record* class_record,
                                     const File::StringType& name,
                                     const SymbolNameInfo& name_info) -> void
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

    auto VTableDumper::process_method_overload_list(const PDB::TPIStream& tpi_stream,
                                                    const PDB::CodeView::TPI::FieldList* method_record,
                                                    Class& class_entry) -> void
    {
        auto list = tpi_stream.GetTypeRecord(method_record->data.LF_METHOD.mList);

        File::StringType base_method_name = Symbols::get_method_name(method_record);
        File::StringType base_method_name_clean = Symbols::clean_name(base_method_name);

        size_t next_offset = 0;

        for (size_t i = 0; i < method_record->data.LF_METHOD.count; i++)
        {
            PDB::CodeView::TPI::Record::Data* overload_record = 
                (PDB::CodeView::TPI::Record::Data*)((uint8_t*)&list->data.LF_METHODLIST.mList + next_offset);

            next_offset += sizeof(PDB::CodeView::TPI::Record::Data::METHOD);
            if (Symbols::is_virtual(overload_record->METHOD.attributes))
            {
                next_offset += sizeof(uint32_t);
            }

            if (!Symbols::is_virtual(overload_record->METHOD.attributes)) continue;
        
            auto function_record = tpi_stream.GetTypeRecord(overload_record->METHOD.index);
            if (!function_record || function_record->header.kind != PDB::CodeView::TPI::TypeRecordKind::LF_MFUNCTION) continue;

            int32_t vtable_offset = overload_record->METHOD.vbaseoff[0];

            // Generate signature for mangling
            MethodSignature signature = symbols.generate_method_signature(tpi_stream, function_record, base_method_name);
        
            // Generate mangled suffix based on parameters
            File::StringType mangled_suffix = generate_mangled_suffix(signature);
            File::StringType mangled_name = base_method_name_clean + mangled_suffix;

            auto& function = class_entry.functions[vtable_offset];
            function.name = mangled_name;
            function.signature = signature;
            function.offset = vtable_offset;
            function.is_overload = true;
        }
    }

    auto VTableDumper::process_onemethod(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* method_record, Class& class_entry) -> void
    {
        const auto is_virtual = method_record->data.LF_ONEMETHOD.attributes.mprop == (uint16_t)PDB::CodeView::TPI::MethodProperty::Intro ||
                                method_record->data.LF_ONEMETHOD.attributes.mprop == (uint16_t)PDB::CodeView::TPI::MethodProperty::PureIntro;
        if (!is_virtual) return;

        File::StringType method_name = Symbols::get_method_name(method_record);
        int32_t vtable_offset = method_record->data.LF_ONEMETHOD.vbaseoff[0];
        auto function_record = tpi_stream.GetTypeRecord(method_record->data.LF_ONEMETHOD.index);

        Output::send(STR("  method {} offset {}\n"), method_name, vtable_offset);

        File::StringType method_name_clean = Symbols::clean_name(method_name);

        auto& function = class_entry.functions[vtable_offset];
        function.name = method_name_clean;
        function.signature = symbols.generate_method_signature(tpi_stream, function_record, method_name);
        function.offset = vtable_offset;
        function.is_overload = false;
    }

    auto VTableDumper::dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
    {
        Output::send(STR("Dumping {} struct symbols for {}\n"), names.size(), symbols.pdb_file_path.filename().stem().wstring());

        const PDB::TPIStream tpi_stream = PDB::CreateTPIStream(symbols.pdb_file);

        for (const PDB::CodeView::TPI::Record* type_record : tpi_stream.GetTypeRecords())
        {
            if (type_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_CLASS ||
                type_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE)
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

        // Use config utility instead of hardcoded list
        for (const auto& object_item : ConfigUtil::GetObjectItems())
        {
            if (object_item.valid_for_vtable != ValidForVTable::Yes) continue;

            vtable_names.emplace(object_item.name, SymbolNameInfo{object_item.valid_for_vtable, object_item.valid_for_member_vars});
        }

        dump_vtable_for_symbol(vtable_names);
    }

    auto VTableDumper::generate_files() -> void
    {
        const auto& pdb_info = symbols.get_pdb_name_info();
        // Use base_version for file naming (e.g., "4_27" from "4_27_CasePreserving")
        const auto& pdb_name = pdb_info.base_version;
        // Include suffix in template filename if present
        auto pdb_full_name = pdb_info.full_name;
        // Construct filename prefix: base_version + suffix_string (e.g., "4_27" or "4_27_CasePreserving")
        auto pdb_filename_prefix = pdb_name + pdb_info.get_suffix_string();

        auto default_template_file = std::filesystem::path{STR("VTableLayout.ini")};

        Output::send(STR("Generating file '{}'\n"), default_template_file.wstring());

        auto template_file = std::format(STR("VTableLayout_{}_Template.ini"), pdb_full_name);

        Output::send(STR("Generating file '{}'\n"), template_file);

        Output::Targets<Output::NewFileDevice> ini_dumper;
        auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
        ini_file_device.set_file_name_and_path(vtable_templates_output_path / template_file);
        ini_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{string};
        });

        // Iterate through object_items first to preserve order
        for (const auto& object_item : ConfigUtil::GetObjectItems())
        {
            const auto& class_name = object_item.name;

            // Find the corresponding class entry
            auto class_it = std::find_if(
                    type_container.get_class_entries().begin(),
                    type_container.get_class_entries().end(),
                    [&class_name](const auto& entry) {
                        return entry.first == class_name || entry.second.class_name == class_name;
                    }
                    );

            // Skip if no class entry or skipping based on config
            if (class_it == type_container.get_class_entries().end() ||
                object_item.valid_for_vtable != ValidForVTable::Yes)
            {
                continue;
            }

            const auto& class_entry = class_it->second;

            // Skip if no functions
            if (class_entry.functions.empty())
            {
                continue;
            }

            // Create function body file (uses filename prefix for proper suffix handling)
            auto function_body_file = vtable_gen_function_bodies_path /
                                      std::format(STR("{}_VTableOffsets_{}_FunctionBody.cpp"), pdb_filename_prefix, class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), function_body_file.wstring());

            Output::Targets<Output::NewFileDevice> function_body_dumper;
            auto& function_body_file_device = function_body_dumper.get_device<Output::NewFileDevice>();
            function_body_file_device.set_file_name_and_path(function_body_file);
            function_body_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            // Generate VTable offset entries with object_item order
            ini_dumper.send(STR("[{}]\n"), class_entry.class_name);

            for (const auto& [function_index, function_entry] : class_entry.functions)
            {
                auto local_class_name = class_entry.class_name;
                if (auto pos = local_class_name.find(STR("Property")); pos != local_class_name.npos)
                {
                    local_class_name.replace(0, 1, STR("F"));
                }

                function_body_dumper.send(STR("if (auto it = {}::VTableLayoutMap.find(STR(\"{}\")); it == {}::VTableLayoutMap.end())\n"),
                                          local_class_name,
                                          function_entry.name,
                                          local_class_name);
                function_body_dumper.send(STR("{\n"));
                function_body_dumper.send(
                        STR("    {}::VTableLayoutMap.emplace(STR(\"{}\"), 0x{:X});\n"),
                        local_class_name,
                        function_entry.name,
                        function_entry.offset);
                function_body_dumper.send(STR("}\n\n"));

                // Handle INI output for function entries
                ini_dumper.send(STR("; {}\n"), function_entry.signature.to_string());
                ini_dumper.send(STR("{}\n"), function_entry.name);
            }

            ini_dumper.send(STR("\n"));
        }
    }

    auto VTableDumper::output_cleanup() -> void
    {
        // Using vtable_gen_function_bodies_path from Helpers.hpp
        if (std::filesystem::exists(vtable_gen_function_bodies_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(vtable_gen_function_bodies_path))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != STR(".cpp") && item.path().extension() != STR(".hpp"))
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }
    }
} // namespace RC::UVTD