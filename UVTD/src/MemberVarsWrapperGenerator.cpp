#include <format>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/ConfigUtil.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/MemberVarsWrapperGenerator.hpp>

namespace RC::UVTD
{
    auto MemberVarsWrapperGenerator::generate_files() -> void
    {
        auto macro_setter_file = std::filesystem::path{STR("MacroSetter.hpp")};

        Output::send(STR("Generating file '{}'\n"), macro_setter_file.wstring());

        Output::Targets<Output::NewFileDevice> macro_setter_dumper;
        auto& macro_setter_file_device = macro_setter_dumper.get_device<Output::NewFileDevice>();
        macro_setter_file_device.set_file_name_and_path(macro_setter_file);
        macro_setter_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{string};
        });

        // Keep track of processed variables to avoid duplicates
        std::unordered_map<File::StringType, std::unordered_set<File::StringType>> processed_variables;

        for (const auto& [class_name, class_entry] : type_container.get_class_entries())
        {
            if (class_entry.variables.empty())
            {
                continue;
            }

            auto wrapper_header_file = member_variable_layouts_gen_output_include_path /
                                       std::format(STR("MemberVariableLayout_HeaderWrapper_{}.hpp"), class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());

            Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
            auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
            wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
            wrapper_header_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            auto wrapper_src_file =
                    member_variable_layouts_gen_output_include_path / std::format(STR("MemberVariableLayout_SrcWrapper_{}.hpp"), class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_src_file.wstring());

            Output::Targets<Output::NewFileDevice> wrapper_src_dumper;
            auto& wrapper_src_file_device = wrapper_src_dumper.get_device<Output::NewFileDevice>();
            wrapper_src_file_device.set_file_name_and_path(wrapper_src_file);
            wrapper_src_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            auto final_class_name = class_entry.class_name;
            unify_uobject_array_if_needed(final_class_name);
            header_wrapper_dumper.send(STR("static std::unordered_map<File::StringType, int32_t> MemberOffsets;\n\n"));
            wrapper_src_dumper.send(STR("std::unordered_map<File::StringType, int32_t> {}::MemberOffsets{{}};\n\n"), final_class_name);

            // Check if this class has an inheritance relationship that needs special handling
            auto inheritance_info = ConfigUtil::GetClassInheritance(class_entry.class_name);

            // Use configuration instead of hardcoded values
            const auto& private_variables_map = ConfigUtil::GetPrivateVariables();
            auto private_variables_for_class = private_variables_map.find(class_entry.class_name);

            // Initialize processed variables for this class if needed
            if (processed_variables.find(final_class_name) == processed_variables.end())
            {
                processed_variables[final_class_name] = std::unordered_set<File::StringType>();
            }

            for (const auto& [variable_name, variable] : class_entry.variables)
            {
                // Check if this type should be excluded from getters based on configuration
                if (ConfigUtil::ShouldFilterType(variable.type, TypeFilterCategory::ExcludeFromGetters))
                {
                    continue;
                }

                bool is_private = private_variables_for_class != private_variables_map.end() &&
                                  private_variables_for_class->second.find(variable.name) != private_variables_for_class->second.end();

                File::StringType final_variable_name = variable.name;
                File::StringType final_type_name = variable.type;

                // Apply class-specific member renaming
                auto rename_info = ConfigUtil::GetMemberRenameInfo(class_entry.class_name, variable.name);
                if (rename_info.has_value())
                {
                    Output::send(STR("Renaming member {}.{} to {}\n"), class_entry.class_name, variable.name, rename_info->mapped_name);
                    final_variable_name = rename_info->mapped_name;
                }

                unify_uobject_array_if_needed(final_type_name);

                // Check if we've already processed this variable name
                if (processed_variables[final_class_name].find(final_variable_name) != processed_variables[final_class_name].end())
                {
                    continue; // Skip duplicate
                }
                processed_variables[final_class_name].insert(final_variable_name);

                if (is_private)
                {
                    header_wrapper_dumper.send(STR("private:\n"));
                }
                else
                {
                    header_wrapper_dumper.send(STR("public:\n"));
                }

                header_wrapper_dumper.send(STR("    {}& Get{}();\n"), final_type_name, final_variable_name);
                header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n\n"), final_type_name, final_variable_name);
                wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), final_type_name, final_class_name, final_variable_name);

                // Handle classes with inheritance relationship
                if (inheritance_info.has_value())
                {
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(STR("    static const int32_t offset = []() -> int32_t {\n"));
                    wrapper_src_dumper.send(STR("        static auto& primary_offsets = Version::IsBelow({}, {}) ? {}::MemberOffsets : {}::MemberOffsets;\n"),
                                            inheritance_info->version_major,
                                            inheritance_info->version_minor,
                                            final_class_name,
                                            inheritance_info->parent_class);
                    wrapper_src_dumper.send(STR("        auto offset_it = primary_offsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR("        if (offset_it == primary_offsets.end()) {\n"));

                    // Only check other class if we're not already using it and if inheritance is enabled
                    if (inheritance_info->inherit_members)
                    {
                        wrapper_src_dumper.send(STR("            // Check in the other class if member not found\n"));
                        wrapper_src_dumper.send(
                                STR("            static auto& fallback_offsets = Version::IsBelow({}, {}) ? {}::MemberOffsets : {}::MemberOffsets;\n"),
                                inheritance_info->version_major,
                                inheritance_info->version_minor,
                                inheritance_info->parent_class,
                                // Swapped these
                                final_class_name); // Swapped these
                        wrapper_src_dumper.send(STR("            if (&fallback_offsets != &primary_offsets) { // Only check if different from primary\n"));
                        wrapper_src_dumper.send(STR("                auto fallback_offset = fallback_offsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR("                if (fallback_offset != fallback_offsets.end()) {\n"));
                        wrapper_src_dumper.send(STR("                    return fallback_offset->second;\n"));
                        wrapper_src_dumper.send(STR("                }\n"));
                        wrapper_src_dumper.send(STR("            }\n"));
                        wrapper_src_dumper.send(
                                STR(
                                        "            throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version or its parent class.\"}};\n"),
                                final_class_name,
                                final_variable_name);
                    }
                    else
                    {
                        wrapper_src_dumper.send(
                                STR(
                                        "            throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}};\n"),
                                final_class_name,
                                final_variable_name);
                    }
                    wrapper_src_dumper.send(STR("        }\n"));
                    wrapper_src_dumper.send(STR("        return offset_it->second;\n"));
                    wrapper_src_dumper.send(STR("    }();\n"));
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset);\n"), final_type_name);
                    wrapper_src_dumper.send(STR("}\n"));

                    // Now do the same for the const version
                    wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), final_type_name, final_class_name, final_variable_name);
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(STR("    static const int32_t offset = []() -> int32_t {\n"));
                    wrapper_src_dumper.send(STR("        static auto& primary_offsets = Version::IsBelow({}, {}) ? {}::MemberOffsets : {}::MemberOffsets;\n"),
                                            inheritance_info->version_major,
                                            inheritance_info->version_minor,
                                            final_class_name,
                                            inheritance_info->parent_class);
                    wrapper_src_dumper.send(STR("        auto offset_it = primary_offsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR("        if (offset_it == primary_offsets.end()) {\n"));

                    // Only check other class if we're not already using it and if inheritance is enabled
                    if (inheritance_info->inherit_members)
                    {
                        wrapper_src_dumper.send(STR("            // Check in the other class if member not found\n"));
                        wrapper_src_dumper.send(
                                STR("            static auto& fallback_offsets = Version::IsBelow({}, {}) ? {}::MemberOffsets : {}::MemberOffsets;\n"),
                                inheritance_info->version_major,
                                inheritance_info->version_minor,
                                inheritance_info->parent_class,
                                // Swapped these
                                final_class_name); // Swapped these
                        wrapper_src_dumper.send(STR("            if (&fallback_offsets != &primary_offsets) { // Only check if different from primary\n"));
                        wrapper_src_dumper.send(STR("                auto fallback_offset = fallback_offsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR("                if (fallback_offset != fallback_offsets.end()) {\n"));
                        wrapper_src_dumper.send(STR("                    return fallback_offset->second;\n"));
                        wrapper_src_dumper.send(STR("                }\n"));
                        wrapper_src_dumper.send(STR("            }\n"));
                        wrapper_src_dumper.send(
                                STR(
                                        "            throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version or its parent class.\"}};\n"),
                                final_class_name,
                                final_variable_name);
                    }
                    else
                    {
                        wrapper_src_dumper.send(
                                STR(
                                        "            throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}};\n"),
                                final_class_name,
                                final_variable_name);
                    }
                    wrapper_src_dumper.send(STR("        }\n"));
                    wrapper_src_dumper.send(STR("        return offset_it->second;\n"));
                    wrapper_src_dumper.send(STR("    }();\n"));
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset);\n"), final_type_name);
                    wrapper_src_dumper.send(STR("}\n\n"));
                }
                else
                {
                    // Standard handling for classes without special inheritance
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(STR("    static const int32_t offset = []() -> int32_t {\n"));
                    wrapper_src_dumper.send(STR("        auto offset_it = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR(
                            "        if (offset_it == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                            "that doesn't exist in this engine version.\"}}; }}\n"),
                                            final_class_name,
                                            final_variable_name);
                    wrapper_src_dumper.send(STR("        return offset_it->second;\n"));
                    wrapper_src_dumper.send(STR("    }();\n"));
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset);\n"), final_type_name);
                    wrapper_src_dumper.send(STR("}\n"));

                    wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), final_type_name, final_class_name, final_variable_name);
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(STR("    static const int32_t offset = []() -> int32_t {\n"));
                    wrapper_src_dumper.send(STR("        auto offset_it = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR(
                            "        if (offset_it == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                            "that doesn't exist in this engine version.\"}}; }}\n"),
                                            final_class_name,
                                            final_variable_name);
                    wrapper_src_dumper.send(STR("        return offset_it->second;\n"));
                    wrapper_src_dumper.send(STR("    }();\n"));
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset);\n"), final_type_name);
                    wrapper_src_dumper.send(STR("}\n\n"));
                }

                // Generate macro setter only for the final variable name
                macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"),
                                         final_class_name,
                                         rename_info.has_value() ? variable.name : final_variable_name); // Use original name if it's been renamed
                macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"),
                                         final_class_name,
                                         final_variable_name);

                // Also check for the renamed version if applicable
                if (rename_info.has_value() && rename_info->generate_alias_setter && final_variable_name != variable.name)
                {
                    macro_setter_dumper.send(STR("// Also support using the renamed version in the INI file\n"));
                    macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"),
                                             final_class_name,
                                             final_variable_name); // Renamed version
                    macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"),
                                             final_class_name,
                                             final_variable_name);
                }
            }
        }
    }

    auto MemberVarsWrapperGenerator::output_cleanup() -> void
    {
        if (std::filesystem::exists(member_variable_layouts_gen_output_include_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_include_path))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != STR(".hpp"))
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }
    }
} // namespace RC::UVTD