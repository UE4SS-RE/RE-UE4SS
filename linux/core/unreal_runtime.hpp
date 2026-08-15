#pragma once

#include "pattern_resolver.hpp"

#include <string>

namespace ue4ss::linux::core
{
    // Applies resolver results to dependency-light UEPseudo state only. It
    // does not dereference game memory and does not install any hooks.
    [[nodiscard]] bool apply_unreal_resolver_state(const ResolvedUnrealState& resolved, std::string& error) noexcept;
}
