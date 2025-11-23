#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>

namespace RC::UVTD
{
    struct ObjectItem
    {
        File::StringType name{};
        ValidForVTable valid_for_vtable{};
        ValidForMemberVars valid_for_member_vars{};
    };

    class UVTDConfig
    {
    private:
        static constexpr const char* DEFAULT_CONFIG_PATH = "Config";
        
    public:
        // Static instance for singleton access
        static UVTDConfig& Get();

        // Initialize config by loading from files
        bool Initialize(const std::filesystem::path& config_dir = DEFAULT_CONFIG_PATH);

        // Reload configuration from files
        bool Reload(const std::filesystem::path& config_dir = DEFAULT_CONFIG_PATH);

        // Configurations loaded from JSON
        std::vector<ObjectItem> object_items;
        std::unordered_map<File::StringType, std::unordered_set<File::StringType>> private_variables;
        std::vector<File::StringType> types_to_not_dump;
        std::unordered_set<File::StringType> valid_udt_names;
        std::vector<File::StringType> uprefix_to_fprefix;
        std::unordered_map<File::StringType, File::StringType> member_rename_map;
        std::unordered_set<File::StringType> non_case_preserving_variants;
        std::unordered_set<File::StringType> case_preserving_variants;
        std::vector<std::filesystem::path> pdbs_to_dump;
        std::vector<File::StringType> virtual_generator_includes;

    private:
        UVTDConfig() = default;
        ~UVTDConfig() = default;
        UVTDConfig(const UVTDConfig&) = delete;
        UVTDConfig& operator=(const UVTDConfig&) = delete;
    };
} // namespace RC::UVTD