#include "pattern_resolver.hpp"

#if UE4SS_WITH_PATTERNSLEUTH
#include "ue4ss/patternsleuth_abi.h"
#endif

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
    using ue4ss::linux::Segment;
    using ue4ss::linux::core::Capability;
    using ue4ss::linux::core::CapabilityState;
    using ue4ss::linux::core::RuntimeData;

    [[nodiscard]] bool address_in_segment(const RuntimeData& runtime, std::uint64_t address, bool executable)
    {
        return std::ranges::any_of(runtime.image.segments, [address, executable](const Segment& segment) {
            if (address < segment.address || address >= segment.address + segment.memory_size)
            {
                return false;
            }
            return segment.readable && (!executable || segment.executable);
        });
    }

    [[nodiscard]] std::string address_detail(const RuntimeData& runtime, std::uint64_t address)
    {
        const auto module = std::ranges::find_if(runtime.image.modules, [&runtime](const auto& candidate) {
            return candidate.path == runtime.image.path;
        });
        const std::uintptr_t load_bias = module == runtime.image.modules.end() ? 0u : module->load_bias;
        std::ostringstream detail;
        if (address >= load_bias)
        {
            detail << "validated at RVA 0x" << std::hex << (address - load_bias);
        }
        else
        {
            detail << "validated at runtime address 0x" << std::hex << address;
        }
        return detail.str();
    }

#if UE4SS_WITH_PATTERNSLEUTH
    void resolver_log(std::uint32_t level, const std::uint8_t* message, std::uintptr_t length, void* user_data) noexcept
    {
        try
        {
            auto& runtime = *static_cast<RuntimeData*>(user_data);
            const std::string text{reinterpret_cast<const char*>(message), length};
            const std::string log_level = level == UE4SS_PS_LOG_ERROR ? "error" : level == UE4SS_PS_LOG_WARNING ? "warning" : "info";
            ue4ss::linux::core::log_line(runtime, log_level, text);
        }
        catch (...)
        {
        }
    }

    void add_address_capability(ue4ss::linux::core::PatternReport& report,
                                const RuntimeData& runtime,
                                const ue4ss_ps_linux_results& results,
                                std::uint64_t mask,
                                std::uint64_t address,
                                std::string name,
                                bool executable)
    {
        if ((results.available_mask & mask) == 0u)
        {
            report.capabilities.push_back({std::move(name), CapabilityState::unavailable, "resolver returned no unique result"});
            return;
        }
        if (address == 0u || !address_in_segment(runtime, address, executable))
        {
            report.capabilities.push_back({std::move(name), CapabilityState::unavailable, "resolver result failed ELF segment validation"});
            return;
        }
        report.capabilities.push_back({std::move(name), CapabilityState::available, address_detail(runtime, address)});
    }
#endif
} // namespace

namespace ue4ss::linux::core
{
    PatternReport scan_unreal_patterns(RuntimeData& runtime)
    {
        PatternReport report;
#if UE4SS_WITH_PATTERNSLEUTH
        constexpr std::uint64_t enabled = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING | UE4SS_PS_FNAME_CTOR | UE4SS_PS_GMALLOC |
                                          UE4SS_PS_STATIC_CONSTRUCT_OBJECT | UE4SS_PS_STATIC_FIND_OBJECT | UE4SS_PS_GAME_ENGINE_TICK | UE4SS_PS_ENGINE_LOOP_INIT |
                                          UE4SS_PS_UFUNCTION_BIND | UE4SS_PS_ALLOCATE_UOBJECT_INDEX | UE4SS_PS_FREE_UOBJECT_INDEX | UE4SS_PS_ENGINE_VERSION;
        ue4ss_ps_linux_config config{
                .struct_size = sizeof(config),
                .abi_version = UE4SS_PS_LINUX_ABI_VERSION,
                .enabled_mask = enabled,
                .log = resolver_log,
                .user_data = &runtime,
        };
        ue4ss_ps_linux_results results{};
        results.struct_size = sizeof(results);
        const int scan_error = ps_scan_linux_v1(&config, &results);
        report.scan_succeeded = scan_error == 0;
        report.resolved = {
                .available_mask = results.available_mask,
                .engine_major = results.engine_major,
                .engine_minor = results.engine_minor,
                .guobject_array = results.guobject_array,
                .fname_to_string = results.fname_to_string,
                .fname_ctor = results.fname_ctor,
                .gmalloc = results.gmalloc,
                .static_construct_object = results.static_construct_object,
                .static_find_object = results.static_find_object,
                .game_engine_tick = results.game_engine_tick,
                .engine_loop_init = results.engine_loop_init,
                .ufunction_bind = results.ufunction_bind,
                .allocate_uobject_index = results.allocate_uobject_index,
                .free_uobject_index = results.free_uobject_index,
        };
        report.capabilities.push_back({
                "unreal_pattern_scan",
                scan_error == 0 ? CapabilityState::available : CapabilityState::unavailable,
                scan_error == 0 ? "ELF image scanned without an FFI error" : "patternsleuth bridge failed with error " + std::to_string(scan_error),
        });

        add_address_capability(report, runtime, results, UE4SS_PS_GUOBJECT_ARRAY, results.guobject_array, "object_registry", false);
        add_address_capability(report, runtime, results, UE4SS_PS_FNAME_TO_STRING, results.fname_to_string, "fname_to_string", true);
        add_address_capability(report, runtime, results, UE4SS_PS_FNAME_CTOR, results.fname_ctor, "fname_constructor", true);
        add_address_capability(report, runtime, results, UE4SS_PS_GMALLOC, results.gmalloc, "gmalloc", false);
        add_address_capability(report, runtime, results, UE4SS_PS_STATIC_CONSTRUCT_OBJECT, results.static_construct_object, "static_construct_object", true);
        add_address_capability(report, runtime, results, UE4SS_PS_STATIC_FIND_OBJECT, results.static_find_object, "static_find_object", true);
        add_address_capability(report, runtime, results, UE4SS_PS_GAME_ENGINE_TICK, results.game_engine_tick, "game_thread_tick", true);
        add_address_capability(report, runtime, results, UE4SS_PS_ENGINE_LOOP_INIT, results.engine_loop_init, "engine_loop_init", true);
        add_address_capability(report, runtime, results, UE4SS_PS_UFUNCTION_BIND, results.ufunction_bind, "ufunction_bind", true);
        add_address_capability(report, runtime, results, UE4SS_PS_ALLOCATE_UOBJECT_INDEX, results.allocate_uobject_index, "notify_new_object", true);
        add_address_capability(report, runtime, results, UE4SS_PS_FREE_UOBJECT_INDEX, results.free_uobject_index, "notify_delete_object", true);

        if ((results.available_mask & UE4SS_PS_ENGINE_VERSION) != 0u)
        {
            report.capabilities.push_back({
                    "engine_version",
                    CapabilityState::available,
                    std::to_string(results.engine_major) + "." + std::to_string(results.engine_minor),
            });
        }
        else
        {
            report.capabilities.push_back({"engine_version", CapabilityState::unavailable, "resolver returned no unique result"});
        }

        constexpr std::uint64_t critical = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING | UE4SS_PS_FNAME_CTOR | UE4SS_PS_GMALLOC |
                                           UE4SS_PS_STATIC_CONSTRUCT_OBJECT | UE4SS_PS_STATIC_FIND_OBJECT | UE4SS_PS_GAME_ENGINE_TICK;
        report.critical_capabilities_available =
                scan_error == 0 && (results.available_mask & critical) == critical && address_in_segment(runtime, results.guobject_array, false) &&
                address_in_segment(runtime, results.fname_to_string, true) && address_in_segment(runtime, results.fname_ctor, true) &&
                address_in_segment(runtime, results.gmalloc, false) && address_in_segment(runtime, results.static_construct_object, true) &&
                address_in_segment(runtime, results.static_find_object, true) && address_in_segment(runtime, results.game_engine_tick, true);
#else
        report.capabilities.push_back({"unreal_pattern_scan", CapabilityState::disabled, "UE4SS_WITH_PATTERNSLEUTH=OFF"});
#endif
        return report;
    }
} // namespace ue4ss::linux::core
