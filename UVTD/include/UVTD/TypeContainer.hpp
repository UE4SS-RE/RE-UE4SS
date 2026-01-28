#pragma once

#include <unordered_map>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>
#include <UVTD/PDBNameInfo.hpp>

namespace RC::UVTD
{
    class TypeContainer
    {
      private:
        using ClassEntries = std::unordered_map<File::StringType, Class>;

        ClassEntries class_entries;

        // Track the version of the first PDB that populated this container
        // Used for detecting type changes during join
        std::optional<PDBNameInfo> m_source_pdb_info{};

      public:
        // Join without version tracking (backward compatibility)
        auto join(const TypeContainer& other) -> void;

        // Join with version tracking - detects type changes between versions
        auto join(const TypeContainer& other, const PDBNameInfo& other_pdb_info) -> void;

        // Set the source PDB info for this container
        auto set_source_pdb_info(const PDBNameInfo& info) -> void { m_source_pdb_info = info; }
        auto get_source_pdb_info() const -> const std::optional<PDBNameInfo>& { return m_source_pdb_info; }

      public:
        constexpr auto get_class_entries() const -> const ClassEntries&
        {
            return class_entries;
        }

        auto get_class_entries() -> ClassEntries&
        {
            return class_entries;
        }

      public:
        auto get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&;
    };
} // namespace RC::UVTD