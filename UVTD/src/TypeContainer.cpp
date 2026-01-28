#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    // Helper to check if two types are meaningfully different
    // Ignores whitespace differences and normalizes some common variations
    static auto types_are_different(const File::StringType& type1, const File::StringType& type2) -> bool
    {
        if (type1 == type2) return false;

        // TODO: Add more sophisticated type comparison if needed
        // For now, simple string comparison
        return true;
    }

    auto TypeContainer::join(const TypeContainer& other) -> void
    {
        // Backward compatibility - join without version tracking
        for (const auto& [_, class_entry] : other.class_entries)
        {
            SymbolNameInfo name_info = SymbolNameInfo{class_entry.valid_for_vtable, class_entry.valid_for_member_vars};
            Class& this_entry = get_or_create_class_entry(class_entry.class_name, class_entry.class_name_clean, name_info);

            for (const auto& [vtable_offset, function] : class_entry.functions)
            {
                this_entry.functions[vtable_offset] = function;
            }

            for (const auto& variable : class_entry.variables)
            {
                auto existing = std::find_if(this_entry.variables.begin(), this_entry.variables.end(),
                    [&variable](const MemberVariable& var) {
                        return var.name == variable.name;
                    });

                if (existing != this_entry.variables.end())
                {
                    *existing = variable;
                }
                else
                {
                    this_entry.variables.push_back(variable);
                }
            }
        }

        // Copy source PDB info if we don't have one and other does
        if (!m_source_pdb_info.has_value() && other.m_source_pdb_info.has_value())
        {
            m_source_pdb_info = other.m_source_pdb_info;
        }
    }

    auto TypeContainer::join(const TypeContainer& other, const PDBNameInfo& other_pdb_info) -> void
    {
        const File::StringType other_version_key = other_pdb_info.base_version;

        for (const auto& [_, class_entry] : other.class_entries)
        {
            SymbolNameInfo name_info = SymbolNameInfo{class_entry.valid_for_vtable, class_entry.valid_for_member_vars};
            Class& this_entry = get_or_create_class_entry(class_entry.class_name, class_entry.class_name_clean, name_info);

            for (const auto& [vtable_offset, function] : class_entry.functions)
            {
                this_entry.functions[vtable_offset] = function;
            }

            for (const auto& variable : class_entry.variables)
            {
                auto existing = std::find_if(this_entry.variables.begin(), this_entry.variables.end(),
                    [&variable](const MemberVariable& var) {
                        return var.name == variable.name;
                    });

                if (existing != this_entry.variables.end())
                {
                    // Variable exists - check if type changed
                    if (types_are_different(existing->type, variable.type))
                    {
                        // Type changed between versions!
                        Output::send(STR("  Type change detected for {}::{}: '{}' -> '{}'\n"),
                                     class_entry.class_name, variable.name,
                                     existing->type, variable.type);

                        // If this is the first type change detected, also record the original type
                        if (existing->types_by_version.empty() && m_source_pdb_info.has_value())
                        {
                            VersionedType original_type;
                            original_type.type = existing->type;
                            original_type.size = existing->size;
                            original_type.major_version = m_source_pdb_info->major_version;
                            original_type.minor_version = m_source_pdb_info->minor_version;
                            original_type.is_bitfield = existing->is_bitfield;
                            original_type.bit_position = existing->bit_position;
                            original_type.bit_length = existing->bit_length;
                            existing->types_by_version[m_source_pdb_info->base_version] = original_type;
                        }

                        // Record the new type
                        VersionedType new_type;
                        new_type.type = variable.type;
                        new_type.size = variable.size;
                        new_type.major_version = other_pdb_info.major_version;
                        new_type.minor_version = other_pdb_info.minor_version;
                        new_type.is_bitfield = variable.is_bitfield;
                        new_type.bit_position = variable.bit_position;
                        new_type.bit_length = variable.bit_length;
                        existing->types_by_version[other_version_key] = new_type;

                        // Update the "current" type to the newer one
                        existing->type = variable.type;
                        existing->size = variable.size;
                        existing->is_bitfield = variable.is_bitfield;
                        existing->bit_position = variable.bit_position;
                        existing->bit_length = variable.bit_length;
                    }
                    else
                    {
                        // Same type - just update offset and bitfield info if needed
                        existing->offset = variable.offset;
                        existing->is_bitfield = variable.is_bitfield;
                        existing->bit_position = variable.bit_position;
                        existing->bit_length = variable.bit_length;
                    }
                }
                else
                {
                    // New variable - add it
                    this_entry.variables.push_back(variable);
                }
            }
        }

        // Update source PDB info to track earliest version
        if (!m_source_pdb_info.has_value())
        {
            m_source_pdb_info = other_pdb_info;
        }
        else if (other_pdb_info.major_version < m_source_pdb_info->major_version ||
                 (other_pdb_info.major_version == m_source_pdb_info->major_version &&
                  other_pdb_info.minor_version < m_source_pdb_info->minor_version))
        {
            m_source_pdb_info = other_pdb_info;
        }
    }

    auto TypeContainer::get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info)
            -> Class&
    {
        auto& class_entry = [&]() -> auto&
        {
            if (auto it = class_entries.find(symbol_name); it != class_entries.end())
            {
                return it->second;
            }
            else
            {
                return class_entries.emplace(symbol_name_clean, Class{.class_name = File::StringType{symbol_name}, .class_name_clean = symbol_name_clean})
                        .first->second;
            }
        }
        ();

        class_entry.valid_for_member_vars = name_info.valid_for_member_vars;
        class_entry.valid_for_vtable = name_info.valid_for_vtable;
        return class_entry;
    }
} // namespace RC::UVTD