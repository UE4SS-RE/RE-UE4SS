#pragma once

#include <cstdint>
#include <string>

namespace ue4ss::linux::core
{
    enum class ObjectRegistryValidationState
    {
        ready,
        not_initialized,
        invalid,
        unsupported,
    };

    struct ObjectRegistryValidation
    {
        ObjectRegistryValidationState state{ObjectRegistryValidationState::invalid};
        std::int32_t num_elements{};
        std::int32_t num_chunks{};
        std::int32_t validated_objects{};
        std::string detail;
    };

    // Reads through process_vm_readv so invalid resolver output becomes a
    // validation failure instead of a SIGSEGV in the server process.
    [[nodiscard]] ObjectRegistryValidation validate_object_registry(std::uint64_t address,
                                                                    std::uint16_t engine_major,
                                                                    std::uint16_t engine_minor) noexcept;
}
