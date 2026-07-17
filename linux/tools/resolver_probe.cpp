#include "ue4ss/elf_image.hpp"
#include "ue4ss/patternsleuth_abi.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace
{
    struct Capability
    {
        std::uint64_t mask;
        const char* name;
        std::uint64_t address;
        bool executable;
        bool critical;
    };

    void resolver_log(std::uint32_t level, const std::uint8_t* message, std::uintptr_t length, void*)
    {
        const char* label = level == UE4SS_PS_LOG_ERROR ? "error" : level == UE4SS_PS_LOG_WARNING ? "warning" : "info";
        std::cerr << "[patternsleuth-offline] [" << label << "] "
                  << std::string_view{reinterpret_cast<const char*>(message), length} << '\n';
    }

    [[nodiscard]] std::string json_escape(std::string_view input)
    {
        std::string output;
        for (const char character : input)
        {
            if (character == '"' || character == '\\')
            {
                output.push_back('\\');
            }
            output.push_back(character);
        }
        return output;
    }

    [[nodiscard]] const ue4ss::linux::Segment* containing_segment(const ue4ss::linux::ElfImage& image,
                                                                   std::uint64_t address,
                                                                   bool executable)
    {
        for (const auto& segment : image.segments)
        {
            if (address >= segment.address && address < segment.address + segment.memory_size && segment.readable &&
                (!executable || segment.executable))
            {
                return &segment;
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::string hex(std::uint64_t value)
    {
        std::ostringstream output;
        output << "0x" << std::hex << value;
        return output.str();
    }
} // namespace

int main(int argc, char** argv)
{
    bool json{};
    bool require_critical{};
    std::string path;
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument{argv[index]};
        if (argument == "--json")
        {
            json = true;
        }
        else if (argument == "--require-critical")
        {
            require_critical = true;
        }
        else if (path.empty())
        {
            path = argument;
        }
        else
        {
            std::cerr << "usage: ue4ss_resolver_probe [--json] [--require-critical] <elf-image>\n";
            return 64;
        }
    }
    if (path.empty())
    {
        std::cerr << "usage: ue4ss_resolver_probe [--json] [--require-critical] <elf-image>\n";
        return 64;
    }

    try
    {
        const auto image = ue4ss::linux::inspect_elf_file(path);
        constexpr std::uint64_t enabled = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING | UE4SS_PS_FNAME_CTOR | UE4SS_PS_GMALLOC |
                                          UE4SS_PS_STATIC_CONSTRUCT_OBJECT | UE4SS_PS_STATIC_FIND_OBJECT | UE4SS_PS_GAME_ENGINE_TICK |
                                          UE4SS_PS_ENGINE_LOOP_INIT | UE4SS_PS_UFUNCTION_BIND | UE4SS_PS_ALLOCATE_UOBJECT_INDEX |
                                          UE4SS_PS_FREE_UOBJECT_INDEX | UE4SS_PS_ENGINE_VERSION;
        ue4ss_ps_linux_config config{
                .struct_size = sizeof(config),
                .abi_version = UE4SS_PS_LINUX_ABI_VERSION,
                .enabled_mask = enabled,
                .log = resolver_log,
                .user_data = nullptr,
        };
        ue4ss_ps_linux_results results{};
        results.struct_size = sizeof(results);
        const int scan_error = ps_scan_linux_file_v1(path.c_str(), &config, &results);

        const std::array capabilities{
                Capability{UE4SS_PS_GUOBJECT_ARRAY, "object_registry", results.guobject_array, false, true},
                Capability{UE4SS_PS_FNAME_TO_STRING, "fname_to_string", results.fname_to_string, true, true},
                Capability{UE4SS_PS_FNAME_CTOR, "fname_constructor", results.fname_ctor, true, true},
                Capability{UE4SS_PS_GMALLOC, "gmalloc", results.gmalloc, false, true},
                Capability{UE4SS_PS_STATIC_CONSTRUCT_OBJECT, "static_construct_object", results.static_construct_object, true, true},
                Capability{UE4SS_PS_STATIC_FIND_OBJECT, "static_find_object", results.static_find_object, true, true},
                Capability{UE4SS_PS_GAME_ENGINE_TICK, "game_thread_tick", results.game_engine_tick, true, true},
                Capability{UE4SS_PS_ENGINE_LOOP_INIT, "engine_loop_init", results.engine_loop_init, true, false},
                Capability{UE4SS_PS_UFUNCTION_BIND, "ufunction_bind", results.ufunction_bind, true, false},
                Capability{UE4SS_PS_ALLOCATE_UOBJECT_INDEX, "notify_new_object", results.allocate_uobject_index, true, false},
                Capability{UE4SS_PS_FREE_UOBJECT_INDEX, "notify_delete_object", results.free_uobject_index, true, false},
        };

        bool critical_available = scan_error == 0;
        for (const auto& capability : capabilities)
        {
            const bool available = (results.available_mask & capability.mask) != 0 && capability.address != 0 &&
                                   containing_segment(image, capability.address, capability.executable) != nullptr;
            if (capability.critical)
            {
                critical_available = critical_available && available;
            }
        }

        if (json)
        {
            std::cout << "{\n"
                      << "  \"path\": \"" << json_escape(image.path) << "\",\n"
                      << "  \"sha256\": \"" << image.sha256 << "\",\n"
                      << "  \"build_id\": \"" << image.build_id << "\",\n"
                      << "  \"scan_error\": " << scan_error << ",\n"
                      << "  \"engine_version\": \"" << results.engine_major << '.' << results.engine_minor << "\",\n"
                      << "  \"available_mask\": \"" << hex(results.available_mask) << "\",\n"
                      << "  \"failed_mask\": \"" << hex(results.failed_mask) << "\",\n"
                      << "  \"critical_available\": " << (critical_available ? "true" : "false") << ",\n"
                      << "  \"capabilities\": {\n";
            for (std::size_t index = 0; index < capabilities.size(); ++index)
            {
                const auto& capability = capabilities[index];
                const auto* segment = containing_segment(image, capability.address, capability.executable);
                const bool available = (results.available_mask & capability.mask) != 0 && capability.address != 0 && segment != nullptr;
                std::cout << "    \"" << capability.name << "\": {\"state\": \"" << (available ? "available" : "unavailable")
                          << "\", \"rva\": \"" << hex(capability.address) << "\", \"segment_valid\": "
                          << (segment != nullptr ? "true" : "false") << '}' << (index + 1u == capabilities.size() ? "\n" : ",\n");
            }
            std::cout << "  }\n}\n";
        }
        else
        {
            std::cout << "ELF build-id: " << image.build_id << "\nSHA-256: " << image.sha256 << "\nEngine: " << results.engine_major << '.'
                      << results.engine_minor << "\nCritical resolver gate: " << (critical_available ? "available" : "unavailable") << '\n';
            for (const auto& capability : capabilities)
            {
                const bool available = (results.available_mask & capability.mask) != 0 && capability.address != 0 &&
                                       containing_segment(image, capability.address, capability.executable) != nullptr;
                std::cout << capability.name << ": " << (available ? hex(capability.address) : "unavailable") << '\n';
            }
        }

        if (scan_error != 0)
        {
            return 65;
        }
        return require_critical && !critical_available ? 2 : 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ue4ss_resolver_probe: " << error.what() << '\n';
        return 65;
    }
}
