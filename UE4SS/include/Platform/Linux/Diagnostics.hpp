#pragma once

#ifdef __linux__

#include <filesystem>
#include <string_view>

namespace RC::LinuxDiagnostics
{
    auto is_enabled() -> bool;
    auto output_environment(const std::filesystem::path& executable) -> void;
    auto output_inactive_reason(std::string_view reason) -> void;
} // namespace RC::LinuxDiagnostics

#endif
