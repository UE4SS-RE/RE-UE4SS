#pragma once

#include "ue4ss/abi.h"
#include "ue4ss/elf_image.hpp"

#include <string>
#include <vector>

namespace ue4ss::linux::core
{
    enum class CapabilityState
    {
        available,
        unavailable,
        disabled,
    };

    struct Capability
    {
        std::string name;
        CapabilityState state{CapabilityState::unavailable};
        std::string detail;
    };

    struct RuntimeData
    {
        ue4ss_runtime_state state{UE4SS_STATE_NOT_STARTED};
        int last_error{};
        bool required{};
        std::string root_path;
        std::string state_path;
        std::string executable_path;
        std::string message{"not started"};
        ElfImage image;
        std::vector<Capability> capabilities;
    };

    void log_line(const RuntimeData& runtime, const std::string& level, const std::string& message);
    void write_status_file(const RuntimeData& runtime);
    void copy_public_status(const RuntimeData& runtime, ue4ss_status& status);
} // namespace ue4ss::linux::core
