#include <fstream>
#include <format>
#include <iostream>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <UVTD/Config.hpp>

#include <glaze/glaze.hpp>

namespace RC::UVTD
{
    // Helper struct for JSON parsing
    struct ObjectItemJson {
        std::string name;
        ValidForVTable valid_for_vtable;
        ValidForMemberVars valid_for_member_vars;
    };
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
                    for (const auto& item : result.value())
                    {
                        object_items.push_back({
                            to_wstring(item.name),
                            item.valid_for_vtable,
                            item.valid_for_member_vars
                        });
                    }
                    Output::send(STR("Loaded {} object items\n"), object_items.size());
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
            
            // Load types_not_to_dump.json
            std::filesystem::path types_path = config_dir / "types_not_to_dump.json";
            if (std::filesystem::exists(types_path))
            {
                std::ifstream file(types_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::vector<std::string>>(json_str);
                if (result.has_value())
                {
                    types_to_not_dump.clear();
                    for (const auto& type : result.value())
                    {
                        types_to_not_dump.push_back(to_wstring(type));
                    }
                    Output::send(STR("Loaded {} types not to dump\n"), types_to_not_dump.size());
                }
                else
                {
                    Output::send(STR("Failed to parse types_not_to_dump.json\n"));
                }
            }
            else
            {
                Output::send(STR("types_not_to_dump.json not found\n"));
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
            
            // Load member_rename_map.json
            std::filesystem::path rename_path = config_dir / "member_rename_map.json";
            if (std::filesystem::exists(rename_path))
            {
                std::ifstream file(rename_path);
                std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                auto result = glz::read_json<std::unordered_map<std::string, std::string>>(json_str);
                if (result.has_value())
                {
                    member_rename_map.clear();
                    for (const auto& [orig, renamed] : result.value())
                    {
                        member_rename_map[to_wstring(orig)] = to_wstring(renamed);
                    }
                    Output::send(STR("Loaded {} member rename mappings\n"), member_rename_map.size());
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
            
            return true;
        }
        catch (const std::exception& e)
        {
            Output::send(STR("Exception during config initialization: {}\n"), to_wstring(e.what()));
            return false;
        }
    }
} // namespace RC::UVTD