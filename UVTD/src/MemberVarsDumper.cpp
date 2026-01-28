#include <format>
#include <unordered_map>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/ConfigUtil.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/MemberVarsDumper.hpp>

namespace RC::UVTD
{
    auto MemberVarsDumper::process_class(const PDB::TPIStream& tpi_stream,
                                         const PDB::CodeView::TPI::Record* class_record,
                                         const File::StringType& name,
                                         const SymbolNameInfo& name_info) -> void
    {
        auto changed = change_prefix(name, symbols.is_425_plus);
        if (!changed.has_value()) return;

        File::StringType class_name = *changed;
        File::StringType class_name_clean = Symbols::clean_name(class_name);
        auto& class_entry = type_container.get_or_create_class_entry(class_name, class_name_clean, name_info);

        // Get the class size directly from the PDB numeric leaf
        const uint8_t* data = reinterpret_cast<const uint8_t*>(class_record->data.LF_CLASS.data);
        const uint8_t* data_copy = data;
        class_entry.total_size = static_cast<uint32_t>(Symbols::read_numeric(data_copy));

        auto fields = tpi_stream.GetTypeRecord(class_record->data.LF_CLASS.field);

        auto list_size = fields->header.size - sizeof(uint16_t);
        for (size_t i = 0; i < list_size; i++)
        {
            auto field_record = (PDB::CodeView::TPI::FieldList*)((uint8_t*)&fields->data.LF_FIELD.list + i);

            if (field_record->kind == PDB::CodeView::TPI::TypeRecordKind::LF_MEMBER)
            {
                process_member(tpi_stream, field_record, class_entry);
            }
        }
    }

    auto MemberVarsDumper::process_member(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* field_record, Class& class_entry) -> void
    {
        // Get the original member name without renaming - this is important so we capture the true variable name
        File::StringType member_name = Symbols::get_leaf_name(field_record->data.LF_STMEMBER.name, field_record->data.LF_MEMBER.lfEasy.kind);

        auto changed = change_prefix(Symbols::get_type_name(tpi_stream, field_record->data.LF_MEMBER.index, symbols.is_x64()), symbols.is_425_plus);
        if (!changed.has_value()) return;

        File::StringType type_name = *changed;

        // Check if we should completely exclude this type based on configuration
        if (ConfigUtil::ShouldFilterType(type_name, TypeFilterCategory::CompleteExclusion))
        {
            return;
        }

        // Check if this variable already exists (to avoid duplicates)
        auto existing = std::find_if(class_entry.variables.begin(),
                                     class_entry.variables.end(),
                                     [&member_name](const MemberVariable& var) {
                                         return var.name == member_name;
                                     });

        // Extract bitfield information if applicable
        auto bitfield_info = Symbols::get_bitfield_info(tpi_stream, field_record->data.LF_MEMBER.index);

        if (existing != class_entry.variables.end())
        {
            // Update existing variable
            existing->type = type_name;
            existing->offset = *(uint16_t*)field_record->data.LF_MEMBER.offset;
            existing->is_bitfield = bitfield_info.is_bitfield;
            existing->bit_position = bitfield_info.bit_position;
            existing->bit_length = bitfield_info.bit_length;
        }
        else
        {
            // Calculate the size of this member
            uint32_t member_size = Symbols::get_type_size(tpi_stream, field_record->data.LF_MEMBER.index, symbols.is_x64());

            MemberVariable variable;
            variable.type = type_name;
            variable.name = member_name;
            variable.offset = *(uint16_t*)field_record->data.LF_MEMBER.offset;
            variable.type_index = field_record->data.LF_MEMBER.index;
            variable.size = member_size;
            variable.is_bitfield = bitfield_info.is_bitfield;
            variable.bit_position = bitfield_info.bit_position;
            variable.bit_length = bitfield_info.bit_length;

            class_entry.variables.push_back(variable);
        }
    }

    auto MemberVarsDumper::dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
    {
        Output::send(STR("Dumping {} symbols for {}\n"), names.size(), symbols.pdb_file_path.filename().stem().wstring());

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

    auto MemberVarsDumper::generate_code() -> void
    {
        std::unordered_map<File::StringType, SymbolNameInfo> member_vars_names;

        // Use config utility instead of hardcoded list
        for (const ObjectItem& item : ConfigUtil::GetObjectItems())
        {
            if (item.valid_for_member_vars != ValidForMemberVars::Yes) continue;
            member_vars_names.emplace(item.name, SymbolNameInfo{item.valid_for_vtable, item.valid_for_member_vars});
        }

        dump_member_variable_layouts(member_vars_names);
    }

    auto MemberVarsDumper::generate_files() -> void
    {
        const auto& pdb_info = symbols.get_pdb_name_info();
        // Use full name for templates (includes suffixes like CasePreserving)
        const auto& pdb_full_name = pdb_info.full_name;
        // Use base version for function body files (these match the base version)
        const auto& pdb_base_version = pdb_info.base_version;
        // Use version_no_separator for class names (e.g., "427")
        const auto& pdb_name_no_underscore = pdb_info.version_no_separator;

        auto template_file = std::format(STR("MemberVariableLayout_{}_Template.ini"), pdb_full_name);

        Output::send(STR("Generating file '{}'\n"), template_file);

        Output::Targets<Output::NewFileDevice> ini_dumper;
        auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
        ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / template_file);
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
                object_item.valid_for_member_vars != ValidForMemberVars::Yes)
            {
                continue;
            }

            const auto& class_entry = class_it->second;

            if (class_entry.variables.empty())
            {
                continue;
            }

            // Use full name for file (includes suffix like CasePreserving)
            auto default_setter_src_file = member_variable_layouts_gen_function_bodies_path /
                                           std::format(STR("{}_MemberVariableLayout_DefaultSetter_{}.cpp"), pdb_full_name, class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), default_setter_src_file.wstring());

            Output::Targets<Output::NewFileDevice> default_setter_src_dumper;
            auto& default_setter_src_file_device = default_setter_src_dumper.get_device<Output::NewFileDevice>();
            default_setter_src_file_device.set_file_name_and_path(default_setter_src_file);
            default_setter_src_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            ini_dumper.send(STR("[{}]\n"), class_entry.class_name);
            // Output total size as a comment at the top
            ini_dumper.send(STR("; Total Size: 0x{:X}\n"), class_entry.total_size);

            // Track variables we've already processed to avoid duplicates
            std::unordered_set<File::StringType> processed_variables;

            // Iterate through sorted variables with formatted type info
            for (size_t i = 0; i < class_entry.variables.size(); ++i)
            {
                const auto& variable = class_entry.variables[i];

                // Calculate padding to next member
                uint32_t padding = 0;
                if (i + 1 < class_entry.variables.size())
                {
                    uint32_t current_end = variable.offset + variable.size;
                    uint32_t next_start = class_entry.variables[i + 1].offset;
                    if (next_start > current_end)
                    {
                        padding = next_start - current_end;
                    }
                }

                // Output type info line with formatted columns
                // Format: Type (padded to 35 chars) | Size | Padding (if present)
                if (padding > 0)
                {
                    ini_dumper.send(fmt::format(STR("; {:<35} Size: 0x{:04X}  Padding: 0x{:X}\n"),
                                                variable.type,
                                                variable.size,
                                                padding));
                }
                else
                {
                    ini_dumper.send(fmt::format(STR("; {:<35} Size: 0x{:04X}\n"),
                                                variable.type,
                                                variable.size));
                }

                // Output the actual member assignment
                // For bitfields, include bit position, length, and storage size: offset:bit_pos:bit_len:storage_size
                if (variable.is_bitfield)
                {
                    ini_dumper.send(fmt::format(STR("{} = 0x{:X}:{}:{}:{}\n"), variable.name, variable.offset, variable.bit_position, variable.bit_length, variable.size));
                }
                else
                {
                    ini_dumper.send(fmt::format(STR("{} = 0x{:X}\n"), variable.name, variable.offset));
                }

                // Check if this is a name that should be renamed for code generation
                auto rename_info = ConfigUtil::GetMemberRenameInfo(class_entry.class_name, variable.name);
                File::StringType final_variable_name = variable.name;
                if (rename_info.has_value())
                {
                    final_variable_name = rename_info->mapped_name;
                }

                // But for code generation, use the internal variable name
                File::StringType final_class_name = class_entry.class_name;
                unify_uobject_array_if_needed(final_class_name);

                // Skip if we've already processed this variable to avoid duplicates
                if (processed_variables.find(final_variable_name) != processed_variables.end())
                {
                    continue;
                }
                processed_variables.insert(final_variable_name);

                // Generate the default setter code
                default_setter_src_dumper.send(STR("if (auto it = {}::MemberOffsets.find(STR(\"{}\")); it == {}::MemberOffsets.end())\n"),
                                               final_class_name,
                                               final_variable_name,
                                               final_class_name);
                default_setter_src_dumper.send(STR("{\n"));
                default_setter_src_dumper.send(
                        STR("    {}::MemberOffsets.emplace(STR(\"{}\"), 0x{:X});\n"),
                        final_class_name,
                        final_variable_name,
                        variable.offset);
                // For bitfields, also populate BitfieldInfos with bit position, length, and storage size
                if (variable.is_bitfield)
                {
                    default_setter_src_dumper.send(
                            STR("    {}::BitfieldInfos.emplace(STR(\"{}\"), BitfieldInfo{{{}, {}, {}}});\n"),
                            final_class_name,
                            final_variable_name,
                            variable.bit_position,
                            variable.bit_length,
                            variable.size);
                }
                default_setter_src_dumper.send(STR("}\n\n"));
            }

            // Output UEP_TotalSize at the end of the class section
            ini_dumper.send(STR("UEP_TotalSize = 0x{:X}\n"), class_entry.total_size);

            // Add UEP_TotalSize to the default setter
            {
                File::StringType total_size_class_name = class_entry.class_name;
                unify_uobject_array_if_needed(total_size_class_name);
                default_setter_src_dumper.send(STR("if (auto it = {}::MemberOffsets.find(STR(\"UEP_TotalSize\")); it == {}::MemberOffsets.end())\n"),
                                               total_size_class_name, total_size_class_name);
                default_setter_src_dumper.send(STR("{\n"));
                default_setter_src_dumper.send(STR("    {}::MemberOffsets.emplace(STR(\"UEP_TotalSize\"), 0x{:X});\n"),
                                               total_size_class_name, class_entry.total_size);
                default_setter_src_dumper.send(STR("}\n"));
            }

            ini_dumper.send(STR("\n"));
        }
    }

    auto MemberVarsDumper::output_cleanup() -> void
    {
        if (std::filesystem::exists(member_variable_layouts_gen_function_bodies_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_function_bodies_path))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp"))
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }
    }

} // namespace RC::UVTD