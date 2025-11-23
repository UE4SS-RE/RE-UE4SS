#pragma once

#include <UVTD/Config.hpp>

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

        // Types not to dump access
        inline const std::vector<File::StringType>& GetTypesNotToDump() 
        {
            return UVTDConfig::Get().types_to_not_dump;
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

        // Member rename map access
        inline const std::unordered_map<File::StringType, File::StringType>& GetMemberRenameMap() 
        {
            return UVTDConfig::Get().member_rename_map;
        }

        // Case preserving variants access
        inline const std::unordered_set<File::StringType>& GetNonCasePreservingVariants() 
        {
            return UVTDConfig::Get().non_case_preserving_variants;
        }

        // Case preserving variants access
        inline const std::unordered_set<File::StringType>& GetCasePreservingVariants() 
        {
            return UVTDConfig::Get().case_preserving_variants;
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

        // Helper methods
        inline bool IsNonCasePreservingVariant(const File::StringType& pdb_name) 
        {
            return GetNonCasePreservingVariants().find(pdb_name) != GetNonCasePreservingVariants().end();
        }

        inline bool IsCasePreservingVariant(const File::StringType& pdb_name) 
        {
            return GetCasePreservingVariants().find(pdb_name) != GetCasePreservingVariants().end();
        }
    }
}