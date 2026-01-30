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

    // Add const prefix to type if it doesn't already start with const
    // Prevents "const const T*" when the type is already "const T*"
    static auto add_const_if_needed(const File::StringType& type) -> File::StringType
    {
        if (type.starts_with(STR("const ")))
        {
            return type;
        }
        return STR("const ") + type;
    }

    // Check if a type is a pointer-to-member-function
    // These have the format: ReturnType (ClassName::*)(Args...)
    // They need special handling because the syntax for reference return types is complex
    static auto is_pointer_to_member_function(const File::StringType& type) -> bool
    {
        // Look for the pattern "::*)(" which is unique to pointer-to-member-function types
        return type.find(STR("::*)(")) != File::StringType::npos;
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
        dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset);\n"), add_const_if_needed(type_name));
        dumper.send(STR("}\n\n"));
    }


    // Information about a versioned getter that needs a public wrapper
    struct VersionedGetterInfo
    {
        File::StringType variable_name;
        File::StringType getter_suffix;
        File::StringType type_name;
        int32_t major_version;
        int32_t minor_version;
    };

    // Information about a variable that needs versioned wrapper generation
    struct VariableWrapperInfo
    {
        File::StringType class_name;
        File::StringType variable_name;
        std::vector<VersionedGetterInfo> getters;
    };

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

        // Generate helper function for parsing member offset values (supports bitfield format: offset:bit_pos:bit_len:storage_size)
        // INI is processed first, then defaults - emplace ensures INI values aren't overwritten
        macro_setter_dumper.send(STR("auto ParseMemberOffset = [](const File::StringType& val_str, auto& offsets, auto& bitfields, const File::StringType& member_name)\n"));
        macro_setter_dumper.send(STR("{\n"));
        macro_setter_dumper.send(STR("    try {\n"));
        macro_setter_dumper.send(STR("        int32_t offset = std::stoi(val_str, nullptr, 0);\n"));
        macro_setter_dumper.send(STR("        offsets.emplace(member_name, offset);\n"));
        macro_setter_dumper.send(STR("        size_t colon1 = val_str.find(':');\n"));
        macro_setter_dumper.send(STR("        if (colon1 != File::StringType::npos && colon1 + 1 < val_str.size()) {\n"));
        macro_setter_dumper.send(STR("            uint8_t bit_pos = static_cast<uint8_t>(std::stoi(val_str.substr(colon1 + 1)));\n"));
        macro_setter_dumper.send(STR("            uint8_t bit_len = 1;\n"));
        macro_setter_dumper.send(STR("            uint8_t storage_size = 1;\n"));
        macro_setter_dumper.send(STR("            size_t colon2 = val_str.find(':', colon1 + 1);\n"));
        macro_setter_dumper.send(STR("            if (colon2 != File::StringType::npos && colon2 + 1 < val_str.size()) {\n"));
        macro_setter_dumper.send(STR("                bit_len = static_cast<uint8_t>(std::stoi(val_str.substr(colon2 + 1)));\n"));
        macro_setter_dumper.send(STR("                size_t colon3 = val_str.find(':', colon2 + 1);\n"));
        macro_setter_dumper.send(STR("                if (colon3 != File::StringType::npos && colon3 + 1 < val_str.size()) {\n"));
        macro_setter_dumper.send(STR("                    storage_size = static_cast<uint8_t>(std::stoi(val_str.substr(colon3 + 1)));\n"));
        macro_setter_dumper.send(STR("                }\n"));
        macro_setter_dumper.send(STR("            }\n"));
        macro_setter_dumper.send(STR("            bitfields.emplace(member_name, Unreal::BitfieldInfo{bit_pos, bit_len, storage_size});\n"));
        macro_setter_dumper.send(STR("        }\n"));
        macro_setter_dumper.send(STR("    } catch (const std::exception&) {}\n"));
        macro_setter_dumper.send(STR("};\n\n"));

        // Collect variables that need versioned wrappers (for separate file generation)
        std::vector<VariableWrapperInfo> variables_needing_wrappers;

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

            auto final_class_name_clean = class_entry.class_name_clean;
            unify_uobject_array_if_needed(final_class_name_clean);
            auto wrapper_header_file = member_variable_layouts_wrappers_output_path /
                                       std::format(STR("MemberVariableLayout_HeaderWrapper_{}.hpp"), final_class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());

            Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
            auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
            wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
            wrapper_header_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            auto wrapper_src_file = member_variable_layouts_wrappers_output_path / 
                                    std::format(STR("MemberVariableLayout_SrcWrapper_{}.hpp"), final_class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_src_file.wstring());

            Output::Targets<Output::NewFileDevice> wrapper_src_dumper;
            auto& wrapper_src_file_device = wrapper_src_dumper.get_device<Output::NewFileDevice>();
            wrapper_src_file_device.set_file_name_and_path(wrapper_src_file);
            wrapper_src_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            auto final_class_name = class_entry.class_name;
            unify_uobject_array_if_needed(final_class_name);
            header_wrapper_dumper.send(STR("static std::unordered_map<File::StringType, int32_t> MemberOffsets;\n"));
            header_wrapper_dumper.send(STR("static std::unordered_map<File::StringType, BitfieldInfo> BitfieldInfos;\n\n"));
            wrapper_src_dumper.send(STR("std::unordered_map<File::StringType, int32_t> {}::MemberOffsets{{}};\n"), final_class_name);
            wrapper_src_dumper.send(STR("std::unordered_map<File::StringType, BitfieldInfo> {}::BitfieldInfos{{}};\n\n"), final_class_name);

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
                    if (!rename_info->mapped_name.empty())
                    {
                        Output::send(STR("Renaming member {}.{} to {}\n"), class_entry.class_name, variable.name, rename_info->mapped_name);
                        final_variable_name = rename_info->mapped_name;
                    }
                    if (!rename_info->mapped_type_name.empty())
                    {
                        Output::send(STR("Renaming type {} for {}.{} to {}\n"), final_type_name, class_entry.class_name, variable.name, rename_info->mapped_type_name);
                        final_type_name = rename_info->mapped_type_name;
                    }
                }

                unify_uobject_array_if_needed(final_type_name);

                // Check if we've already processed this variable name
                if (processed_variables[final_class_name].find(final_variable_name) != processed_variables[final_class_name].end())
                {
                    continue;
                }
                processed_variables[final_class_name].insert(final_variable_name);

                // Check if this variable has version-specific type changes
                // If so, we generate versioned getters instead of a single getter
                // Versioned getters are always private - public wrappers will be added later
                // Versioned bitfields are handled by BitfieldProxy at runtime (storage_size, bit_pos, bit_len from INI),
                // so they don't need versioned getters
                bool has_versioned_getters = variable.has_type_changes() && !inheritance_info.has_value() && !variable.is_bitfield;

                // Count how many unique getters we'll actually generate after normalization
                // Types that normalize to the same thing (e.g., FDefaultAllocator vs TSizedDefaultAllocator<32>,
                // or TArray<T*> vs TArray<TObjectPtr<T>>) should only generate one getter
                size_t valid_getter_count = 0;
                std::set<File::StringType> seen_normalized_types;
                File::StringType best_type_for_single_getter; // The "best" type to use if we only need one getter

                if (has_versioned_getters)
                {
                    for (const auto& [version_key, versioned_type] : variable.types_by_version)
                    {
                        File::StringType version_type_name = versioned_type.type;
                        unify_uobject_array_if_needed(version_type_name);

                        // Skip raw pointer types when TObjectPtr version exists (for non-container types)
                        if (should_skip_type_variant(version_type_name, variable.types_by_version))
                        {
                            continue;
                        }

                        // Normalize the type for comparison
                        File::StringType normalized = normalize_type_for_comparison(version_type_name);

                        // Only count unique normalized types
                        if (seen_normalized_types.find(normalized) == seen_normalized_types.end())
                        {
                            seen_normalized_types.insert(normalized);
                            valid_getter_count++;
                        }

                        // Always update best_type_for_single_getter - we want the newest version
                        // Since we iterate in version order (oldest first), later versions overwrite
                        // This ensures we use TObjectPtr/TSizedDefaultAllocator versions when available
                        best_type_for_single_getter = version_type_name;
                    }
                }

                // Only treat as versioned if we have multiple unique types after normalization
                // If only one unique type remains, treat as a normal getter with the best type
                bool effectively_has_versioned_getters = has_versioned_getters && valid_getter_count > 1;

                if (is_private || effectively_has_versioned_getters)
                {
                    header_wrapper_dumper.send(STR("private:\n"));
                }
                else
                {
                    header_wrapper_dumper.send(STR("public:\n"));
                }

                if (has_versioned_getters)
                {
                    // If after normalization we only have one unique type, generate a single getter
                    // using the "best" type (the most recent version, which has TObjectPtr/TSizedDefaultAllocator)
                    if (valid_getter_count == 1)
                    {
                        Output::send(STR("  Unified versioned getters for {}::{} to single type: {}\n"),
                                     final_class_name, final_variable_name, best_type_for_single_getter);

                        // Generate single getter with the best type (no suffix)
                        header_wrapper_dumper.send(STR("    {}& Get{}();\n"), best_type_for_single_getter, final_variable_name);
                        header_wrapper_dumper.send(STR("    {}& Get{}() const;\n\n"), add_const_if_needed(best_type_for_single_getter), final_variable_name);

                        wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), best_type_for_single_getter, final_class_name, final_variable_name);
                        generate_simple_getter_body(wrapper_src_dumper, best_type_for_single_getter, final_class_name, final_variable_name);

                        wrapper_src_dumper.send(STR("{}& {}::Get{}() const\n"), add_const_if_needed(best_type_for_single_getter), final_class_name, final_variable_name);
                        generate_simple_const_getter_body(wrapper_src_dumper, best_type_for_single_getter, final_class_name, final_variable_name);
                    }
                    else
                    {
                        // Generate version-specific getters
                        // The types_by_version map is sorted by version key (e.g., "4_25", "5_00")
                        // We generate:
                        // - GetVarBase() - for the earliest version
                        // - GetVar425() - for >= 4.25 (or whatever version introduced the change)
                        // - etc.

                        Output::send(STR("  Generating version-specific getters for {}::{}\n"),
                                     final_class_name, final_variable_name);

                        // Collect info for wrapper generation if we have multiple valid getters
                        VariableWrapperInfo wrapper_info;
                        wrapper_info.class_name = final_class_name;
                        wrapper_info.variable_name = final_variable_name;

                        // Track which normalized types we've already generated getters for
                        std::set<File::StringType> generated_normalized_types;

                        // Generate the getters
                        bool is_first = true;
                        for (const auto& [version_key, versioned_type] : variable.types_by_version)
                        {
                            File::StringType version_type_name = versioned_type.type;
                            unify_uobject_array_if_needed(version_type_name);

                            // Skip raw pointer types when TObjectPtr version exists (for non-container types)
                            if (should_skip_type_variant(version_type_name, variable.types_by_version))
                            {
                                Output::send(STR("  Skipping type variant for {}::{} (type: {})\n"),
                                             final_class_name, final_variable_name, version_type_name);
                                continue;
                            }

                            // Skip if we've already generated a getter for this normalized type
                            File::StringType normalized = normalize_type_for_comparison(version_type_name);
                            if (generated_normalized_types.find(normalized) != generated_normalized_types.end())
                            {
                                Output::send(STR("  Skipping duplicate normalized type for {}::{} (type: {} -> {})\n"),
                                             final_class_name, final_variable_name, version_type_name, normalized);
                                continue;
                            }
                            generated_normalized_types.insert(normalized);

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

                            // Collect for wrapper generation
                            wrapper_info.getters.push_back({
                                final_variable_name,
                                getter_suffix,
                                version_type_name,
                                versioned_type.major_version,
                                versioned_type.minor_version
                            });

                            File::StringType versioned_getter_name = final_variable_name + getter_suffix;

                            // Check if this is a pointer-to-member-function type which needs special handling
                            if (is_pointer_to_member_function(version_type_name))
                            {
                                // Generate typedef with unique name for this versioned getter
                                File::StringType typedef_name = versioned_getter_name + STR("_Type");
                                header_wrapper_dumper.send(STR("    using {} = {};\n"), typedef_name, version_type_name);
                                header_wrapper_dumper.send(STR("    {}& Get{}();\n"), typedef_name, versioned_getter_name);
                                header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n"), typedef_name, versioned_getter_name);

                                // Source implementations with qualified typedef
                                wrapper_src_dumper.send(STR("{}::{}& {}::Get{}()\n"), final_class_name, typedef_name, final_class_name, versioned_getter_name);
                                generate_simple_getter_body(wrapper_src_dumper, typedef_name, final_class_name, final_variable_name);

                                wrapper_src_dumper.send(STR("const {}::{}& {}::Get{}() const\n"), final_class_name, typedef_name, final_class_name, versioned_getter_name);
                                generate_simple_const_getter_body(wrapper_src_dumper, typedef_name, final_class_name, final_variable_name);
                            }
                            else
                            {
                                // Header declarations
                                header_wrapper_dumper.send(STR("    {}& Get{}();\n"), version_type_name, versioned_getter_name);
                                header_wrapper_dumper.send(STR("    {}& Get{}() const;\n"), add_const_if_needed(version_type_name), versioned_getter_name);

                                // Source implementations
                                wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), version_type_name, final_class_name, versioned_getter_name);
                                generate_simple_getter_body(wrapper_src_dumper, version_type_name, final_class_name, final_variable_name);

                                wrapper_src_dumper.send(STR("{}& {}::Get{}() const\n"), add_const_if_needed(version_type_name), final_class_name, versioned_getter_name);
                                generate_simple_const_getter_body(wrapper_src_dumper, version_type_name, final_class_name, final_variable_name);
                            }
                        }

                        // Add a newline after the versioned getters in header
                        header_wrapper_dumper.send(STR("\n"));

                        // Save wrapper info if we have multiple versioned getters
                        if (wrapper_info.getters.size() > 1)
                        {
                            variables_needing_wrappers.push_back(std::move(wrapper_info));
                        }
                    }
                }
                else
                {
                    // Standard getter - single type for all versions
                    if (variable.is_bitfield)
                    {
                        // For bitfields, use BitfieldProxy for read/write access via single getter
                        // Bit position, length, and storage size come from BitfieldInfos at runtime (supports INI overrides)
                        // Bitfields don't care about type changes - they always use BitfieldProxy
                        header_wrapper_dumper.send(STR("    BitfieldProxy Get{}();\n"), final_variable_name);
                        header_wrapper_dumper.send(STR("    ConstBitfieldProxy Get{}() const;\n\n"), final_variable_name);

                        // Generate non-const BitfieldProxy getter implementation (with body)
                        wrapper_src_dumper.send(STR("BitfieldProxy {}::Get{}()\n"), final_class_name, final_variable_name);
                        wrapper_src_dumper.send(STR("{\n"));
                        wrapper_src_dumper.send(STR("    static auto offset_it = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR(
                                "    if (offset_it == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                                "that doesn't exist in this engine version.\"}}; }}\n"),
                                                final_class_name,
                                                final_variable_name);
                        wrapper_src_dumper.send(STR("    static auto bitfield_it = BitfieldInfos.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR(
                                "    if (bitfield_it == BitfieldInfos.end()) {{ throw std::runtime_error{{\"Bitfield info not found for '{}::{}'.\"}}; }}\n"),
                                                final_class_name,
                                                final_variable_name);
                        wrapper_src_dumper.send(STR("    auto& info = bitfield_it->second;\n"));
                        wrapper_src_dumper.send(STR("    return BitfieldProxy(Helper::Casting::ptr_cast<void*>(this, offset_it->second), info.bit_pos, info.bit_len, info.storage_size);\n"));
                        wrapper_src_dumper.send(STR("}\n"));

                        // Generate const ConstBitfieldProxy getter implementation
                        wrapper_src_dumper.send(STR("ConstBitfieldProxy {}::Get{}() const\n"), final_class_name, final_variable_name);
                        wrapper_src_dumper.send(STR("{\n"));
                        wrapper_src_dumper.send(STR("    static auto offset_it = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR(
                                "    if (offset_it == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                                "that doesn't exist in this engine version.\"}}; }}\n"),
                                                final_class_name,
                                                final_variable_name);
                        wrapper_src_dumper.send(STR("    static auto bitfield_it = BitfieldInfos.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR(
                                "    if (bitfield_it == BitfieldInfos.end()) {{ throw std::runtime_error{{\"Bitfield info not found for '{}::{}'.\"}}; }}\n"),
                                                final_class_name,
                                                final_variable_name);
                        wrapper_src_dumper.send(STR("    auto& info = bitfield_it->second;\n"));
                        wrapper_src_dumper.send(STR("    return ConstBitfieldProxy(Helper::Casting::ptr_cast<const void*>(this, offset_it->second), info.bit_pos, info.bit_len, info.storage_size);\n"));
                        wrapper_src_dumper.send(STR("}\n\n"));
                        // Bitfield body generation complete - fall through to MacroSetter generation
                    }
                    else
                    {
                        // Check if this is a pointer-to-member-function type which needs special handling
                        // These types have complex syntax for reference return types: ReturnType (ClassName::*&)(Args...)
                        // We handle this by generating a typedef
                        if (is_pointer_to_member_function(final_type_name))
                        {
                            File::StringType typedef_name = final_variable_name + STR("_Type");
                            header_wrapper_dumper.send(STR("    using {} = {};\n"), typedef_name, final_type_name);
                            header_wrapper_dumper.send(STR("    {}& Get{}();\n"), typedef_name, final_variable_name);
                            header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n\n"), typedef_name, final_variable_name);
                            wrapper_src_dumper.send(STR("{}::{}& {}::Get{}()\n"), final_class_name, typedef_name, final_class_name, final_variable_name);
                        }
                        else
                        {
                            header_wrapper_dumper.send(STR("    {}& Get{}();\n"), final_type_name, final_variable_name);
                            header_wrapper_dumper.send(STR("    {}& Get{}() const;\n\n"), add_const_if_needed(final_type_name), final_variable_name);
                            wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), final_type_name, final_class_name, final_variable_name);
                        }
                    }
                }

                // Handle classes with inheritance relationship (only for non-versioned, non-bitfield getters)
                if (inheritance_info.has_value() && !variable.has_type_changes() && !variable.is_bitfield)
                {
                    bool is_pmf_inherit = is_pointer_to_member_function(final_type_name);
                    File::StringType typedef_name_inherit = final_variable_name + STR("_Type");

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

                    // Now do the same for the const version - use typedef for pointer-to-member-function types
                    if (is_pmf_inherit)
                    {
                        wrapper_src_dumper.send(STR("const {}::{}& {}::Get{}() const\n"), final_class_name, typedef_name_inherit, final_class_name, final_variable_name);
                    }
                    else
                    {
                        wrapper_src_dumper.send(STR("{}& {}::Get{}() const\n"), add_const_if_needed(final_type_name), final_class_name, final_variable_name);
                    }
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
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset);\n"), add_const_if_needed(final_type_name));
                    wrapper_src_dumper.send(STR("}\n\n"));
                }
                else if (!variable.has_type_changes() && !variable.is_bitfield)
                {
                    // Standard handling for classes without special inheritance, type changes, or bitfields
                    bool is_pmf = is_pointer_to_member_function(final_type_name);
                    File::StringType typedef_name = final_variable_name + STR("_Type");

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

                    // Const getter - use typedef for pointer-to-member-function types
                    if (is_pmf)
                    {
                        wrapper_src_dumper.send(STR("const {}::{}& {}::Get{}() const\n"), final_class_name, typedef_name, final_class_name, final_variable_name);
                    }
                    else
                    {
                        wrapper_src_dumper.send(STR("{}& {}::Get{}() const\n"), add_const_if_needed(final_type_name), final_class_name, final_variable_name);
                    }
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
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset);\n"), add_const_if_needed(final_type_name));
                    wrapper_src_dumper.send(STR("}\n\n"));
                }
                // Note: if variable.has_type_changes() is true, getters were already generated above

                // Generate macro setter only for the final variable name
                // Bitfields use string parsing (supports "0x30:bit_pos:bit_len" format)
                // Other types use simple int64 parsing
                bool needs_bitfield_parsing = variable.is_bitfield;

                if (needs_bitfield_parsing)
                {
                    macro_setter_dumper.send(STR("if (auto val_str = parser.get_string(STR(\"{}\"), STR(\"{}\")); !val_str.empty())\n"),
                                             final_class_name,
                                             rename_info.has_value() ? variable.name : final_variable_name);
                    macro_setter_dumper.send(STR("    ParseMemberOffset(val_str, Unreal::{}::MemberOffsets, Unreal::{}::BitfieldInfos, STR(\"{}\"));\n"),
                                             final_class_name,
                                             final_class_name,
                                             final_variable_name);
                }
                else
                {
                    macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"),
                                             final_class_name,
                                             rename_info.has_value() ? variable.name : final_variable_name);
                    macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"),
                                             final_class_name,
                                             final_variable_name);
                }

                // Also check for the renamed version if applicable
                if (rename_info.has_value() && rename_info->generate_alias_setter && final_variable_name != variable.name)
                {
                    macro_setter_dumper.send(STR("// Also support using the renamed version in the INI file\n"));
                    if (needs_bitfield_parsing)
                    {
                        macro_setter_dumper.send(STR("if (auto val_str = parser.get_string(STR(\"{}\"), STR(\"{}\")); !val_str.empty())\n"),
                                                 final_class_name,
                                                 final_variable_name);
                        macro_setter_dumper.send(STR("    ParseMemberOffset(val_str, Unreal::{}::MemberOffsets, Unreal::{}::BitfieldInfos, STR(\"{}\"));\n"),
                                                 final_class_name,
                                                 final_class_name,
                                                 final_variable_name);
                    }
                    else
                    {
                        macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"),
                                                 final_class_name,
                                                 final_variable_name);
                        macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"),
                                                 final_class_name,
                                                 final_variable_name);
                    }
                }
            }

            // Add UEP_TotalSize getter at the end - returns cached reference to the total size of the class
            // Returns -1 if not found, allowing caller to use fallback logic to determine size
            header_wrapper_dumper.send(STR("public:\n"));
            header_wrapper_dumper.send(STR("    static int32_t& UEP_TotalSize();\n"));
            wrapper_src_dumper.send(STR("int32_t& {}::UEP_TotalSize()\n"), final_class_name);
            wrapper_src_dumper.send(STR("{\n"));
            wrapper_src_dumper.send(STR("    static int32_t cached = []() {\n"));
            wrapper_src_dumper.send(STR("        auto it = MemberOffsets.find(STR(\"UEP_TotalSize\"));\n"));
            wrapper_src_dumper.send(STR("        return it != MemberOffsets.end() ? it->second : -1;\n"));
            wrapper_src_dumper.send(STR("    }();\n"));
            wrapper_src_dumper.send(STR("    return cached;\n"));
            wrapper_src_dumper.send(STR("}\n\n"));

            // Add UEP_TotalSize macro setter entry
            macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"UEP_TotalSize\"), -1); val != -1)\n"),
                                     final_class_name);
            macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"UEP_TotalSize\"), static_cast<int32_t>(val));\n"),
                                     final_class_name);
        }

        // Generate versioned wrapper stubs file if we have any variables needing wrappers
        if (!variables_needing_wrappers.empty())
        {
            auto wrapper_stubs_file = member_variable_layouts_wrappers_output_path / std::filesystem::path{STR("VersionedMemberWrapperStubs.hpp")};

            Output::send(STR("Generating versioned wrapper stubs file '{}'\n"), wrapper_stubs_file.wstring());

            Output::Targets<Output::NewFileDevice> stubs_dumper;
            auto& stubs_file_device = stubs_dumper.get_device<Output::NewFileDevice>();
            stubs_file_device.set_file_name_and_path(wrapper_stubs_file);
            stubs_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            stubs_dumper.send(STR("// Auto-generated stubs for versioned member variable wrappers\n"));
            stubs_dumper.send(STR("// These members have different types across UE versions and need manual wrapper implementation\n"));
            stubs_dumper.send(STR("// Implement public wrapper functions that handle version switching\n\n"));

            for (const auto& wrapper_info : variables_needing_wrappers)
            {
                stubs_dumper.send(STR("// {}::{}\n"), wrapper_info.class_name, wrapper_info.variable_name);
                stubs_dumper.send(STR("// Available versioned getters:\n"));

                for (const auto& getter : wrapper_info.getters)
                {
                    if (getter.getter_suffix == STR("Base"))
                    {
                        stubs_dumper.send(STR("//   Get{}{}() -> {} (versions < {}.{})\n"),
                                         getter.variable_name, getter.getter_suffix, getter.type_name,
                                         wrapper_info.getters.size() > 1 ? wrapper_info.getters[1].major_version : 0,
                                         wrapper_info.getters.size() > 1 ? wrapper_info.getters[1].minor_version : 0);
                    }
                    else
                    {
                        stubs_dumper.send(STR("//   Get{}{}() -> {} (versions >= {}.{})\n"),
                                         getter.variable_name, getter.getter_suffix, getter.type_name,
                                         getter.major_version, getter.minor_version);
                    }
                }
                stubs_dumper.send(STR("\n"));
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