#pragma once

#include <UVTD/Config.hpp>
#include <UVTD/PDBNameInfo.hpp>

namespace RC::UVTD
{
    /**
     * Utility namespace that provides easy access to configuration values.
     * This replaces the static variables previously defined in Helpers.hpp
     */
    namespace ConfigUtil
    {
        // Object items access
        inline const std::vector<ObjectItem>& GetObjectItems() 
        {
            return UVTDConfig::Get().object_items;
        }

        // Private variables access
        inline const std::unordered_map<File::StringType, std::unordered_set<File::StringType>>& GetPrivateVariables() 
        {
            return UVTDConfig::Get().private_variables;
        }

        // Get types to filter by category
        inline const std::vector<File::StringType>& GetTypesToFilterByCategory(TypeFilterCategory category) 
        {
            static const std::vector<File::StringType> empty_vector;
            const auto& filter_map = UVTDConfig::Get().types_to_filter;
            auto it = filter_map.find(category);
            if (it != filter_map.end()) {
                return it->second;
            }
            return empty_vector;
        }

        // Check if type should be filtered out based on category
        inline bool ShouldFilterType(const File::StringType& type_name, TypeFilterCategory category)
        {
            const auto& types_to_filter = GetTypesToFilterByCategory(category);
            for (const auto& filter : types_to_filter) {
                if (type_name.find(filter) != type_name.npos) {
                    return true;
                }
            }
            return false;
        }

        // Valid UDT names access
        inline const std::unordered_set<File::StringType>& GetValidUDTNames() 
        {
            return UVTDConfig::Get().valid_udt_names;
        }

        // UPrefix to FPrefix access
        inline const std::vector<File::StringType>& GetUPrefixToFPrefix() 
        {
            return UVTDConfig::Get().uprefix_to_fprefix;
        }

        // Enhanced member rename map access
        inline const std::unordered_map<File::StringType, std::unordered_map<File::StringType, MemberRenameInfo>>& GetMemberRenameMap() 
        {
            return UVTDConfig::Get().member_rename_map;
        }
        
        // Helper to get member renamed info for a variable in a specific class
        inline std::optional<MemberRenameInfo> GetMemberRenameInfo(
            const File::StringType& class_name, 
            const File::StringType& member_name)
        {
            const auto& map = UVTDConfig::Get().member_rename_map;
            
            // First check class-specific mapping
            auto class_it = map.find(class_name);
            if (class_it != map.end()) {
                auto member_it = class_it->second.find(member_name);
                if (member_it != class_it->second.end()) {
                    return member_it->second;
                }
            }
            
            // Then check global mapping (using "Global" as the class name)
            auto global_it = map.find(STR("Global"));
            if (global_it != map.end()) {
                auto member_it = global_it->second.find(member_name);
                if (member_it != global_it->second.end()) {
                    return member_it->second;
                }
            }
            
            return std::nullopt;
        }
        
        // Helper to get mapped name for a variable
        inline File::StringType GetMappedName(
            const File::StringType& class_name,
            const File::StringType& original_name)
        {
            auto info = GetMemberRenameInfo(class_name, original_name);
            if (info.has_value()) {
                return info->mapped_name;
            }
            return original_name;
        }

        // PDBs to dump access
        inline const std::vector<std::filesystem::path>& GetPDBsToDump() 
        {
            return UVTDConfig::Get().pdbs_to_dump;
        }
        
        // Virtual generator includes access
        inline const std::vector<File::StringType>& GetVirtualGeneratorIncludes() 
        {
            return UVTDConfig::Get().virtual_generator_includes;
        }

        // Class inheritance relationships access
        inline const std::unordered_map<File::StringType, ClassInheritanceInfo>& GetClassInheritanceMap() 
        {
            return UVTDConfig::Get().class_inheritance_map;
        }

        // Helper to get class inheritance info
        inline std::optional<ClassInheritanceInfo> GetClassInheritance(const File::StringType& class_name)
        {
            const auto& map = UVTDConfig::Get().class_inheritance_map;
            auto it = map.find(class_name);
            if (it != map.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        // Suffix definitions access
        inline const std::unordered_map<File::StringType, SuffixDefinition>& GetSuffixDefinitions()
        {
            return UVTDConfig::Get().suffix_definitions;
        }

        // Get suffix definition for a specific suffix name
        inline std::optional<SuffixDefinition> GetSuffixDefinition(const File::StringType& suffix)
        {
            const auto& map = UVTDConfig::Get().suffix_definitions;
            auto it = map.find(suffix);
            if (it != map.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        // Get the ifdef macro for a suffix (returns auto-generated macro if not in config)
        inline File::StringType GetSuffixIfdefMacro(const File::StringType& suffix)
        {
            auto def = GetSuffixDefinition(suffix);
            if (def.has_value()) {
                return def->ifdef_macro;
            }
            // Fall back to auto-generated macro from PDBNameInfo
            return PDBNameInfo::suffix_to_macro(suffix);
        }

        // Check if a specific suffix variant exists for a base version
        // Scans pdbs_to_dump looking for base_version_suffix pattern
        inline bool HasSuffixVariant(const File::StringType& base_version, const File::StringType& suffix)
        {
            // Look for a PDB named base_version_suffix (e.g., "4_27_CasePreserving")
            File::StringType target = base_version + STR("_") + suffix;

            for (const auto& pdb_path : GetPDBsToDump())
            {
                File::StringType pdb_stem = pdb_path.filename().stem().wstring();
                if (pdb_stem == target)
                {
                    return true;
                }
            }
            return false;
        }
    }
}