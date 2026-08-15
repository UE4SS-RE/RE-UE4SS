#pragma once

#include <cstdint>
#include <string>

namespace ue4ss::linux::core
{
    struct UnrealScriptSetLayout
    {
        std::int32_t hash_next_id_offset{};
        std::int32_t hash_index_offset{};
        std::int32_t element_stride{};
        std::int32_t sparse_alignment{};
        std::int32_t sparse_stride{};
    };

    struct UnrealScriptMapLayout
    {
        std::int32_t value_offset{};
        UnrealScriptSetLayout set;
    };

    // These calculations are compiled against UEPseudo's actual UE container
    // templates. The dependency-light runtime consumes only the resulting POD
    // layouts and still performs all target-memory access through safe reads.
    [[nodiscard]] bool calculate_script_set_layout(std::int32_t element_size,
                                                   std::int32_t element_alignment,
                                                   UnrealScriptSetLayout& output,
                                                   std::string& error) noexcept;
    [[nodiscard]] bool calculate_script_map_layout(std::int32_t key_size,
                                                   std::int32_t key_alignment,
                                                   std::int32_t value_size,
                                                   std::int32_t value_alignment,
                                                   UnrealScriptMapLayout& output,
                                                   std::string& error) noexcept;
    [[nodiscard]] std::uint32_t script_set_storage_size() noexcept;
    [[nodiscard]] std::uint32_t script_map_storage_size() noexcept;
    [[nodiscard]] std::uint32_t script_set_hash_size(std::uint32_t element_count) noexcept;
}
