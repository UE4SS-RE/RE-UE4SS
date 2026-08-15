#pragma once

#include "status.hpp"

#include <cstdint>
#include <vector>

namespace ue4ss::linux::core
{
    struct ResolvedUnrealState
    {
        std::uint64_t available_mask{};
        std::uint16_t engine_major{};
        std::uint16_t engine_minor{};
        std::uint64_t guobject_array{};
        std::uint64_t fname_to_string{};
        std::uint64_t fname_ctor{};
        std::uint64_t gmalloc{};
        std::uint64_t static_construct_object{};
        std::uint64_t static_find_object{};
        std::uint64_t game_engine_tick{};
        std::uint64_t engine_loop_init{};
        std::uint64_t ufunction_bind{};
        std::uint64_t allocate_uobject_index{};
        std::uint64_t free_uobject_index{};
    };

    struct PatternReport
    {
        std::vector<Capability> capabilities;
        ResolvedUnrealState resolved;
        bool scan_succeeded{};
        bool critical_capabilities_available{};
    };

    [[nodiscard]] PatternReport scan_unreal_patterns(RuntimeData& runtime);
} // namespace ue4ss::linux::core
