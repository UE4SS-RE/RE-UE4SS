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

    // Enhanced member rename structure
    struct MemberRenameInfo
    {
        File::StringType mapped_name{};
        File::StringType mapped_type_name{};
        bool generate_alias_setter{false};
        File::StringType description{};
    };

    struct ClassInheritanceInfo
    {
        File::StringType parent_class{};
        int32_t version_major{};
        int32_t version_minor{};
        bool inherit_members{true};
    };

    // Type filtering categories
    enum class TypeFilterCategory
    {
        CompleteExclusion,     // Completely exclude from parsing
        ExcludeFromGetters,    // Exclude from getter generation
        ExcludeFromSolBindings // Exclude from Sol bindings
    };

    // Suffix definition for PDB naming - maps suffix to ifdef macro and description
    struct SuffixDefinition
    {
        File::StringType suffix{};           // e.g., "CasePreserving"
        File::StringType ifdef_macro{};      // e.g., "WITH_CASE_PRESERVING_NAME"
        File::StringType description{};      // Human-readable description
        bool generates_variant{true};        // Whether this suffix creates a variant build
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

        // Configurations loaded from JSON
        std::vector<ObjectItem> object_items;
        std::unordered_map<File::StringType, std::unordered_set<File::StringType>> private_variables;
        
        // Enhanced type filtering configuration
        std::unordered_map<TypeFilterCategory, std::vector<File::StringType>> types_to_filter;
        
        std::unordered_set<File::StringType> valid_udt_names;
        std::vector<File::StringType> uprefix_to_fprefix;

        // Maps a PDB class name onto the name it should be emitted as. Used where the engine renamed
        // a class outright rather than just swapping its prefix, e.g. UAssetClassProperty became
        // USoftClassProperty in 4.18 with an identical layout. Emitting the old class under the new
        // name keeps a single set of member offsets covering every engine version, so consumers never
        // have to branch on which name the running engine happens to use.
        std::unordered_map<File::StringType, File::StringType> class_rename_map;
        
        // Enhanced member rename map - outer key is class name, inner key is member name
        std::unordered_map<File::StringType, std::unordered_map<File::StringType, MemberRenameInfo>> member_rename_map;
        
        std::vector<std::filesystem::path> pdbs_to_dump;
        std::vector<File::StringType> virtual_generator_includes;

        // Class inheritance relationships
        std::unordered_map<File::StringType, ClassInheritanceInfo> class_inheritance_map;

        // Suffix definitions - maps suffix name to its configuration
        std::unordered_map<File::StringType, SuffixDefinition> suffix_definitions;

    private:
        UVTDConfig() = default;
        ~UVTDConfig() = default;
        UVTDConfig(const UVTDConfig&) = delete;
        UVTDConfig& operator=(const UVTDConfig&) = delete;
    };
} // namespace RC::UVTD