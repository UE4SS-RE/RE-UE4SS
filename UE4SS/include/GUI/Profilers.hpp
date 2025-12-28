#pragma once

// Automatically enable the profiler tab when profilers are enabled
#ifdef UE4SS_PROFILERS
#define UE4SS_PROFILER_TAB 1
#endif

#ifdef UE4SS_PROFILER_TAB

namespace RC::GUI::Profilers
{
    auto render() -> void;
} // namespace RC::GUI::Profilers

#endif // UE4SS_PROFILER_TAB