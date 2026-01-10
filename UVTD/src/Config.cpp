#include <fstream>
#include <format>
#include <iostream>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <UVTD/Config.hpp>

#include <glaze/glaze.hpp>

namespace RC::UVTD
{
    // Enum conversion helpers for JSON parsing
    inline std::string_view TypeFilterCategoryToString(TypeFilterCategory category) {
        switch (category) {
            case TypeFilterCategory::CompleteExclusion: return "complete_exclusion";
            case TypeFilterCategory::ExcludeFromGetters: return "exclude_from_getters";
            case TypeFilterCategory::ExcludeFromSolBindings: return "exclude_from_sol_bindings";
            default: return "unknown";
        }
    }

    inline TypeFilterCategory StringToTypeFilterCategory(const std::string& str) {
        if (str == "complete_exclusion") return TypeFilterCategory::CompleteExclusion;
        if (str == "exclude_from_getters") return TypeFilterCategory::ExcludeFromGetters;
        if (str == "exclude_from_sol_bindings") return TypeFilterCategory::ExcludeFromSolBindings;
        return TypeFilterCategory::CompleteExclusion; // Default
    }

    // Helper function to get a readable name for a filter category
    inline std::string GetReadableCategoryName(const std::string& category_str) {
        if (category_str == "complete_exclusion") return "Complete Exclusion";
        if (category_str == "exclude_from_getters") return "Exclude From Getters";
        if (category_str == "exclude_from_sol_bindings") return "Exclude From Sol Bindings";
        return category_str;
    }

    // Helper structs for JSON parsing
    struct ObjectItemJson {
        std::string name;
        ValidForVTable valid_for_vtable;
        ValidForMemberVars valid_for_member_vars;
    };
    
    struct MemberRenameInfoJson {
        std::string mapped_name;
        bool generate_alias_setter{false};
        std::string description;
    };

    struct ClassInheritanceInfoJson {
        std::string parent_class;
        int32_t version_major;
        int32_t version_minor;
        bool inherit_members{true};
    };

    struct SuffixDefinitionJson {
        std::string suffix;
        std::string ifdef_macro;
        std::string description;
        bool generates_variant{true};
    };

    struct SuffixDefinitionsFile {
        std::vector<SuffixDefinitionJson> suffixes;
    };

    // New type for type filtering
    using TypeFilterMapJson = std::unordered_map<std::string, std::vector<std::string>>;
}

// Glaze JSON serialization metadata 
namespace glz {
    template <>
    struct meta<RC::UVTD::ObjectItemJson> {
        static constexpr auto value = glz::object(
            "name", &RC::UVTD::ObjectItemJson::name,
            "valid_for_vtable", &RC::UVTD::ObjectItemJson::valid_for_vtable,
            "valid_for_member_vars", &RC::UVTD::ObjectItemJson::valid_for_member_vars
        );
    };
    
    template <>
    struct meta<RC::UVTD::MemberRenameInfoJson> {
        static constexpr auto value = glz::object(
            "mapped_name", &RC::UVTD::MemberRenameInfoJson::mapped_name,
            "generate_alias_setter", &RC::UVTD::MemberRenameInfoJson::generate_alias_setter,
            "description", &RC::UVTD::MemberRenameInfoJson::description
        );
    };

    template <>
    struct meta<RC::UVTD::ClassInheritanceInfoJson> {
        static constexpr auto value = glz::object(
            "parent_class", &RC::UVTD::ClassInheritanceInfoJson::parent_class,
            "version_major", &RC::UVTD::ClassInheritanceInfoJson::version_major,
            "version_minor", &RC::UVTD::ClassInheritanceInfoJson::version_minor,
            "inherit_members", &RC::UVTD::ClassInheritanceInfoJson::inherit_members
        );
    };

    template <>
    struct meta<RC::UVTD::SuffixDefinitionJson> {
        static constexpr auto value = glz::object(
            "suffix", &RC::UVTD::SuffixDefinitionJson::suffix,
            "ifdef_macro", &RC::UVTD::SuffixDefinitionJson::ifdef_macro,
            "description", &RC::UVTD::SuffixDefinitionJson::description,
            "generates_variant", &RC::UVTD::SuffixDefinitionJson::generates_variant
        );
    };

    template <>
    struct meta<RC::UVTD::SuffixDefinitionsFile> {
        static constexpr auto value = glz::object(
            "suffixes", &RC::UVTD::SuffixDefinitionsFile::suffixes
        );
    };
}

namespace RC::UVTD
{
    UVTDConfig& UVTDConfig::Get()
    {
        static UVTDConfig instance;
        return instance;
    }

    bool UVTDConfig::Initialize(const std::filesystem::path& config_dir)
    {
        if (!std::filesystem::exists(config_dir))
        {
            Output::send(STR("Config directory not found: {}\n"), config_dir.wstring());
            return false;
        }

        Output::send(STR("Loading configuration from: {}\n"), config_dir.wstring());
        
        try {
            // Load object_items.json
            std::filesystem::path object_items_path = config_dir / "object_items.json";
            if (std::filesystem::exists(object_items_path))
            {
                std::ifstream file(object_items_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
                auto result = glz::read_json<std::vector<ObjectItemJson>>(json_str);
                if (result.has_value())
                {
                    object_items.clear();
        
                    std::unordered_set<std::string> seen_names;

                    for (const auto& item : result.value())
                    {
                        if (seen_names.insert(item.name).second)
                        {
                            // First time seeing this name, add it
                            object_items.push_back({
                                to_wstring(item.name),
                                item.valid_for_vtable,
                                item.valid_for_member_vars
                            });
                        }
                        else
                        {
                            // Duplicate - find and update existing entry
                            Output::send(STR("Duplicate entry for object item '{}'. Merging properties.\n"), to_wstring(item.name));
        
                            for (auto& existing : object_items)
                            {
                                if (existing.name == to_wstring(item.name))
                                {
                                    if (item.valid_for_vtable == ValidForVTable::Yes)
                                        existing.valid_for_vtable = ValidForVTable::Yes;
                                    if (item.valid_for_member_vars == ValidForMemberVars::Yes)
                                        existing.valid_for_member_vars = ValidForMemberVars::Yes;
                                    break;
                                }
                            }
                        }
                    }
        
                    Output::send(STR("Loaded {} unique object items\n"), object_items.size());
                }
                else
                {
                    Output::send(STR("Failed to parse object_items.json\n"));
                }
            }
            else
            {
                Output::send(STR("object_items.json not found\n"));
            }
            
            // Load private_variables.json
            std::filesystem::path private_vars_path = config_dir / "private_variables.json";
            if (std::filesystem::exists(private_vars_path))
            {
                std::ifstream file(private_vars_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::unordered_map<std::string, std::vector<std::string>>>(json_str);
                if (result.has_value())
                {
                    private_variables.clear();
                    for (const auto& [class_name, vars] : result.value())
                    {
                        std::unordered_set<File::StringType> wide_vars;
                        for (const auto& var : vars)
                        {
                            wide_vars.insert(to_wstring(var));
                        }
                        private_variables[to_wstring(class_name)] = std::move(wide_vars);
                    }
                    Output::send(STR("Loaded private variables for {} classes\n"), private_variables.size());
                }
                else
                {
                    Output::send(STR("Failed to parse private_variables.json\n"));
                }
            }
            else
            {
                Output::send(STR("private_variables.json not found\n"));
            }
            
            // Load types_filter.json
            std::filesystem::path types_filter_path = config_dir / "types_filter.json";
            if (std::filesystem::exists(types_filter_path))
            {
                std::ifstream file(types_filter_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<TypeFilterMapJson>(json_str);
                if (result.has_value())
                {
                    types_to_filter.clear();
                    
                    Output::send(STR("Loaded type filters:\n"));
                    for (const auto& [category_str, type_list] : result.value())
                    {
                        auto category = StringToTypeFilterCategory(category_str);
                        
                        std::vector<File::StringType> wide_types;
                        for (const auto& type : type_list)
                        {
                            wide_types.push_back(to_wstring(type));
                        }
                        
                        types_to_filter[category] = std::move(wide_types);
                        
                        // Print detailed info about each category
                        Output::send(STR("  {} types in '{}' category\n"), 
                                     types_to_filter[category].size(), 
                                     to_wstring(GetReadableCategoryName(category_str)));
                    }
                }
                else
                {
                    Output::send(STR("Failed to parse types_filter.json\n"));
                }
            }
            else
            {
                Output::send(STR("types_filter.json not found\n"));
            }
            
            // Load valid_udt_names.json
            std::filesystem::path udt_path = config_dir / "valid_udt_names.json";
            if (std::filesystem::exists(udt_path))
            {
                std::ifstream file(udt_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::vector<std::string>>(json_str);
                if (result.has_value())
                {
                    valid_udt_names.clear();
                    for (const auto& name : result.value())
                    {
                        valid_udt_names.insert(to_wstring(name));
                    }
                    Output::send(STR("Loaded {} valid UDT names\n"), valid_udt_names.size());
                }
                else
                {
                    Output::send(STR("Failed to parse valid_udt_names.json\n"));
                }
            }
            else
            {
                Output::send(STR("valid_udt_names.json not found\n"));
            }
            
            // Load uprefix_to_fprefix.json
            std::filesystem::path prefix_path = config_dir / "uprefix_to_fprefix.json";
            if (std::filesystem::exists(prefix_path))
            {
                std::ifstream file(prefix_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::vector<std::string>>(json_str);
                if (result.has_value())
                {
                    uprefix_to_fprefix.clear();
                    for (const auto& prefix : result.value())
                    {
                        uprefix_to_fprefix.push_back(to_wstring(prefix));
                    }
                    Output::send(STR("Loaded {} U-prefix to F-prefix conversions\n"), uprefix_to_fprefix.size());
                }
                else
                {
                    Output::send(STR("Failed to parse uprefix_to_fprefix.json\n"));
                }
            }
            else
            {
                Output::send(STR("uprefix_to_fprefix.json not found\n"));
            }
            
            // Load enhanced member_rename_map.json
            std::filesystem::path rename_path = config_dir / "member_rename_map.json";
            if (std::filesystem::exists(rename_path))
            {
                std::ifstream file(rename_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                // Parse the hierarchical structure - outer key is class name, inner key is member name
                using ClassMemberMap = std::unordered_map<std::string, std::unordered_map<std::string, MemberRenameInfoJson>>;
                auto result = glz::read_json<ClassMemberMap>(json_str);
                
                if (result.has_value())
                {
                    member_rename_map.clear();
                    size_t total_entries = 0;
                    
                    for (const auto& [class_name, members] : result.value())
                    {
                        auto class_name_wide = to_wstring(class_name);
                        std::unordered_map<File::StringType, MemberRenameInfo> class_members;
                        
                        for (const auto& [member_name, info] : members)
                        {
                            class_members[to_wstring(member_name)] = {
                                to_wstring(info.mapped_name),
                                info.generate_alias_setter,
                                to_wstring(info.description)
                            };
                            total_entries++;
                        }
                        
                        member_rename_map[class_name_wide] = std::move(class_members);
                    }
                    
                    Output::send(STR("Loaded {} member rename mappings across {} classes\n"), 
                                total_entries, member_rename_map.size());
                }
                else
                {
                    Output::send(STR("Failed to parse member_rename_map.json\n"));
                }
            }
            else
            {
                Output::send(STR("member_rename_map.json not found\n"));
            }
            
            // Load case_preserving_variants.json
            std::filesystem::path variants_path = config_dir / "case_preserving_variants.json";
            if (std::filesystem::exists(variants_path))
            {
                std::ifstream file(variants_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::unordered_map<std::string, std::vector<std::string>>>(json_str);
                if (result.has_value())
                {
                    non_case_preserving_variants.clear();
                    case_preserving_variants.clear();
                    
                    auto it_non = result.value().find("non_case_preserving");
                    if (it_non != result.value().end())
                    {
                        for (const auto& variant : it_non->second)
                        {
                            non_case_preserving_variants.insert(to_wstring(variant));
                        }
                    }
                    
                    auto it_case = result.value().find("case_preserving");
                    if (it_case != result.value().end())
                    {
                        for (const auto& variant : it_case->second)
                        {
                            case_preserving_variants.insert(to_wstring(variant));
                        }
                    }
                    
                    Output::send(STR("Loaded case preserving variants: {} non-preserving, {} preserving\n"), 
                        non_case_preserving_variants.size(), case_preserving_variants.size());
                }
                else
                {
                    Output::send(STR("Failed to parse case_preserving_variants.json\n"));
                }
            }
            else
            {
                Output::send(STR("case_preserving_variants.json not found\n"));
            }
            
            // Load pdbs_to_dump.json
            std::filesystem::path pdbs_path = config_dir / "pdbs_to_dump.json";
            if (std::filesystem::exists(pdbs_path))
            {
                std::ifstream file(pdbs_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::vector<std::string>>(json_str);
                if (result.has_value())
                {
                    pdbs_to_dump.clear();
                    for (const auto& pdb_path : result.value())
                    {
                        pdbs_to_dump.push_back(pdb_path);
                    }
                    Output::send(STR("Loaded {} PDBs to dump\n"), pdbs_to_dump.size());
                }
                else
                {
                    Output::send(STR("Failed to parse pdbs_to_dump.json\n"));
                }
            }
            else
            {
                Output::send(STR("pdbs_to_dump.json not found\n"));
            }
            
            // Load virtual_generator_includes.json
            std::filesystem::path includes_path = config_dir / "virtual_generator_includes.json";
            if (std::filesystem::exists(includes_path))
            {
                std::ifstream file(includes_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::vector<std::string>>(json_str);
                if (result.has_value())
                {
                    virtual_generator_includes.clear();
                    for (const auto& include : result.value())
                    {
                        virtual_generator_includes.push_back(to_wstring(include));
                    }
                    Output::send(STR("Loaded {} virtual generator includes\n"), virtual_generator_includes.size());
                }
                else
                {
                    Output::send(STR("Failed to parse virtual_generator_includes.json\n"));
                }
            }
            else
            {
                Output::send(STR("virtual_generator_includes.json not found\n"));
            }

            // Load class inheritance relationships
            std::filesystem::path inheritance_path = config_dir / "class_inheritance.json";
            if (std::filesystem::exists(inheritance_path))
            {
                std::ifstream file(inheritance_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

                // Parse the hierarchical structure
                using InheritanceMap = std::unordered_map<std::string, ClassInheritanceInfoJson>;
                auto result = glz::read_json<InheritanceMap>(json_str);

                if (result.has_value())
                {
                    class_inheritance_map.clear();
                    for (const auto& [class_name, info] : result.value())
                    {
                        class_inheritance_map[to_wstring(class_name)] = {
                            to_wstring(info.parent_class),
                            info.version_major,
                            info.version_minor,
                            info.inherit_members
                        };
                    }
                    Output::send(STR("Loaded {} class inheritance relationships\n"), class_inheritance_map.size());
                }
                else
                {
                    Output::send(STR("Failed to parse class_inheritance.json\n"));
                }
            }
            else
            {
                Output::send(STR("class_inheritance.json not found\n"));
            }

            // Load suffix definitions
            std::filesystem::path suffix_path = config_dir / "suffix_definitions.json";
            if (std::filesystem::exists(suffix_path))
            {
                std::ifstream file(suffix_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

                auto result = glz::read_json<SuffixDefinitionsFile>(json_str);
                if (result.has_value())
                {
                    suffix_definitions.clear();
                    for (const auto& def : result.value().suffixes)
                    {
                        auto suffix_wide = to_wstring(def.suffix);
                        suffix_definitions[suffix_wide] = {
                            suffix_wide,
                            to_wstring(def.ifdef_macro),
                            to_wstring(def.description),
                            def.generates_variant
                        };
                    }
                    Output::send(STR("Loaded {} suffix definitions\n"), suffix_definitions.size());
                }
                else
                {
                    Output::send(STR("Failed to parse suffix_definitions.json\n"));
                }
            }
            else
            {
                Output::send(STR("suffix_definitions.json not found\n"));
            }

            return true;
        }
        catch (const std::exception& e)
        {
            Output::send(STR("Exception during config initialization: {}\n"), to_wstring(e.what()));
            return false;
        }
    }
} // namespace RC::UVTD