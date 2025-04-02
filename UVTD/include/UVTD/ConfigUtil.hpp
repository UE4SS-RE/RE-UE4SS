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

        // Get types to filter by category
        inline const std::vector<File::StringType>& GetTypesToFilterByCategory(TypeFilterCategory category)
        {
            static const std::vector<File::StringType> empty_vector;
            const auto& filter_map = UVTDConfig::Get().types_to_filter;
            auto it = filter_map.find(category);
            if (it != filter_map.end())
            {
                return it->second;
            }
            return empty_vector;
        }

        // Check if type should be filtered out based on category
        inline bool ShouldFilterType(const File::StringType& type_name, TypeFilterCategory category)
        {
            const auto& types_to_filter = GetTypesToFilterByCategory(category);
            for (const auto& filter : types_to_filter)
            {
                if (type_name.find(filter) != type_name.npos)
                {
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
            if (class_it != map.end())
            {
                auto member_it = class_it->second.find(member_name);
                if (member_it != class_it->second.end())
                {
                    return member_it->second;
                }
            }

            // Then check global mapping (using "Global" as the class name)
            auto global_it = map.find(STR("Global"));
            if (global_it != map.end())
            {
                auto member_it = global_it->second.find(member_name);
                if (member_it != global_it->second.end())
                {
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
            if (info.has_value())
            {
                return info->mapped_name;
            }
            return original_name;
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
            if (it != map.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        // Get method overload mappings for a specific class and method
        inline std::optional<File::StringType> GetMethodOverloadName(
                const File::StringType& class_name,
                const File::StringType& method_name,
                const File::StringType& return_type,
                const std::vector<File::StringType>& param_types,
                bool is_const
                )
        {
            const auto& method_overload_mappings = UVTDConfig::Get().method_overload_mappings;

            // First, try class-specific mapping
            auto class_it = method_overload_mappings.find(class_name);
            if (class_it != method_overload_mappings.end())
            {
                auto method_it = class_it->second.find(method_name);
                if (method_it != class_it->second.end())
                {
                    for (const auto& overload : method_it->second)
                    {
                        if (overload.MatchesSignature(return_type, param_types, is_const))
                        {
                            return overload.mapped_name;
                        }
                    }
                }
            }

            // If no class-specific mapping, try global mapping (using a "Global" class key)
            auto global_it = method_overload_mappings.find(STR("Global"));
            if (global_it != method_overload_mappings.end())
            {
                auto method_it = global_it->second.find(method_name);
                if (method_it != global_it->second.end())
                {
                    for (const auto& overload : method_it->second)
                    {
                        if (overload.MatchesSignature(return_type, param_types, is_const))
                        {
                            return overload.mapped_name;
                        }
                    }
                }
            }

            // No mapping found
            return std::nullopt;
        }
    }
} // namespace RC::UVTD