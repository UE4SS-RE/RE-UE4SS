#include "status.hpp"
#include "ue4ss/mod_catalog.hpp"
#if UE4SS_WITH_LUA_RUNTIME
#include "headless_lua.hpp"
#endif
#include "object_registry_validator.hpp"
#include "pattern_resolver.hpp"
#include "unreal_readonly.hpp"
#include "game_thread_scheduler.hpp"
#if UE4SS_WITH_HOOK_BACKEND
#include "ufunction_hooks.hpp"
#include "object_notifications.hpp"
#include "blueprint_hooks.hpp"
#include "begin_play_hooks.hpp"
#include "load_map_hooks.hpp"
#include "console_exec_hooks.hpp"
#endif
#if UE4SS_WITH_UNREAL_RUNTIME
#include "unreal_runtime.hpp"
#endif

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <elf.h>
#include <filesystem>
#include <mutex>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <unistd.h>

namespace
{
    using ue4ss::linux::core::Capability;
    using ue4ss::linux::core::CapabilityState;
    using ue4ss::linux::core::RuntimeData;

    std::mutex g_runtime_mutex;
    std::condition_variable g_initialization_finished;
    RuntimeData g_runtime;
    std::thread g_initialization_thread;
    std::thread g_runtime_monitor_thread;
    bool g_initialization_complete{};
    bool g_runtime_monitor_stop{};
    std::condition_variable g_runtime_monitor_wakeup;

    struct ActiveLuaRuntime
    {
        ue4ss::linux::core::ReadOnlyUnrealRuntime unreal;
#if UE4SS_WITH_HOOK_BACKEND
        ue4ss::linux::core::GameThreadScheduler scheduler;
        ue4ss::linux::core::UFunctionHookManager ufunction_hooks;
        ue4ss::linux::core::ObjectNotificationManager object_notifications;
        ue4ss::linux::core::BlueprintHookManager blueprint_hooks;
        ue4ss::linux::core::BeginPlayHookManager begin_play_hooks;
        ue4ss::linux::core::EndPlayHookManager end_play_hooks;
        ue4ss::linux::core::InitGameStateHookManager init_game_state_hooks;
        ue4ss::linux::core::LoadMapHookManager load_map_hooks;
        ue4ss::linux::core::ProcessConsoleExecHookManager process_console_exec_hooks;
        ue4ss::linux::core::CallFunctionByNameWithArgumentsHookManager
                call_function_by_name_hooks;
        ue4ss::linux::core::LocalPlayerExecHookManager local_player_exec_hooks;
#endif
#if UE4SS_WITH_LUA_RUNTIME
        struct LoadedLuaMod
        {
            ue4ss::linux::LuaModDescriptor descriptor;
            std::unique_ptr<ue4ss::linux::core::HeadlessLuaState> state;
        };
        std::filesystem::path ue4ss_root_path;
        std::filesystem::path server_executable_path;
        std::vector<ue4ss::linux::LuaModDescriptor> available_mods;
        std::vector<LoadedLuaMod> loaded_mods;
        std::mutex mod_lifecycle_mutex;
        std::unordered_set<std::string> pending_mod_lifecycle;

        [[nodiscard]] bool load_mod(
                const ue4ss::linux::LuaModDescriptor& mod,
                std::string& error) noexcept;
        [[nodiscard]] bool queue_mod_lifecycle(
                ue4ss::linux::core::LuaModLifecycleAction action,
                std::string_view target_mod,
                std::string& error) noexcept;
        void apply_mod_lifecycle(
                ue4ss::linux::core::LuaModLifecycleAction action,
                std::string normalized_target) noexcept;
#endif

        ~ActiveLuaRuntime()
        {
#if UE4SS_WITH_LUA_RUNTIME
            // Lua states own callback registrations. Destroy them while all
            // managers are still accepting unregister operations, before any
            // detour or scheduler is torn down.
            loaded_mods.clear();
#endif
#if UE4SS_WITH_HOOK_BACKEND
            object_notifications.stop();
            local_player_exec_hooks.stop();
            call_function_by_name_hooks.stop();
            process_console_exec_hooks.stop();
            load_map_hooks.stop();
            init_game_state_hooks.stop();
            end_play_hooks.stop();
            begin_play_hooks.stop();
            blueprint_hooks.stop();
            ufunction_hooks.stop();
            scheduler.stop();
#endif
        }
    };

    struct LuaModActivation
    {
        Capability capability{"lua_mods", CapabilityState::unavailable, "not activated"};
        std::unique_ptr<ActiveLuaRuntime> runtime;
        std::vector<std::string> log_messages;
    };

    std::unique_ptr<ActiveLuaRuntime> g_active_lua_runtime;

    struct RegistryMonitorConfig
    {
        ue4ss::linux::core::ResolvedUnrealState resolved;
        std::filesystem::path root_path;
        std::filesystem::path executable_path;
        std::chrono::seconds timeout{120};
        std::chrono::seconds settle_time{3};
    };

    [[nodiscard]] std::string copy_optional(const char* value)
    {
        return value != nullptr ? value : "";
    }

    [[nodiscard]] std::string ascii_lower_copy(std::string_view value)
    {
        std::string output;
        output.reserve(value.size());
        for (const char character : value)
        {
            output.push_back(character >= 'A' && character <= 'Z'
                                     ? static_cast<char>(character + ('a' - 'A'))
                                     : character);
        }
        return output;
    }

    [[nodiscard]] bool hashes_equal(std::string_view left, std::string_view right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < left.size(); ++index)
        {
            const auto normalize = [](char character) {
                return character >= 'A' && character <= 'F' ? static_cast<char>(character + ('a' - 'A')) : character;
            };
            if (normalize(left[index]) != normalize(right[index]))
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool should_scan_unreal(const RuntimeData& runtime)
    {
        if (const char* force = std::getenv("UE4SS_SCAN_UNREAL"); force != nullptr)
        {
            return std::strcmp(force, "1") == 0 || std::strcmp(force, "true") == 0 || std::strcmp(force, "yes") == 0;
        }
        return std::filesystem::path{runtime.executable_path}.filename().string().starts_with("PalServer-Linux-Shipping");
    }

    [[nodiscard]] std::chrono::seconds registry_monitor_timeout()
    {
        constexpr unsigned long default_seconds = 120u;
        constexpr unsigned long maximum_seconds = 600u;
        const char* configured = std::getenv("UE4SS_REGISTRY_WAIT_SECONDS");
        if (configured == nullptr || configured[0] == '\0')
        {
            return std::chrono::seconds{default_seconds};
        }

        char* end{};
        errno = 0;
        const unsigned long seconds = std::strtoul(configured, &end, 10);
        if (errno != 0 || end == configured || *end != '\0' || seconds == 0u || seconds > maximum_seconds)
        {
            return std::chrono::seconds{default_seconds};
        }
        return std::chrono::seconds{seconds};
    }

    [[nodiscard]] std::chrono::seconds registry_settle_time()
    {
        constexpr unsigned long default_seconds = 3u;
        constexpr unsigned long maximum_seconds = 30u;
        const char* configured = std::getenv("UE4SS_REGISTRY_SETTLE_SECONDS");
        if (configured == nullptr || configured[0] == '\0')
        {
            return std::chrono::seconds{default_seconds};
        }

        char* end{};
        errno = 0;
        const unsigned long seconds = std::strtoul(configured, &end, 10);
        if (errno != 0 || end == configured || *end != '\0' || seconds == 0u || seconds > maximum_seconds)
        {
            return std::chrono::seconds{default_seconds};
        }
        return std::chrono::seconds{seconds};
    }

    void set_capability(RuntimeData& runtime, std::string_view name, CapabilityState state, std::string detail)
    {
        const auto capability = std::find_if(runtime.capabilities.begin(), runtime.capabilities.end(), [name](const Capability& candidate) {
            return candidate.name == name;
        });
        if (capability != runtime.capabilities.end())
        {
            capability->state = state;
            capability->detail = std::move(detail);
            return;
        }
        runtime.capabilities.push_back({std::string{name}, state, std::move(detail)});
    }

#if UE4SS_WITH_LUA_RUNTIME
    bool ActiveLuaRuntime::load_mod(
            const ue4ss::linux::LuaModDescriptor& mod,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            {
                const std::string normalized_name = ascii_lower_copy(mod.name);
                std::lock_guard lock{mod_lifecycle_mutex};
                if (std::any_of(
                            loaded_mods.begin(),
                            loaded_mods.end(),
                            [&normalized_name](const LoadedLuaMod& loaded) {
                                return ascii_lower_copy(loaded.descriptor.name) ==
                                       normalized_name;
                            }))
                {
                    error = "Lua mod '" + mod.name + "' is already loaded";
                    return false;
                }
            }

            auto lua =
                    std::make_unique<ue4ss::linux::core::HeadlessLuaState>();
            const auto binding_result = lua->install_readonly_unreal_bindings(
                    unreal,
#if UE4SS_WITH_HOOK_BACKEND
                    &scheduler,
                    &ufunction_hooks,
                    &object_notifications,
                    &blueprint_hooks,
                    &begin_play_hooks,
                    &end_play_hooks,
                    &init_game_state_hooks,
                    &load_map_hooks,
                    &process_console_exec_hooks,
                    &call_function_by_name_hooks,
                    &local_player_exec_hooks,
#else
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
#endif
                    server_executable_path.parent_path(),
                    ue4ss_root_path);
            if (!binding_result.success)
            {
                error = "mod '" + mod.name +
                        "' binding setup failed: " + binding_result.detail;
                return false;
            }
            lua->configure_mod_lifecycle(
                    mod.name,
                    [this](
                            ue4ss::linux::core::LuaModLifecycleAction action,
                            std::string_view target_mod,
                            std::string& lifecycle_error) {
                        return queue_mod_lifecycle(
                                action, target_mod, lifecycle_error);
                    });
            const auto execution_result = lua->execute_file(mod.main_script);
            if (!execution_result.success)
            {
                error = "mod '" + mod.name +
                        "' failed during protected load: " +
                        execution_result.detail;
                return false;
            }
            {
                std::lock_guard lock{mod_lifecycle_mutex};
                loaded_mods.push_back({mod, std::move(lua)});
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while loading Lua mod '" + mod.name + "'";
            return false;
        }
    }

    bool ActiveLuaRuntime::queue_mod_lifecycle(
            ue4ss::linux::core::LuaModLifecycleAction action,
            std::string_view target_mod,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
#if UE4SS_WITH_HOOK_BACKEND
            if (!scheduler.is_ready())
            {
                error = "validated game-thread scheduler is unavailable";
                return false;
            }
            const std::string normalized_target =
                    ascii_lower_copy(target_mod);
            if (normalized_target.empty())
            {
                error = "mod lifecycle target is empty";
                return false;
            }
            {
                std::lock_guard lock{mod_lifecycle_mutex};
                const bool known = std::any_of(
                        available_mods.begin(),
                        available_mods.end(),
                        [&normalized_target](const auto& candidate) {
                            return ascii_lower_copy(candidate.name) ==
                                   normalized_target;
                        });
                if (!known)
                {
                    error = "mod '" + std::string{target_mod} + "' is not installed";
                    return false;
                }
                if (!pending_mod_lifecycle.emplace(normalized_target).second)
                {
                    error = "a lifecycle request for mod '" +
                            std::string{target_mod} + "' is already queued";
                    return false;
                }
            }
            const std::uint64_t task = scheduler.enqueue(
                    [this, action, normalized_target] {
                        apply_mod_lifecycle(action, normalized_target);
                        return true;
                    });
            if (task == 0u)
            {
                std::lock_guard lock{mod_lifecycle_mutex};
                pending_mod_lifecycle.erase(normalized_target);
                error = "mod lifecycle request could not be scheduled";
                return false;
            }
            return true;
#else
            static_cast<void>(action);
            static_cast<void>(target_mod);
            error = "UE4SS_WITH_HOOK_BACKEND=OFF";
            return false;
#endif
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while queuing a mod lifecycle request";
            return false;
        }
    }

    void ActiveLuaRuntime::apply_mod_lifecycle(
            ue4ss::linux::core::LuaModLifecycleAction action,
            std::string normalized_target) noexcept
    {
        std::string result_error;
        std::string display_name = normalized_target;
        try
        {
            ue4ss::linux::LuaModDescriptor descriptor;
            std::unique_ptr<ue4ss::linux::core::HeadlessLuaState> old_state;
            {
                std::lock_guard lock{mod_lifecycle_mutex};
                const auto available = std::find_if(
                        available_mods.begin(),
                        available_mods.end(),
                        [&normalized_target](const auto& candidate) {
                            return ascii_lower_copy(candidate.name) ==
                                   normalized_target;
                        });
                if (available != available_mods.end())
                {
                    descriptor = *available;
                    display_name = available->name;
                }
                const auto loaded = std::find_if(
                        loaded_mods.begin(),
                        loaded_mods.end(),
                        [&normalized_target](const LoadedLuaMod& candidate) {
                            return ascii_lower_copy(candidate.descriptor.name) ==
                                   normalized_target;
                        });
                if (loaded == loaded_mods.end())
                {
                    result_error = "mod is not currently loaded";
                }
                else
                {
                    old_state = std::move(loaded->state);
                    loaded_mods.erase(loaded);
                }
            }

            // Destruction must happen after leaving the lifecycle mutex: it
            // waits for in-flight Lua callbacks and clears scheduler-owned
            // actions, either of which may need the same controller.
            old_state.reset();
            if (result_error.empty() &&
                action == ue4ss::linux::core::LuaModLifecycleAction::restart &&
                !load_mod(descriptor, result_error))
            {
                if (result_error.empty())
                {
                    result_error = "mod reload failed without a diagnostic";
                }
            }
        }
        catch (const std::exception& exception)
        {
            result_error = exception.what();
        }
        catch (...)
        {
            result_error = "unknown exception while applying mod lifecycle request";
        }

        {
            std::lock_guard lock{mod_lifecycle_mutex};
            pending_mod_lifecycle.erase(normalized_target);
        }
        const char* action_name =
                action == ue4ss::linux::core::LuaModLifecycleAction::restart
                        ? "restart"
                        : "uninstall";
        if (result_error.empty())
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] Lua mod %s completed: %s\n",
                    action_name,
                    display_name.c_str());
        }
        else
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] Lua mod %s failed for %s: %s\n",
                    action_name,
                    display_name.c_str(),
                    result_error.c_str());
        }
    }
#endif

    [[nodiscard]] Capability validate_lua_readonly_capability(
            ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyObjectApiValidation& object_api_validation,
            ue4ss::linux::core::GameThreadScheduler* scheduler,
            ue4ss::linux::core::UFunctionHookManager* hooks,
            ue4ss::linux::core::ObjectNotificationManager* notifications,
            ue4ss::linux::core::BlueprintHookManager* blueprint_hooks,
            ue4ss::linux::core::BeginPlayHookManager* begin_play_hooks,
            ue4ss::linux::core::EndPlayHookManager* end_play_hooks,
            ue4ss::linux::core::InitGameStateHookManager* init_game_state_hooks,
            ue4ss::linux::core::LoadMapHookManager* load_map_hooks,
            ue4ss::linux::core::ProcessConsoleExecHookManager* process_console_exec_hooks,
            ue4ss::linux::core::CallFunctionByNameWithArgumentsHookManager*
                    call_function_by_name_hooks,
            ue4ss::linux::core::LocalPlayerExecHookManager* local_player_exec_hooks)
    {
#if UE4SS_WITH_LUA_RUNTIME
        if (!object_api_validation.success)
        {
            return {"lua_readonly_bindings", CapabilityState::unavailable, "read-only UObject API gate did not pass"};
        }
        const auto probe = ue4ss::linux::core::probe_readonly_unreal_lua(
                runtime,
                scheduler,
                hooks,
                notifications,
                blueprint_hooks,
                begin_play_hooks,
                end_play_hooks,
                init_game_state_hooks,
                load_map_hooks,
                process_console_exec_hooks,
                call_function_by_name_hooks,
                local_player_exec_hooks);
        return {"lua_readonly_bindings",
                probe.success ? CapabilityState::available : CapabilityState::unavailable,
                probe.detail};
#else
        static_cast<void>(runtime);
        static_cast<void>(object_api_validation);
        static_cast<void>(scheduler);
        static_cast<void>(hooks);
        static_cast<void>(notifications);
        static_cast<void>(blueprint_hooks);
        static_cast<void>(begin_play_hooks);
        static_cast<void>(end_play_hooks);
        static_cast<void>(init_game_state_hooks);
        static_cast<void>(load_map_hooks);
        static_cast<void>(process_console_exec_hooks);
        static_cast<void>(call_function_by_name_hooks);
        static_cast<void>(local_player_exec_hooks);
        return {"lua_readonly_bindings", CapabilityState::disabled, "UE4SS_WITH_LUA_RUNTIME=OFF"};
#endif
    }

    [[nodiscard]] LuaModActivation activate_lua_mods(const std::filesystem::path& root_path,
                                                      const std::filesystem::path& executable_path,
                                                      const Capability& lua_binding_validation,
                                                      std::unique_ptr<ActiveLuaRuntime> active) noexcept
    {
        LuaModActivation activation;
#if UE4SS_WITH_LUA_RUNTIME
        try
        {
            if (lua_binding_validation.state != CapabilityState::available)
            {
                activation.capability.detail = "read-only Lua binding gate did not pass";
                return activation;
            }
            const auto catalog = ue4ss::linux::discover_lua_mods(root_path);
            if (!catalog.rejected_mods.empty())
            {
                activation.capability.detail = "one or more enabled mod directories failed safe path validation";
                activation.log_messages = catalog.rejected_mods;
                return activation;
            }

            if (active == nullptr || !active->unreal.is_ready())
            {
                activation.capability.detail = "validated shared read-only Unreal runtime is required";
                return activation;
            }

            active->ue4ss_root_path = root_path;
            active->server_executable_path = executable_path;
            active->available_mods = catalog.enabled_mods;

            for (const auto& mod : catalog.enabled_mods)
            {
                std::string load_error;
                if (!active->load_mod(mod, load_error))
                {
                    activation.capability.detail = std::move(load_error);
                    return activation;
                }
                activation.log_messages.push_back("loaded read-only Lua mod '" + mod.name + "' from " + mod.main_script.string());
            }

            activation.capability = {
                    "lua_mods",
                    CapabilityState::available,
                    std::to_string(catalog.enabled_mods.size()) +
                            " Lua mod(s) loaded in isolated VMs with UObject discovery, reflected properties/functions, game-thread scheduling, native/Blueprint UFunction hooks, hook parameters, and object notifications",
            };
            activation.runtime = std::move(active);
            return activation;
        }
        catch (const std::exception& exception)
        {
            activation.capability.detail = exception.what();
            return activation;
        }
        catch (...)
        {
            activation.capability.detail = "unknown exception while activating read-only Lua mods";
            return activation;
        }
#else
        static_cast<void>(root_path);
        static_cast<void>(executable_path);
        static_cast<void>(lua_binding_validation);
        static_cast<void>(active);
        activation.capability = {"lua_mods", CapabilityState::disabled, "UE4SS_WITH_LUA_RUNTIME=OFF"};
        return activation;
#endif
    }

    void publish_runtime_update(CapabilityState state,
                                const std::string& detail,
                                const std::string& message,
                                const ue4ss::linux::core::FNameRuntimeValidation* fname_validation = nullptr,
                                const ue4ss::linux::core::ReadOnlyObjectApiValidation* object_api_validation = nullptr,
                                const Capability* fproperty_validation = nullptr,
                                const Capability* uenum_validation = nullptr,
                                const Capability* process_event_validation = nullptr,
                                const Capability* process_internal_validation = nullptr,
                                const Capability* ufunction_hook_validation = nullptr,
                                const Capability* blueprint_hook_validation = nullptr,
                                const Capability* begin_play_hook_validation = nullptr,
                                const Capability* end_play_hook_validation = nullptr,
                                const Capability* init_game_state_hook_validation = nullptr,
                                const Capability* load_map_hook_validation = nullptr,
                                const Capability* process_console_exec_hook_validation = nullptr,
                                const Capability* call_function_by_name_hook_validation = nullptr,
                                const Capability* local_player_exec_hook_validation = nullptr,
                                const Capability* object_notification_validation = nullptr,
                                const Capability* scheduler_validation = nullptr,
                                const Capability* lua_binding_validation = nullptr,
                                LuaModActivation* lua_mod_activation = nullptr)
    {
        RuntimeData snapshot;
        {
            std::lock_guard lock{g_runtime_mutex};
            if (g_runtime_monitor_stop || g_runtime.state == UE4SS_STATE_STOPPED)
            {
                return;
            }
            set_capability(g_runtime, "object_registry_layout", state, detail);
            if (fname_validation != nullptr)
            {
                set_capability(g_runtime,
                               "fname_runtime_abi",
                               fname_validation->success ? CapabilityState::available : CapabilityState::unavailable,
                               fname_validation->detail);
                if (object_api_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   "readonly_object_api",
                                   object_api_validation->success ? CapabilityState::available : CapabilityState::unavailable,
                                   object_api_validation->detail);
                }
                if (fproperty_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   fproperty_validation->name,
                                   fproperty_validation->state,
                                   fproperty_validation->detail);
                }
                if (uenum_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   uenum_validation->name,
                                   uenum_validation->state,
                                   uenum_validation->detail);
                }
                if (process_event_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   process_event_validation->name,
                                   process_event_validation->state,
                                   process_event_validation->detail);
                }
                if (process_internal_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   process_internal_validation->name,
                                   process_internal_validation->state,
                                   process_internal_validation->detail);
                }
                if (ufunction_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   ufunction_hook_validation->name,
                                   ufunction_hook_validation->state,
                                   ufunction_hook_validation->detail);
                }
                if (blueprint_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   blueprint_hook_validation->name,
                                   blueprint_hook_validation->state,
                                   blueprint_hook_validation->detail);
                }
                if (begin_play_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   begin_play_hook_validation->name,
                                   begin_play_hook_validation->state,
                                   begin_play_hook_validation->detail);
                }
                if (end_play_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   end_play_hook_validation->name,
                                   end_play_hook_validation->state,
                                   end_play_hook_validation->detail);
                }
                if (init_game_state_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   init_game_state_hook_validation->name,
                                   init_game_state_hook_validation->state,
                                   init_game_state_hook_validation->detail);
                }
                if (load_map_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   load_map_hook_validation->name,
                                   load_map_hook_validation->state,
                                   load_map_hook_validation->detail);
                }
                if (process_console_exec_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   process_console_exec_hook_validation->name,
                                   process_console_exec_hook_validation->state,
                                   process_console_exec_hook_validation->detail);
                }
                if (call_function_by_name_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   call_function_by_name_hook_validation->name,
                                   call_function_by_name_hook_validation->state,
                                   call_function_by_name_hook_validation->detail);
                }
                if (local_player_exec_hook_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   local_player_exec_hook_validation->name,
                                   local_player_exec_hook_validation->state,
                                   local_player_exec_hook_validation->detail);
                }
                if (object_notification_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   object_notification_validation->name,
                                   object_notification_validation->state,
                                   object_notification_validation->detail);
                }
                if (lua_binding_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   lua_binding_validation->name,
                                   lua_binding_validation->state,
                                   lua_binding_validation->detail);
                }
                if (scheduler_validation != nullptr)
                {
                    set_capability(g_runtime,
                                   scheduler_validation->name,
                                   scheduler_validation->state,
                                   scheduler_validation->detail);
                }
                if (lua_mod_activation != nullptr)
                {
                    set_capability(g_runtime,
                                   lua_mod_activation->capability.name,
                                   lua_mod_activation->capability.state,
                                   lua_mod_activation->capability.detail);
                    if (lua_mod_activation->capability.state == CapabilityState::available)
                    {
                        g_active_lua_runtime = std::move(lua_mod_activation->runtime);
                    }
                }
                const bool runtime_ready =
                        state == CapabilityState::available && fname_validation->success &&
                        object_api_validation != nullptr && object_api_validation->success &&
                        fproperty_validation != nullptr &&
                        fproperty_validation->state == CapabilityState::available &&
                        uenum_validation != nullptr &&
                        uenum_validation->state == CapabilityState::available &&
                        process_event_validation != nullptr &&
                        process_event_validation->state == CapabilityState::available &&
                        process_internal_validation != nullptr &&
                        process_internal_validation->state == CapabilityState::available &&
                        ufunction_hook_validation != nullptr &&
                        ufunction_hook_validation->state == CapabilityState::available &&
                        blueprint_hook_validation != nullptr &&
                        blueprint_hook_validation->state == CapabilityState::available &&
                        begin_play_hook_validation != nullptr &&
                        begin_play_hook_validation->state == CapabilityState::available &&
                        end_play_hook_validation != nullptr &&
                        end_play_hook_validation->state == CapabilityState::available &&
                        init_game_state_hook_validation != nullptr &&
                        init_game_state_hook_validation->state == CapabilityState::available &&
                        load_map_hook_validation != nullptr &&
                        load_map_hook_validation->state == CapabilityState::available &&
                        process_console_exec_hook_validation != nullptr &&
                        process_console_exec_hook_validation->state == CapabilityState::available &&
                        call_function_by_name_hook_validation != nullptr &&
                        call_function_by_name_hook_validation->state == CapabilityState::available &&
                        local_player_exec_hook_validation != nullptr &&
                        local_player_exec_hook_validation->state == CapabilityState::available &&
                        object_notification_validation != nullptr &&
                        object_notification_validation->state == CapabilityState::available &&
                        scheduler_validation != nullptr &&
                        scheduler_validation->state == CapabilityState::available &&
                        lua_binding_validation != nullptr &&
                        lua_binding_validation->state == CapabilityState::available &&
                        lua_mod_activation != nullptr &&
                        lua_mod_activation->capability.state == CapabilityState::available;
                if (runtime_ready)
                {
                    g_runtime.state = UE4SS_STATE_PLATFORM_READY;
                    g_runtime.last_error = 0;
                    g_runtime.message =
                            "headless Lua mod runtime is active with reflected access, scheduling, native/Blueprint hooks, BeginPlay/EndPlay/InitGameState/LoadMap/ProcessConsoleExec/CallFunctionByName/ULocalPlayerExec callbacks, remote hook parameters, and object notifications";
                }
                else if (g_runtime.required)
                {
                    g_runtime.state = UE4SS_STATE_FAILED;
                    g_runtime.last_error = ENOTSUP;
                    g_runtime.message = "a required deferred Unreal ABI, hook, or Lua mod gate failed";
                }
                else
                {
                    g_runtime.state = UE4SS_STATE_DEGRADED;
                    g_runtime.message =
                            "one or more deferred Unreal ABI, hook, or Lua mod gates are unavailable; affected features remain fail-closed";
                }
            }
            else
            {
                set_capability(g_runtime, "fname_runtime_abi", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "readonly_object_api", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "fproperty_runtime_abi", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "uenum_runtime_abi", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "game_thread_scheduler", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "begin_play_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "end_play_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "init_game_state_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "load_map_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "process_console_exec_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "call_function_by_name_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "local_player_exec_hooks", CapabilityState::unavailable, "object registry is not ready");
                set_capability(g_runtime, "lua_readonly_bindings", CapabilityState::unavailable, "object registry is not ready");
                if (g_runtime.required)
                {
                    g_runtime.state = UE4SS_STATE_FAILED;
                    g_runtime.last_error = ENOTSUP;
                }
                else
                {
                    g_runtime.state = UE4SS_STATE_DEGRADED;
                }
                g_runtime.message = message;
            }
            snapshot = g_runtime;
        }

        ue4ss::linux::core::log_line(snapshot, state == CapabilityState::available ? "info" : "warning", detail);
        if (fname_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         fname_validation->success ? "info" : "warning",
                                         fname_validation->detail);
        }
        if (object_api_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         object_api_validation->success ? "info" : "warning",
                                         object_api_validation->detail);
        }
        if (fproperty_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         fproperty_validation->state == CapabilityState::available ? "info" : "warning",
                                         fproperty_validation->detail);
        }
        if (uenum_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         uenum_validation->state == CapabilityState::available ? "info" : "warning",
                                         uenum_validation->detail);
        }
        if (lua_binding_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         lua_binding_validation->state == CapabilityState::available ? "info" : "warning",
                                         lua_binding_validation->detail);
        }
        if (process_event_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         process_event_validation->state == CapabilityState::available ? "info" : "warning",
                                         process_event_validation->detail);
        }
        if (process_internal_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         process_internal_validation->state == CapabilityState::available ? "info" : "warning",
                                         process_internal_validation->detail);
        }
        if (ufunction_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         ufunction_hook_validation->state == CapabilityState::available ? "info" : "warning",
                                         ufunction_hook_validation->detail);
        }
        if (blueprint_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         blueprint_hook_validation->state == CapabilityState::available ? "info" : "warning",
                                         blueprint_hook_validation->detail);
        }
        if (begin_play_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         begin_play_hook_validation->state == CapabilityState::available ? "info" : "warning",
                                         begin_play_hook_validation->detail);
        }
        if (end_play_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         end_play_hook_validation->state == CapabilityState::available ? "info" : "warning",
                                         end_play_hook_validation->detail);
        }
        if (init_game_state_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(
                    snapshot,
                    init_game_state_hook_validation->state == CapabilityState::available ? "info" : "warning",
                    init_game_state_hook_validation->detail);
        }
        if (load_map_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         load_map_hook_validation->state == CapabilityState::available ? "info" : "warning",
                                         load_map_hook_validation->detail);
        }
        if (process_console_exec_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(
                    snapshot,
                    process_console_exec_hook_validation->state == CapabilityState::available
                            ? "info"
                            : "warning",
                    process_console_exec_hook_validation->detail);
        }
        if (call_function_by_name_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(
                    snapshot,
                    call_function_by_name_hook_validation->state == CapabilityState::available
                            ? "info"
                            : "warning",
                    call_function_by_name_hook_validation->detail);
        }
        if (local_player_exec_hook_validation != nullptr)
        {
            ue4ss::linux::core::log_line(
                    snapshot,
                    local_player_exec_hook_validation->state == CapabilityState::available
                            ? "info"
                            : "warning",
                    local_player_exec_hook_validation->detail);
        }
        if (object_notification_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         object_notification_validation->state == CapabilityState::available ? "info" : "warning",
                                         object_notification_validation->detail);
        }
        if (scheduler_validation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         scheduler_validation->state == CapabilityState::available ? "info" : "warning",
                                         scheduler_validation->detail);
        }
        if (lua_mod_activation != nullptr)
        {
            ue4ss::linux::core::log_line(snapshot,
                                         lua_mod_activation->capability.state == CapabilityState::available ? "info" : "warning",
                                         lua_mod_activation->capability.detail);
            for (const auto& log_message : lua_mod_activation->log_messages)
            {
                ue4ss::linux::core::log_line(snapshot,
                                             lua_mod_activation->capability.state == CapabilityState::available ? "info" : "warning",
                                             log_message);
            }
        }
        try
        {
            ue4ss::linux::core::write_status_file(snapshot);
        }
        catch (const std::exception& error)
        {
            ue4ss::linux::core::log_line(snapshot, "error", std::string{"cannot publish deferred UE4SS status: "} + error.what());
        }
        if (snapshot.required && snapshot.state == UE4SS_STATE_FAILED)
        {
            ue4ss::linux::core::log_line(
                    snapshot, "error", "UE4SS_REQUIRED is true; terminating after a deferred runtime gate failure");
            _exit(78);
        }
    }

    void monitor_object_registry(RegistryMonitorConfig config)
    {
        using ue4ss::linux::core::ObjectRegistryValidationState;
        const auto deadline = std::chrono::steady_clock::now() + config.timeout;
        ue4ss::linux::core::ObjectRegistryValidation last_validation;
        std::string last_logged_detail;
        std::optional<std::chrono::steady_clock::time_point> stable_since;
        std::int32_t stable_element_count{-1};

        const auto monitor_log = [](const std::string& level, const std::string& message) {
            RuntimeData snapshot;
            {
                std::lock_guard lock{g_runtime_mutex};
                snapshot = g_runtime;
            }
            ue4ss::linux::core::log_line(snapshot, level, message);
        };
        monitor_log("info",
                    "object-registry monitor started with timeout=" + std::to_string(config.timeout.count()) +
                            "s settle=" + std::to_string(config.settle_time.count()) + "s");

        for (;;)
        {
            last_validation = ue4ss::linux::core::validate_object_registry(
                    config.resolved.guobject_array, config.resolved.engine_major, config.resolved.engine_minor);
            if (last_validation.detail != last_logged_detail)
            {
                monitor_log("info", "object-registry monitor: " + last_validation.detail);
                last_logged_detail = last_validation.detail;
            }
            if (last_validation.state == ObjectRegistryValidationState::ready)
            {
                const auto now = std::chrono::steady_clock::now();
                if (!stable_since.has_value() || stable_element_count != last_validation.num_elements)
                {
                    stable_since = now;
                    stable_element_count = last_validation.num_elements;
                    monitor_log("info",
                                "object-registry monitor waiting for stable post-main state at elements=" +
                                        std::to_string(stable_element_count));
                }
                else if (now - *stable_since >= config.settle_time)
                {
                    monitor_log("info", "object-registry monitor entering the FName runtime ABI gate");
                    auto active = std::make_unique<ActiveLuaRuntime>();
                    std::string fname_detail;
                    const bool fname_success = active->unreal.initialize(config.resolved, fname_detail);
                    const ue4ss::linux::core::FNameRuntimeValidation fname_validation{fname_success, std::move(fname_detail)};
                    monitor_log(fname_validation.success ? "info" : "warning", fname_validation.detail);
                    const auto object_api_validation = fname_validation.success
                                                               ? ue4ss::linux::core::validate_readonly_object_api(active->unreal)
                                                               : ue4ss::linux::core::ReadOnlyObjectApiValidation{
                                                                         false, "FName runtime ABI gate did not pass"};
                    monitor_log(object_api_validation.success ? "info" : "warning", object_api_validation.detail);
                    Capability fproperty_validation{
                            "fproperty_runtime_abi",
                            CapabilityState::unavailable,
                            "read-only UObject API gate did not pass",
                    };
                    if (object_api_validation.success)
                    {
                        std::string fproperty_detail;
                        const bool fproperty_ready = active->unreal.initialize_fproperty_abi(fproperty_detail);
                        fproperty_validation = {
                                "fproperty_runtime_abi",
                                fproperty_ready ? CapabilityState::available : CapabilityState::unavailable,
                                std::move(fproperty_detail),
                        };
                    }
                    monitor_log(fproperty_validation.state == CapabilityState::available ? "info" : "warning",
                                fproperty_validation.detail);
                    Capability uenum_validation{
                            "uenum_runtime_abi",
                            CapabilityState::unavailable,
                            "read-only UObject API gate did not pass",
                    };
                    if (object_api_validation.success)
                    {
                        std::string uenum_detail;
                        const bool uenum_ready = active->unreal.initialize_uenum_abi(uenum_detail);
                        uenum_validation = {
                                "uenum_runtime_abi",
                                uenum_ready ? CapabilityState::available : CapabilityState::unavailable,
                                std::move(uenum_detail),
                        };
                    }
                    monitor_log(uenum_validation.state == CapabilityState::available ? "info" : "warning",
                                uenum_validation.detail);
                    Capability process_event_validation{
                            "process_event", CapabilityState::unavailable, "read-only UObject API gate did not pass"};
                    if (object_api_validation.success)
                    {
                        std::string process_event_detail;
                        const bool process_event_ready = active->unreal.initialize_process_event(process_event_detail);
                        process_event_validation = {
                                "process_event",
                                process_event_ready ? CapabilityState::available : CapabilityState::unavailable,
                                std::move(process_event_detail),
                        };
                    }
                    monitor_log(process_event_validation.state == CapabilityState::available ? "info" : "warning",
                                process_event_validation.detail);
                    Capability ufunction_hook_validation{
                            "native_ufunction_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string hook_error;
                        const bool hook_manager_ready = active->ufunction_hooks.start(active->unreal, hook_error);
                        ufunction_hook_validation = {
                                "native_ufunction_hooks",
                                hook_manager_ready ? CapabilityState::available : CapabilityState::unavailable,
                                hook_manager_ready
                                        ? "atomic UFunction::Func pointer-slot hook manager is armed; no executable Unreal code is patched"
                                        : std::move(hook_error),
                        };
                    }
#else
                    ufunction_hook_validation = {
                            "native_ufunction_hooks", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(ufunction_hook_validation.state == CapabilityState::available ? "info" : "warning",
                                ufunction_hook_validation.detail);
                    Capability process_internal_validation{
                            "process_internal",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
                    Capability blueprint_hook_validation{
                            "blueprint_hooks",
                            CapabilityState::unavailable,
                            "ProcessInternal/ProcessLocalScriptFunction runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string blueprint_error;
                        const bool blueprint_ready = active->blueprint_hooks.start(active->unreal, blueprint_error);
                        process_internal_validation = {
                                "process_internal",
                                blueprint_ready ? CapabilityState::available : CapabilityState::unavailable,
                                blueprint_ready
                                        ? active->blueprint_hooks.addresses().detail
                                        : blueprint_error,
                        };
                        blueprint_hook_validation = {
                                "blueprint_hooks",
                                blueprint_ready ? CapabilityState::available : CapabilityState::unavailable,
                                blueprint_ready
                                        ? "Blueprint UFunction::Func and lazy ProcessLocalScriptFunction hook paths are armed"
                                        : std::move(blueprint_error),
                        };
                    }
#else
                    process_internal_validation = {
                            "process_internal", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
                    blueprint_hook_validation = {
                            "blueprint_hooks", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(process_internal_validation.state == CapabilityState::available ? "info" : "warning",
                                process_internal_validation.detail);
                    monitor_log(blueprint_hook_validation.state == CapabilityState::available ? "info" : "warning",
                                blueprint_hook_validation.detail);
                    Capability begin_play_hook_validation{
                            "begin_play_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string begin_play_error;
                        const bool begin_play_ready = active->begin_play_hooks.start(
                                active->unreal, begin_play_error);
                        begin_play_hook_validation = {
                                "begin_play_hooks",
                                begin_play_ready ? CapabilityState::available : CapabilityState::unavailable,
                                begin_play_ready
                                        ? "AActor::BeginPlay native inline hook resolved from the validated UE 5.1 Linux Itanium Actor CDO vtable slot +0x388"
                                        : std::move(begin_play_error),
                        };
                    }
#else
                    begin_play_hook_validation = {
                            "begin_play_hooks", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(begin_play_hook_validation.state == CapabilityState::available ? "info" : "warning",
                                begin_play_hook_validation.detail);
                    Capability end_play_hook_validation{
                            "end_play_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string end_play_error;
                        const bool end_play_ready = active->end_play_hooks.start(
                                active->unreal, end_play_error);
                        end_play_hook_validation = {
                                "end_play_hooks",
                                end_play_ready ? CapabilityState::available : CapabilityState::unavailable,
                                end_play_ready
                                        ? "AActor::EndPlay native inline hook resolved from the validated UE 5.1 Linux Itanium Actor CDO vtable slot +0x390"
                                        : std::move(end_play_error),
                        };
                    }
#else
                    end_play_hook_validation = {
                            "end_play_hooks", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(end_play_hook_validation.state == CapabilityState::available ? "info" : "warning",
                                end_play_hook_validation.detail);
                    Capability init_game_state_hook_validation{
                            "init_game_state_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string init_game_state_error;
                        const bool init_game_state_ready = active->init_game_state_hooks.start(
                                active->unreal, init_game_state_error);
                        init_game_state_hook_validation = {
                                "init_game_state_hooks",
                                init_game_state_ready ? CapabilityState::available : CapabilityState::unavailable,
                                init_game_state_ready
                                        ? "AGameModeBase::InitGameState native inline hook resolved from the validated UE 5.1 Linux Itanium vtable slot +0x740"
                                        : std::move(init_game_state_error),
                        };
                    }
#else
                    init_game_state_hook_validation = {
                            "init_game_state_hooks", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(
                            init_game_state_hook_validation.state == CapabilityState::available ? "info" : "warning",
                            init_game_state_hook_validation.detail);
                    Capability load_map_hook_validation{
                            "load_map_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string load_map_error;
                        const bool load_map_ready = active->load_map_hooks.start(
                                active->unreal, load_map_error);
                        load_map_hook_validation = {
                                "load_map_hooks",
                                load_map_ready ? CapabilityState::available : CapabilityState::unavailable,
                                load_map_ready
                                        ? "UGameEngine::LoadMap native inline hook resolved from the validated UE 5.1 Linux Itanium vtable slot +0x4c8 using the raw indirect-FURL SysV ABI"
                                        : std::move(load_map_error),
                        };
                    }
#else
                    load_map_hook_validation = {
                            "load_map_hooks", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(load_map_hook_validation.state == CapabilityState::available ? "info" : "warning",
                                load_map_hook_validation.detail);
                    Capability process_console_exec_hook_validation{
                            "process_console_exec_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string console_exec_error;
                        const bool console_exec_ready =
                                active->process_console_exec_hooks.start(
                                        active->unreal, console_exec_error);
                        std::string console_exec_detail;
                        if (console_exec_ready)
                        {
                            std::ostringstream detail;
                            detail << "UObject::ProcessConsoleExec native inline hook resolved from the validated UE 5.1 Linux Itanium UObject CDO vtable slot +0x280 at 0x"
                                   << std::hex
                                   << active->process_console_exec_hooks.target_address()
                                   << "; validated r8d=false tail target=0x"
                                   << active->process_console_exec_hooks
                                              .call_function_by_name_target();
                            console_exec_detail = detail.str();
                        }
                        process_console_exec_hook_validation = {
                                "process_console_exec_hooks",
                                console_exec_ready ? CapabilityState::available
                                                   : CapabilityState::unavailable,
                                console_exec_ready
                                        ? std::move(console_exec_detail)
                                        : std::move(console_exec_error),
                        };
                    }
#else
                    process_console_exec_hook_validation = {
                            "process_console_exec_hooks",
                            CapabilityState::disabled,
                            "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(
                            process_console_exec_hook_validation.state == CapabilityState::available
                                    ? "info"
                                    : "warning",
                            process_console_exec_hook_validation.detail);
                    Capability call_function_by_name_hook_validation{
                            "call_function_by_name_hooks",
                            CapabilityState::unavailable,
                            "validated ProcessConsoleExec tail-thunk gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (process_console_exec_hook_validation.state ==
                        CapabilityState::available)
                    {
                        std::string call_function_error;
                        const bool call_function_ready =
                                active->call_function_by_name_hooks.start(
                                        active->unreal,
                                        active->process_console_exec_hooks,
                                        call_function_error);
                        std::string detail;
                        if (call_function_ready)
                        {
                            std::ostringstream stream;
                            stream << "UObject::CallFunctionByNameWithArguments native inline hook resolved through the validated Linux SysV ProcessConsoleExec tail thunk at 0x"
                                   << std::hex
                                   << active->call_function_by_name_hooks.target_address();
                            detail = stream.str();
                        }
                        call_function_by_name_hook_validation = {
                                "call_function_by_name_hooks",
                                call_function_ready ? CapabilityState::available
                                                    : CapabilityState::unavailable,
                                call_function_ready ? std::move(detail)
                                                    : std::move(call_function_error),
                        };
                    }
#else
                    call_function_by_name_hook_validation = {
                            "call_function_by_name_hooks",
                            CapabilityState::disabled,
                            "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(
                            call_function_by_name_hook_validation.state ==
                                            CapabilityState::available
                                    ? "info"
                                    : "warning",
                            call_function_by_name_hook_validation.detail);
                    Capability local_player_exec_hook_validation{
                            "local_player_exec_hooks",
                            CapabilityState::unavailable,
                            "validated UObject runtime gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string local_player_exec_error;
                        const bool local_player_exec_ready =
                                active->local_player_exec_hooks.start(
                                        active->unreal,
                                        local_player_exec_error);
                        std::string detail;
                        if (local_player_exec_ready)
                        {
                            std::ostringstream stream;
                            stream << "ULocalPlayer::Exec native inline hook resolved from the validated Linux Itanium FExec secondary base +0x"
                                   << std::hex
                                   << active->local_player_exec_hooks
                                              .secondary_base_offset()
                                   << " and double-destructor-shifted slot +0x10 at 0x"
                                   << active->local_player_exec_hooks.target_address();
                            detail = stream.str();
                        }
                        local_player_exec_hook_validation = {
                                "local_player_exec_hooks",
                                local_player_exec_ready ? CapabilityState::available
                                                        : CapabilityState::unavailable,
                                local_player_exec_ready ? std::move(detail)
                                                        : std::move(local_player_exec_error),
                        };
                    }
#else
                    local_player_exec_hook_validation = {
                            "local_player_exec_hooks",
                            CapabilityState::disabled,
                            "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(
                            local_player_exec_hook_validation.state ==
                                            CapabilityState::available
                                    ? "info"
                                    : "warning",
                            local_player_exec_hook_validation.detail);
                    Capability scheduler_validation{
                            "game_thread_scheduler",
                            CapabilityState::unavailable,
                            "read-only UObject API gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (object_api_validation.success)
                    {
                        std::string scheduler_error;
                        const bool installed = active->scheduler.start(
                                active->unreal, config.resolved.game_engine_tick, scheduler_error);
                        if (installed && active->scheduler.wait_for_first_tick(std::chrono::seconds{3}))
                        {
                            scheduler_validation = {
                                    "game_thread_scheduler",
                                    CapabilityState::available,
                                    "UE 5.1 UGameEngine::Tick vtable slot matches the ELF resolver and dispatched on the game thread",
                            };
                        }
                        else
                        {
                            active->scheduler.stop();
                            scheduler_validation.detail = installed
                                                                  ? "UGameEngine::Tick hook did not observe a game-thread tick within 3 seconds"
                                                                  : scheduler_error;
                        }
                    }
#else
                    scheduler_validation = {
                            "game_thread_scheduler", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(scheduler_validation.state == CapabilityState::available ? "info" : "warning",
                                scheduler_validation.detail);
                    Capability object_notification_validation{
                            "object_notifications",
                            CapabilityState::unavailable,
                            "validated game-thread scheduler gate did not pass",
                    };
#if UE4SS_WITH_HOOK_BACKEND
                    if (scheduler_validation.state == CapabilityState::available &&
                        config.resolved.static_construct_object != 0u)
                    {
                        std::string notification_error;
                        const bool installed = active->object_notifications.start(
                                active->unreal,
                                active->scheduler,
                                config.resolved.static_construct_object,
                                notification_error);
                        object_notification_validation = {
                                "object_notifications",
                                installed ? CapabilityState::available : CapabilityState::unavailable,
                                installed
                                        ? "StaticConstructObject post-hook is active with game-thread-only Lua delivery"
                                        : std::move(notification_error),
                        };
                    }
                    else if (config.resolved.static_construct_object == 0u)
                    {
                        object_notification_validation.detail = "StaticConstructObject resolver is unavailable";
                    }
#else
                    object_notification_validation = {
                            "object_notifications", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"};
#endif
                    monitor_log(object_notification_validation.state == CapabilityState::available ? "info" : "warning",
                                object_notification_validation.detail);
                    ue4ss::linux::core::GameThreadScheduler* active_scheduler{};
                    ue4ss::linux::core::UFunctionHookManager* active_ufunction_hooks{};
                    ue4ss::linux::core::ObjectNotificationManager* active_object_notifications{};
                    ue4ss::linux::core::BlueprintHookManager* active_blueprint_hooks{};
                    ue4ss::linux::core::BeginPlayHookManager* active_begin_play_hooks{};
                    ue4ss::linux::core::EndPlayHookManager* active_end_play_hooks{};
                    ue4ss::linux::core::InitGameStateHookManager* active_init_game_state_hooks{};
                    ue4ss::linux::core::LoadMapHookManager* active_load_map_hooks{};
                    ue4ss::linux::core::ProcessConsoleExecHookManager*
                            active_process_console_exec_hooks{};
                    ue4ss::linux::core::CallFunctionByNameWithArgumentsHookManager*
                            active_call_function_by_name_hooks{};
                    ue4ss::linux::core::LocalPlayerExecHookManager*
                            active_local_player_exec_hooks{};
#if UE4SS_WITH_HOOK_BACKEND
                    if (scheduler_validation.state == CapabilityState::available)
                    {
                        active_scheduler = &active->scheduler;
                    }
                    if (ufunction_hook_validation.state == CapabilityState::available)
                    {
                        active_ufunction_hooks = &active->ufunction_hooks;
                    }
                    if (object_notification_validation.state == CapabilityState::available)
                    {
                        active_object_notifications = &active->object_notifications;
                    }
                    if (blueprint_hook_validation.state == CapabilityState::available)
                    {
                        active_blueprint_hooks = &active->blueprint_hooks;
                    }
                    if (begin_play_hook_validation.state == CapabilityState::available)
                    {
                        active_begin_play_hooks = &active->begin_play_hooks;
                    }
                    if (end_play_hook_validation.state == CapabilityState::available)
                    {
                        active_end_play_hooks = &active->end_play_hooks;
                    }
                    if (init_game_state_hook_validation.state == CapabilityState::available)
                    {
                        active_init_game_state_hooks = &active->init_game_state_hooks;
                    }
                    if (load_map_hook_validation.state == CapabilityState::available)
                    {
                        active_load_map_hooks = &active->load_map_hooks;
                    }
                    if (process_console_exec_hook_validation.state == CapabilityState::available)
                    {
                        active_process_console_exec_hooks = &active->process_console_exec_hooks;
                    }
                    if (call_function_by_name_hook_validation.state ==
                        CapabilityState::available)
                    {
                        active_call_function_by_name_hooks =
                                &active->call_function_by_name_hooks;
                    }
                    if (local_player_exec_hook_validation.state ==
                        CapabilityState::available)
                    {
                        active_local_player_exec_hooks =
                                &active->local_player_exec_hooks;
                    }
#endif
                    const Capability lua_binding_validation =
                            validate_lua_readonly_capability(
                                    active->unreal,
                                    object_api_validation,
                                    active_scheduler,
                                    active_ufunction_hooks,
                                    active_object_notifications,
                                    active_blueprint_hooks,
                                    active_begin_play_hooks,
                                    active_end_play_hooks,
                                    active_init_game_state_hooks,
                                    active_load_map_hooks,
                                    active_process_console_exec_hooks,
                                    active_call_function_by_name_hooks,
                                    active_local_player_exec_hooks);
                    monitor_log(lua_binding_validation.state == CapabilityState::available ? "info" : "warning",
                                lua_binding_validation.detail);
                    LuaModActivation lua_mod_activation =
                            activate_lua_mods(
                                    config.root_path,
                                    config.executable_path,
                                    lua_binding_validation,
                                    std::move(active));
                    publish_runtime_update(CapabilityState::available,
                                           last_validation.detail,
                                           "UEPseudo resolver state and object registry layout are ready; Lua remains unavailable",
                                           &fname_validation,
                                           &object_api_validation,
                                           &fproperty_validation,
                                           &uenum_validation,
                                           &process_event_validation,
                                           &process_internal_validation,
                                           &ufunction_hook_validation,
                                           &blueprint_hook_validation,
                                           &begin_play_hook_validation,
                                           &end_play_hook_validation,
                                           &init_game_state_hook_validation,
                                           &load_map_hook_validation,
                                           &process_console_exec_hook_validation,
                                           &call_function_by_name_hook_validation,
                                           &local_player_exec_hook_validation,
                                           &object_notification_validation,
                                           &scheduler_validation,
                                           &lua_binding_validation,
                                           &lua_mod_activation);
                    return;
                }
            }
            else
            {
                stable_since.reset();
                stable_element_count = -1;
            }
            if (last_validation.state == ObjectRegistryValidationState::unsupported)
            {
                publish_runtime_update(CapabilityState::unavailable,
                                       last_validation.detail,
                                       "the resolved Unreal object registry layout is unsupported; running fail-closed");
                return;
            }

            std::unique_lock lock{g_runtime_mutex};
            if (g_runtime_monitor_wakeup.wait_until(lock, std::min(deadline, std::chrono::steady_clock::now() + std::chrono::milliseconds{250}), [] {
                    return g_runtime_monitor_stop;
                }))
            {
                lock.unlock();
                monitor_log("info", "object-registry monitor stopped before activation");
                return;
            }
            if (std::chrono::steady_clock::now() >= deadline)
            {
                lock.unlock();
                publish_runtime_update(CapabilityState::unavailable,
                                       "object registry did not become ready within " + std::to_string(config.timeout.count()) +
                                               " seconds; last validation: " + last_validation.detail,
                                       "Unreal object registry readiness timed out; hooks and Lua remain fail-closed");
                return;
            }
        }
    }

    void initialize_platform()
    {
        RuntimeData result;
        std::optional<RegistryMonitorConfig> registry_monitor;
        {
            std::lock_guard lock{g_runtime_mutex};
            result = g_runtime;
        }

        try
        {
            std::filesystem::create_directories(result.state_path);
            ue4ss::linux::core::log_line(result, "info", "native Linux core " UE4SS_LINUX_GIT_SHA " initializing");
            result.image = ue4ss::linux::inspect_current_process();
            result.executable_path = result.image.path;

            result.capabilities = {
                    {"loader_core", CapabilityState::available, "preload shim and versioned C ABI active"},
                    {"elf_image", CapabilityState::available, "64-bit little-endian process image validated"},
                    {"build_identity", CapabilityState::available, "ELF build-id and SHA-256 collected"},
#if UE4SS_WITH_HOOK_BACKEND
                    {"hook_backend", CapabilityState::available, "Zydis inline detours and protected pointer-slot hooks passed their platform gates"},
#else
                    {"hook_backend", CapabilityState::disabled, "UE4SS_WITH_HOOK_BACKEND=OFF"},
#endif
                    {"process_event", CapabilityState::unavailable, "no validated Linux resolver is available"},
                    {"process_internal", CapabilityState::unavailable, "no validated Linux resolver is available"},
#if UE4SS_WITH_UNREAL_SOURCE_GATE
                    {"unreal_abi_headers", CapabilityState::available, "UEPseudo Linux platform plus read-only reflection sources passed the Clang source gate"},
#else
                    {"unreal_abi_headers", CapabilityState::disabled, "UE4SS_WITH_UNREAL_SOURCE_GATE=OFF"},
#endif
                    {"native_ufunction_hooks", CapabilityState::unavailable, "UFunction Func-slot runtime wiring is not enabled"},
                    {"blueprint_hooks", CapabilityState::unavailable, "ProcessInternal resolver not enabled"},
                    {"begin_play_hooks", CapabilityState::unavailable, "waiting for validated AActor CDO vtable activation"},
                    {"end_play_hooks", CapabilityState::unavailable, "waiting for validated AActor CDO vtable activation"},
                    {"init_game_state_hooks", CapabilityState::unavailable, "waiting for validated AGameModeBase vtable activation"},
                    {"load_map_hooks", CapabilityState::unavailable, "waiting for validated UGameEngine vtable activation"},
                    {"process_console_exec_hooks", CapabilityState::unavailable, "waiting for validated UObject CDO vtable activation"},
                    {"call_function_by_name_hooks", CapabilityState::unavailable, "waiting for validated ProcessConsoleExec SysV tail-thunk activation"},
                    {"local_player_exec_hooks", CapabilityState::unavailable, "waiting for validated ULocalPlayer FExec secondary-vtable activation"},
                    {"game_thread_scheduler", CapabilityState::unavailable, "waiting for validated UGameEngine::Tick vtable activation"},
                    {"lua_mods", CapabilityState::unavailable, "reflection layout activation and UE4SS Lua bindings are not enabled yet"},
                    {"cpp_mods", CapabilityState::disabled, "UE4SS_WITH_CPP_MODS=OFF"},
                    {"sdk_generator", CapabilityState::disabled, "UE4SS_WITH_SDK_GENERATOR=OFF"},
            };

#if UE4SS_WITH_LUA_RUNTIME
            const auto lua_probe = ue4ss::linux::core::probe_lua_runtime();
            result.capabilities.push_back({"lua_vm",
                                           lua_probe.success ? CapabilityState::available : CapabilityState::unavailable,
                                           lua_probe.detail});
            if (!lua_probe.success)
            {
                throw std::runtime_error("headless Lua runtime probe failed: " + lua_probe.detail);
            }

            const auto mod_catalog = ue4ss::linux::discover_lua_mods(result.root_path);
            const std::string catalog_detail = std::to_string(mod_catalog.enabled_mods.size()) +
                                               " enabled Lua mod(s) discovered without execution; " +
                                               std::to_string(mod_catalog.rejected_mods.size()) + " rejected";
            result.capabilities.push_back({"lua_mod_catalog",
                                           mod_catalog.rejected_mods.empty() ? CapabilityState::available : CapabilityState::unavailable,
                                           catalog_detail});
            for (const auto& rejected : mod_catalog.rejected_mods)
            {
                ue4ss::linux::core::log_line(result, "warning", rejected);
            }
#else
            result.capabilities.push_back({"lua_vm", CapabilityState::disabled, "UE4SS_WITH_LUA_RUNTIME=OFF"});
            result.capabilities.push_back({"lua_mod_catalog", CapabilityState::disabled, "UE4SS_WITH_LUA_RUNTIME=OFF"});
#endif

            if (result.image.machine != EM_X86_64)
            {
                throw std::runtime_error("the running executable is not an x86_64 ELF image");
            }

            if (const char* expected_hash = std::getenv("UE4SS_SERVER_SHA256"); expected_hash != nullptr && expected_hash[0] != '\0')
            {
                if (!hashes_equal(expected_hash, result.image.sha256))
                {
                    throw std::runtime_error("server SHA-256 does not match UE4SS_SERVER_SHA256; refusing to install hooks");
                }
            }

            if (should_scan_unreal(result))
            {
                ue4ss::linux::core::PatternReport pattern_report = ue4ss::linux::core::scan_unreal_patterns(result);
                result.capabilities.insert(result.capabilities.end(),
                                           std::make_move_iterator(pattern_report.capabilities.begin()),
                                           std::make_move_iterator(pattern_report.capabilities.end()));
                std::string binding_error;
#if UE4SS_WITH_UNREAL_RUNTIME
                const bool resolver_state_bound = pattern_report.critical_capabilities_available &&
                                                  ue4ss::linux::core::apply_unreal_resolver_state(pattern_report.resolved, binding_error);
#else
                const bool resolver_state_bound = false;
                binding_error = "UE4SS_WITH_UNREAL_RUNTIME=OFF";
#endif
                result.capabilities.push_back({
                        "unreal_resolver_state",
                        resolver_state_bound ? CapabilityState::available : CapabilityState::unavailable,
                        resolver_state_bound ? "validated addresses applied to dependency-light UEPseudo state"
                                             : (binding_error.empty() ? "critical resolver gate did not pass" : binding_error),
                });
                if (resolver_state_bound)
                {
                    const auto registry_validation = ue4ss::linux::core::validate_object_registry(
                            pattern_report.resolved.guobject_array,
                            pattern_report.resolved.engine_major,
                            pattern_report.resolved.engine_minor);
                    const bool registry_supported =
                            registry_validation.state != ue4ss::linux::core::ObjectRegistryValidationState::unsupported;
                    result.capabilities.push_back({
                            "object_registry_layout",
                            CapabilityState::unavailable,
                            registry_supported
                                    ? "waiting for stable post-main Unreal initialization: " + registry_validation.detail
                                    : registry_validation.detail,
                    });
                    result.capabilities.push_back({
                            "fname_runtime_abi",
                            CapabilityState::unavailable,
                            "waiting for stable post-main object registry initialization",
                    });
                    result.capabilities.push_back({
                            "lua_readonly_bindings",
                            CapabilityState::unavailable,
                            "waiting for stable post-main object registry initialization",
                    });
                    result.capabilities.push_back({
                            "readonly_object_api",
                            CapabilityState::unavailable,
                            "waiting for stable post-main object registry initialization",
                    });
                    result.capabilities.push_back({
                            "fproperty_runtime_abi",
                            CapabilityState::unavailable,
                            "waiting for stable post-main object registry initialization",
                    });
                    result.capabilities.push_back({
                            "uenum_runtime_abi",
                            CapabilityState::unavailable,
                            "waiting for stable post-main object registry initialization",
                    });
                    if (registry_supported)
                    {
                        registry_monitor = RegistryMonitorConfig{
                                .resolved = pattern_report.resolved,
                                .root_path = result.root_path,
                                .executable_path = result.executable_path,
                                .timeout = registry_monitor_timeout(),
                                .settle_time = registry_settle_time(),
                        };
                    }
                }
                else
                {
                    result.capabilities.push_back({"object_registry_layout", CapabilityState::unavailable, "resolver state is not bound"});
                    result.capabilities.push_back({"fname_runtime_abi", CapabilityState::unavailable, "resolver state is not bound"});
                    result.capabilities.push_back({"readonly_object_api", CapabilityState::unavailable, "resolver state is not bound"});
                    result.capabilities.push_back({"fproperty_runtime_abi", CapabilityState::unavailable, "resolver state is not bound"});
                    result.capabilities.push_back({"uenum_runtime_abi", CapabilityState::unavailable, "resolver state is not bound"});
                    result.capabilities.push_back({"lua_readonly_bindings", CapabilityState::unavailable, "resolver state is not bound"});
                }
                if (!resolver_state_bound && result.required)
                {
                    result.state = UE4SS_STATE_FAILED;
                    result.last_error = ENOTSUP;
                    result.message = "one or more critical Unreal ELF resolvers are unavailable and UE4SS_REQUIRED is true";
                }
                else
                {
                    result.state = UE4SS_STATE_DEGRADED;
                    result.message = resolver_state_bound
                                             ? "Unreal ELF resolvers are bound; waiting for stable post-main object registry validation"
                                             : "one or more critical Unreal ELF resolvers are unavailable; running fail-closed";
                }
            }
            else
            {
                result.capabilities.push_back({"unreal_pattern_scan", CapabilityState::disabled, "not an identified Palworld server process"});
                result.capabilities.push_back({"object_registry", CapabilityState::unavailable, "scan not requested"});
                result.capabilities.push_back({"static_find_object", CapabilityState::unavailable, "scan not requested"});
                result.state = UE4SS_STATE_PLATFORM_READY;
                result.message = "Linux platform bootstrap ready; Unreal and Lua capabilities remain fail-closed";
            }
            ue4ss::linux::core::log_line(result, "info", "ELF build-id=" + result.image.build_id + " sha256=" + result.image.sha256);
            ue4ss::linux::core::log_line(result, "info", result.message);
        }
        catch (const std::exception& error)
        {
            result.state = UE4SS_STATE_FAILED;
            result.last_error = errno != 0 ? errno : EINVAL;
            result.message = error.what();
            ue4ss::linux::core::log_line(result, "error", result.message);
        }
        catch (...)
        {
            result.state = UE4SS_STATE_FAILED;
            result.last_error = EFAULT;
            result.message = "unknown exception during Linux platform initialization";
            ue4ss::linux::core::log_line(result, "error", result.message);
        }

        try
        {
            ue4ss::linux::core::write_status_file(result);
        }
        catch (const std::exception& error)
        {
            result.state = UE4SS_STATE_FAILED;
            result.last_error = errno != 0 ? errno : EIO;
            result.message = std::string{"cannot publish UE4SS status: "} + error.what();
            ue4ss::linux::core::log_line(result, "error", result.message);
        }

        bool monitor_start_failed = false;
        {
            std::lock_guard lock{g_runtime_mutex};
            g_runtime = std::move(result);
            g_initialization_complete = true;
            if (registry_monitor.has_value() && g_runtime.state != UE4SS_STATE_FAILED)
            {
                try
                {
                    g_runtime_monitor_stop = false;
                    g_runtime_monitor_thread = std::thread{monitor_object_registry, *registry_monitor};
                }
                catch (...)
                {
                    g_runtime_monitor_stop = true;
                    monitor_start_failed = true;
                    set_capability(g_runtime,
                                   "object_registry_layout",
                                   CapabilityState::unavailable,
                                   "cannot start the bounded object-registry readiness monitor");
                    g_runtime.message = "object-registry readiness monitor could not be started; runtime features remain fail-closed";
                    if (g_runtime.required)
                    {
                        g_runtime.state = UE4SS_STATE_FAILED;
                        g_runtime.last_error = EAGAIN;
                    }
                }
            }
        }
        g_initialization_finished.notify_all();
        if (monitor_start_failed)
        {
            RuntimeData snapshot;
            {
                std::lock_guard lock{g_runtime_mutex};
                snapshot = g_runtime;
            }
            ue4ss::linux::core::log_line(snapshot, "error", snapshot.message);
            try
            {
                ue4ss::linux::core::write_status_file(snapshot);
            }
            catch (...)
            {
            }
        }
    }
} // namespace

extern "C" UE4SS_API int ue4ss_start(const ue4ss_boot_config* config)
{
    try
    {
        if (config == nullptr || config->struct_size < sizeof(ue4ss_boot_config) || config->abi_version != UE4SS_LINUX_ABI_VERSION)
        {
            return EPROTO;
        }

        std::unique_lock lock{g_runtime_mutex};
        if (g_runtime.state != UE4SS_STATE_NOT_STARTED && g_runtime.state != UE4SS_STATE_STOPPED)
        {
            return EALREADY;
        }
        g_runtime = {};
        g_runtime.state = UE4SS_STATE_INITIALIZING;
        g_runtime.required = config->required != 0;
        g_runtime.root_path = copy_optional(config->root_path);
        g_runtime.state_path = copy_optional(config->state_path);
        g_runtime.executable_path = copy_optional(config->executable_path);
        if (g_runtime.root_path.empty() || g_runtime.state_path.empty())
        {
            g_runtime.state = UE4SS_STATE_FAILED;
            g_runtime.last_error = EINVAL;
            g_runtime.message = "root_path and state_path are required";
            return EINVAL;
        }
        g_initialization_complete = false;
        g_runtime_monitor_stop = false;
        g_initialization_thread = std::thread{initialize_platform};
        if (!g_runtime.required)
        {
            return 0;
        }
        g_initialization_finished.wait(lock, [] {
            return g_initialization_complete;
        });
        const int result = g_runtime.state == UE4SS_STATE_FAILED ? (g_runtime.last_error != 0 ? g_runtime.last_error : EIO) : 0;
        lock.unlock();
        if (g_initialization_thread.joinable())
        {
            g_initialization_thread.join();
        }
        return result;
    }
    catch (...)
    {
        return EFAULT;
    }
}

extern "C" UE4SS_API int ue4ss_get_status(ue4ss_status* status)
{
    if (status == nullptr)
    {
        return EINVAL;
    }
    std::lock_guard lock{g_runtime_mutex};
    ue4ss::linux::core::copy_public_status(g_runtime, *status);
    return 0;
}

extern "C" UE4SS_API void ue4ss_shutdown(void)
{
    try
    {
        if (g_initialization_thread.joinable())
        {
            g_initialization_thread.join();
        }
        {
            std::lock_guard lock{g_runtime_mutex};
            g_runtime_monitor_stop = true;
        }
        g_runtime_monitor_wakeup.notify_all();
        if (g_runtime_monitor_thread.joinable())
        {
            g_runtime_monitor_thread.join();
        }
        {
            std::lock_guard lock{g_runtime_mutex};
            g_active_lua_runtime.reset();
            if (g_runtime.state != UE4SS_STATE_NOT_STARTED && g_runtime.state != UE4SS_STATE_STOPPED)
            {
                g_runtime.state = UE4SS_STATE_STOPPED;
                g_runtime.message = "UE4SS Linux core stopped";
                ue4ss::linux::core::log_line(g_runtime, "info", g_runtime.message);
                ue4ss::linux::core::write_status_file(g_runtime);
            }
        }
    }
    catch (...)
    {
    }
}
