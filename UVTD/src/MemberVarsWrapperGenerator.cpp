#include <algorithm>
#include <format>
#include <set>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/ConfigUtil.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/MemberVarsWrapperGenerator.hpp>

namespace RC::UVTD
{
    // Generate version suffix for getter names (e.g., "425" for version 4.25, "500" for 5.0)
    static auto generate_version_suffix(int32_t major, int32_t minor) -> File::StringType
    {
        return std::format(STR("{}{:02d}"), major, minor);
    }

    // Normalize a type for comparison purposes
    // This handles equivalent types that should be considered the same:
    // - FDefaultAllocator == TSizedDefaultAllocator<32>
    // - T* in TArray/TSet/TMap == TObjectPtr<T> in TArray/TSet/TMap
    // - Whitespace differences (e.g., "TArray<T >" vs "TArray<T>")
    static auto normalize_type_for_comparison(const File::StringType& type) -> File::StringType
    {
        File::StringType normalized = type;

        // Normalize allocator types: TSizedDefaultAllocator<32> -> FDefaultAllocator
        // These are equivalent in UE (TSizedDefaultAllocator<32> was introduced later but is the same)
        size_t pos = 0;
        while ((pos = normalized.find(STR("TSizedDefaultAllocator<32>"), pos)) != File::StringType::npos)
        {
            normalized.replace(pos, 26, STR("FDefaultAllocator"));
            // Don't increment pos - the replacement is shorter, so we're already past it
        }

        // Normalize whitespace: remove spaces before > (e.g., "TArray<T >" -> "TArray<T>")
        pos = 0;
        while ((pos = normalized.find(STR(" >"), pos)) != File::StringType::npos)
        {
            normalized.erase(pos, 1);
            // Don't increment - we removed a char, check same position again
        }

        // Also normalize " >" at the very end
        while (!normalized.empty() && normalized.back() == ' ')
        {
            normalized.pop_back();
        }

        // For container types (TArray, TSet, TMap), normalize TObjectPtr<T> to T*
        // This allows us to treat TArray<AActor*> and TArray<TObjectPtr<AActor>> as equivalent
        if (normalized.starts_with(STR("TArray<")) ||
            normalized.starts_with(STR("TSet<")) ||
            normalized.starts_with(STR("TMap<")))
        {
            // Find TObjectPtr<X> and replace with X*
            size_t tobj_pos = 0;
            while ((tobj_pos = normalized.find(STR("TObjectPtr<"), tobj_pos)) != File::StringType::npos)
            {
                // Find the matching closing >
                size_t start = tobj_pos + 11; // after "TObjectPtr<"
                int depth = 1;
                size_t end = start;
                while (end < normalized.size() && depth > 0)
                {
                    if (normalized[end] == '<') depth++;
                    else if (normalized[end] == '>') depth--;
                    if (depth > 0) end++;
                }

                if (depth == 0)
                {
                    // Extract the inner type
                    File::StringType inner_type = normalized.substr(start, end - start);
                    // Replace TObjectPtr<X> with X*
                    normalized.replace(tobj_pos, end - tobj_pos + 1, inner_type + STR("*"));
                }
                else
                {
                    tobj_pos++; // Move past to avoid infinite loop
                }
            }
        }

        // Normalize pointer spacing: "T *" -> "T*"
        pos = 0;
        while ((pos = normalized.find(STR(" *"), pos)) != File::StringType::npos)
        {
            normalized.erase(pos, 1);
        }

        return normalized;
    }

    // Check if a type should be skipped because a better version exists
    // Returns true if this type should be skipped
    static auto should_skip_type_variant(
        const File::StringType& type,
        const std::map<File::StringType, VersionedType>& all_versions) -> bool
    {
        // For non-container raw pointers, skip if TObjectPtr version exists
        if (!type.starts_with(STR("TObjectPtr<")) && type.ends_with(STR("*")) &&
            !type.starts_with(STR("TArray<")) && !type.starts_with(STR("TSet<")) && !type.starts_with(STR("TMap<")))
        {
            // Check if any version has TObjectPtr
            for (const auto& [_, vt] : all_versions)
            {
                if (vt.type.starts_with(STR("TObjectPtr<")))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // Helper to generate a simple getter body (non-const version)
    static auto generate_simple_getter_body(
        Output::Targets<Output::NewFileDevice>& dumper,
        const File::StringType& type_name,
        const File::StringType& class_name,
        const File::StringType& variable_name) -> void
    {
        dumper.send(STR("{\n"));
        dumper.send(STR("    static const int32_t offset = []() -> int32_t {\n"));
        dumper.send(STR("        auto offset_it = MemberOffsets.find(STR(\"{}\"));\n"), variable_name);
        dumper.send(STR(
                "        if (offset_it == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                "that doesn't exist in this engine version.\"}}; }}\n"),
                        class_name,
                        variable_name);
        dumper.send(STR("        return offset_it->second;\n"));
        dumper.send(STR("    }();\n"));
        dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset);\n"), type_name);
        dumper.send(STR("}\n"));
    }

    // Helper to generate a simple const getter body
    static auto generate_simple_const_getter_body(
        Output::Targets<Output::NewFileDevice>& dumper,
        const File::StringType& type_name,
        const File::StringType& class_name,
        const File::StringType& variable_name) -> void
    {
        dumper.send(STR("{\n"));
        dumper.send(STR("    static const int32_t offset = []() -> int32_t {\n"));
        dumper.send(STR("        auto offset_it = MemberOffsets.find(STR(\"{}\"));\n"), variable_name);
        dumper.send(STR(
                "        if (offset_it == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                "that doesn't exist in this engine version.\"}}; }}\n"),
                        class_name,
                        variable_name);
        dumper.send(STR("        return offset_it->second;\n"));
        dumper.send(STR("    }();\n"));
        dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset);\n"), type_name);
        dumper.send(STR("}\n\n"));
    }

    auto MemberVarsWrapperGenerator::generate_files() -> void
    {
        auto macro_setter_file = macro_setter_output_path / std::filesystem::path{STR("MacroSetter.hpp")};

        Output::send(STR("Generating file '{}'\n"), macro_setter_file.wstring());

        Output::Targets<Output::NewFileDevice> macro_setter_dumper;
        auto& macro_setter_file_device = macro_setter_dumper.get_device<Output::NewFileDevice>();
        macro_setter_file_device.set_file_name_and_path(macro_setter_file);
        macro_setter_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{string};
        });

        // Keep track of processed variables to avoid duplicates
        std::unordered_map<File::StringType, std::unordered_set<File::StringType>> processed_variables;

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

            // Skip if no class entry or the item is not valid for member variables
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

            auto wrapper_header_file = member_variable_layouts_wrappers_output_path /
                                       std::format(STR("MemberVariableLayout_HeaderWrapper_{}.hpp"), class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());

            Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
            auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
            wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
            wrapper_header_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            auto wrapper_src_file = member_variable_layouts_wrappers_output_path / 
                                    std::format(STR("MemberVariableLayout_SrcWrapper_{}.hpp"), class_entry.class_name_clean);

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

            for (const auto& variable : class_entry.variables)
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

                // Check if this variable has version-specific type changes
                // If so, we generate versioned getters instead of a single getter
                if (variable.has_type_changes() && !inheritance_info.has_value())
                {
                    // Generate version-specific getters
                    // The types_by_version map is sorted by version key (e.g., "4_25", "5_00")
                    // We generate:
                    // - GetVarBase() - for the earliest version
                    // - GetVar425() - for >= 4.25 (or whatever version introduced the change)
                    // - etc.

                    Output::send(STR("  Generating version-specific getters for {}::{}\n"),
                                 final_class_name, final_variable_name);

                    bool is_first = true;
                    for (const auto& [version_key, versioned_type] : variable.types_by_version)
                    {
                        File::StringType version_type_name = versioned_type.type;
                        unify_uobject_array_if_needed(version_type_name);

                        File::StringType getter_suffix;
                        if (is_first)
                        {
                            getter_suffix = STR("Base");
                            is_first = false;
                        }
                        else
                        {
                            getter_suffix = generate_version_suffix(versioned_type.major_version, versioned_type.minor_version);
                        }

                        File::StringType versioned_getter_name = final_variable_name + getter_suffix;

                        // Header declarations
                        header_wrapper_dumper.send(STR("    {}& Get{}();\n"), version_type_name, versioned_getter_name);
                        header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n"), version_type_name, versioned_getter_name);

                        // Source implementations
                        wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), version_type_name, final_class_name, versioned_getter_name);
                        generate_simple_getter_body(wrapper_src_dumper, version_type_name, final_class_name, final_variable_name);

                        wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), version_type_name, final_class_name, versioned_getter_name);
                        generate_simple_const_getter_body(wrapper_src_dumper, version_type_name, final_class_name, final_variable_name);
                    }

                    // Add a newline after the versioned getters in header
                    header_wrapper_dumper.send(STR("\n"));
                }
                else
                {
                    // Standard getter - single type for all versions
                    header_wrapper_dumper.send(STR("    {}& Get{}();\n"), final_type_name, final_variable_name);
                    header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n\n"), final_type_name, final_variable_name);
                    wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), final_type_name, final_class_name, final_variable_name);
                }

                // Handle classes with inheritance relationship (only for non-versioned getters)
                if (inheritance_info.has_value() && !variable.has_type_changes())
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
                else if (!variable.has_type_changes())
                {
                    // Standard handling for classes without special inheritance or type changes
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
                // Note: if variable.has_type_changes() is true, getters were already generated above

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
        // Cleanup header wrappers
        if (std::filesystem::exists(member_variable_layouts_wrappers_output_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_wrappers_output_path))
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