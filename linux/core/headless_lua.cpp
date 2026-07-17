#include "headless_lua.hpp"

#include "game_thread_scheduler.hpp"
#include "object_notifications.hpp"
#include "blueprint_hooks.hpp"
#include "begin_play_hooks.hpp"
#include "load_map_hooks.hpp"
#include "console_exec_hooks.hpp"
#include "ufunction_hooks.hpp"
#include "unreal_readonly.hpp"
#include "ue4ss/utf.hpp"

#include <Input/KeyDef.hpp>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <exception>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <unistd.h>

#ifndef UE4SS_LINUX_VERSION_MAJOR
#define UE4SS_LINUX_VERSION_MAJOR 0
#define UE4SS_LINUX_VERSION_MINOR 0
#define UE4SS_LINUX_VERSION_HOTFIX 0
#endif

namespace
{
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

    void prepend_script_module_paths(
            lua_State* state,
            const std::filesystem::path& script_path)
    {
        const int original_top = lua_gettop(state);
        lua_getglobal(state, "package");
        if (lua_type(state, -1) != LUA_TTABLE)
        {
            lua_settop(state, original_top);
            return;
        }

        lua_getfield(state, -1, "path");
        std::size_t existing_length{};
        const char* existing_path = lua_tolstring(state, -1, &existing_length);
        const std::string scripts_directory =
                script_path.parent_path().lexically_normal().generic_string();
        std::string package_path;
        package_path.reserve(
                scripts_directory.size() * 2u + existing_length + 24u);
        package_path.append(scripts_directory).append("/?.lua;");
        package_path.append(scripts_directory).append("/?/init.lua;");
        if (existing_path != nullptr)
        {
            package_path.append(existing_path, existing_length);
        }
        lua_pop(state, 1);
        lua_pushlstring(state, package_path.data(), package_path.size());
        lua_setfield(state, -2, "path");
        lua_settop(state, original_top);
    }

    constexpr const char* k_runtime_registry_key = "UE4SS.ReadOnlyUnrealRuntime";
    constexpr const char* k_scheduler_registry_key = "UE4SS.GameThreadScheduler";
    constexpr const char* k_hooks_registry_key = "UE4SS.UFunctionHookManager";
    constexpr const char* k_notifications_registry_key = "UE4SS.ObjectNotificationManager";
    constexpr const char* k_blueprint_hooks_registry_key = "UE4SS.BlueprintHookManager";
    constexpr const char* k_begin_play_hooks_registry_key = "UE4SS.BeginPlayHookManager";
    constexpr const char* k_end_play_hooks_registry_key = "UE4SS.EndPlayHookManager";
    constexpr const char* k_init_game_state_hooks_registry_key = "UE4SS.InitGameStateHookManager";
    constexpr const char* k_load_map_hooks_registry_key = "UE4SS.LoadMapHookManager";
    constexpr const char* k_process_console_exec_hooks_registry_key =
            "UE4SS.ProcessConsoleExecHookManager";
    constexpr const char* k_call_function_by_name_hooks_registry_key =
            "UE4SS.CallFunctionByNameWithArgumentsHookManager";
    constexpr const char* k_local_player_exec_hooks_registry_key =
            "UE4SS.LocalPlayerExecHookManager";
    constexpr const char* k_state_owner_registry_key = "UE4SS.HeadlessLuaState";
    constexpr const char* k_uobject_metatable = "UE4SS.ReadOnlyUObject";
    constexpr const char* k_remote_param_metatable = "UE4SS.RemoteUnrealParam";
    constexpr const char* k_fname_metatable = "UE4SS.FName";
    constexpr const char* k_fstring_metatable = "UE4SS.FString";
    constexpr const char* k_futf8string_metatable = "UE4SS.FUtf8String";
    constexpr const char* k_fansistring_metatable = "UE4SS.FAnsiString";
    constexpr const char* k_ftext_metatable = "UE4SS.FText";
    constexpr const char* k_furl_metatable = "UE4SS.FURL";
    constexpr const char* k_output_device_metatable = "UE4SS.FOutputDevice";
    constexpr const char* k_tarray_metatable = "UE4SS.TArray";
    constexpr const char* k_tset_metatable = "UE4SS.TSet";
    constexpr const char* k_tmap_metatable = "UE4SS.TMap";
    constexpr const char* k_local_value_param_metatable = "UE4SS.LocalUnrealParam";
    constexpr const char* k_fproperty_metatable = "UE4SS.FProperty";
    constexpr const char* k_uscript_struct_metatable = "UE4SS.UScriptStruct";
    constexpr const char* k_ffield_class_metatable = "UE4SS.FFieldClass";
    constexpr const char* k_weak_object_metatable = "UE4SS.FWeakObjectPtr";
    constexpr const char* k_multicast_delegate_metatable = "UE4SS.MulticastDelegateProperty";
    constexpr const char* k_sparse_multicast_delegate_metatable =
            "UE4SS.MulticastSparseDelegateProperty";
    constexpr const char* k_soft_object_metatable = "UE4SS.TSoftObjectPtrUserdata";
    constexpr const char* k_soft_object_path_metatable = "UE4SS.FSoftObjectPathUserdata";
    constexpr const char* k_thread_id_metatable = "ThreadIdUserdata";
    constexpr const char* k_game_executable_directory_registry_key =
            "UE4SS.GameExecutableDirectory";
    constexpr const char* k_ue4ss_root_directory_registry_key =
            "UE4SS.RootDirectory";

    struct LuaReadOnlyUObject
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle handle;
    };

    struct LuaRemoteObjectParam
    {
        ue4ss::linux::core::HeadlessLuaState* owner{};
        std::uint64_t epoch{};
        bool context_parameter{};
        ue4ss::linux::core::ReadOnlyUObjectHandle handle;
        ue4ss::linux::core::RemoteUnrealParameter parameter;
    };

    struct LuaFName
    {
        ue4ss::linux::core::RawFName value;
        std::string text;
    };

    struct LuaFString
    {
        std::string value;
    };

    struct LuaFURL
    {
        ue4ss::linux::core::UnrealFURLSnapshot value;
    };

    struct LuaFOutputDevice
    {
        ue4ss::linux::core::HeadlessLuaState* owner{};
        std::uint64_t epoch{};
        std::uint64_t address{};
    };

    enum class NarrowStringKind : std::uint8_t
    {
        utf8,
        ansi,
    };

    struct LuaNarrowString
    {
        std::string value;
        NarrowStringKind kind{NarrowStringKind::utf8};
    };

    // Never retain an engine-owned FText or shared-reference pointer in Lua.
    // The value is an immutable UTF-8 snapshot which is materialized into an
    // engine FText only by the validated property/UFunction lifecycle bridge.
    struct LuaFText
    {
        std::string value;
    };

    struct LuaTArray
    {
        std::vector<std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue>> elements;
    };

    struct LuaTSet
    {
        std::vector<std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue>> elements;
    };

    struct LuaTMap
    {
        std::vector<ue4ss::linux::core::UnrealMapEntryValue> entries;
    };

    struct LuaLocalValueParam
    {
        std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue> value;
    };

    struct LuaFProperty
    {
        ue4ss::linux::core::ReflectedPropertyDescription description;
    };

    struct LuaUScriptStruct
    {
        ue4ss::linux::core::UnrealStructBinding binding;
    };

    struct LuaFFieldClass
    {
        std::string name;
    };

    struct LuaWeakObjectPtr
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle handle;
        bool is_null{};
    };

    struct LuaMulticastDelegate
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle owner;
        std::string property_name;
        bool sparse{};
    };

    struct LuaSoftObjectPtr
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle weak_object;
        bool weak_is_null{};
        std::int32_t tag_at_last_test{};
        ue4ss::linux::core::RawFName asset_path_name;
        std::string asset_path_text;
        ue4ss::linux::core::RawFName package_name;
        ue4ss::linux::core::RawFName asset_name;
        std::string package_text;
        std::string asset_text;
        std::string sub_path;
    };

    struct LuaSoftObjectPath
    {
        ue4ss::linux::core::RawFName asset_path_name;
        std::string asset_path_text;
        ue4ss::linux::core::RawFName package_name;
        ue4ss::linux::core::RawFName asset_name;
        std::string package_text;
        std::string asset_text;
        std::string sub_path;
    };

    struct LuaThreadId
    {
        std::thread::id value;
    };

    int push_property_value(lua_State* state,
                            const ue4ss::linux::core::UnrealPropertyValue& value) noexcept;
    int push_fname(lua_State* state,
                   ue4ss::linux::core::RawFName value,
                   std::string text) noexcept;
    int push_fstring(lua_State* state, std::string value) noexcept;
    int push_furl(lua_State* state,
                  const ue4ss::linux::core::UnrealFURLSnapshot& value) noexcept;
    int push_output_device(lua_State* state,
                           ue4ss::linux::core::HeadlessLuaState& owner,
                           std::uint64_t epoch,
                           std::uint64_t address) noexcept;
    int push_narrow_string(lua_State* state,
                           std::string value,
                           NarrowStringKind kind) noexcept;
    int push_ftext(lua_State* state, std::string value) noexcept;
    int push_weak_object(lua_State* state,
                         const ue4ss::linux::core::ReadOnlyUObjectHandle& handle,
                         bool is_null) noexcept;
    int push_struct_binding(
            lua_State* state,
            const ue4ss::linux::core::UnrealStructBinding& binding) noexcept;

    [[nodiscard]] ue4ss::linux::core::ReadOnlyUnrealRuntime* get_runtime(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_runtime_registry_key);
        auto* runtime = static_cast<ue4ss::linux::core::ReadOnlyUnrealRuntime*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return runtime;
    }

    [[nodiscard]] ue4ss::linux::core::GameThreadScheduler* get_scheduler(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_scheduler_registry_key);
        auto* scheduler = static_cast<ue4ss::linux::core::GameThreadScheduler*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return scheduler;
    }

    [[nodiscard]] ue4ss::linux::core::UFunctionHookManager* get_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::UFunctionHookManager*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::ObjectNotificationManager* get_notifications(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_notifications_registry_key);
        auto* notifications = static_cast<ue4ss::linux::core::ObjectNotificationManager*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return notifications;
    }

    [[nodiscard]] ue4ss::linux::core::BlueprintHookManager* get_blueprint_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_blueprint_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::BlueprintHookManager*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::BeginPlayHookManager* get_begin_play_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_begin_play_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::BeginPlayHookManager*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::EndPlayHookManager* get_end_play_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_end_play_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::EndPlayHookManager*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::InitGameStateHookManager* get_init_game_state_hooks(
            lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_init_game_state_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::InitGameStateHookManager*>(
                lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::LoadMapHookManager* get_load_map_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_load_map_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::LoadMapHookManager*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::ProcessConsoleExecHookManager*
    get_process_console_exec_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_process_console_exec_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::ProcessConsoleExecHookManager*>(
                lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::CallFunctionByNameWithArgumentsHookManager*
    get_call_function_by_name_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_call_function_by_name_hooks_registry_key);
        auto* hooks = static_cast<
                ue4ss::linux::core::CallFunctionByNameWithArgumentsHookManager*>(
                lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::LocalPlayerExecHookManager*
    get_local_player_exec_hooks(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_local_player_exec_hooks_registry_key);
        auto* hooks = static_cast<ue4ss::linux::core::LocalPlayerExecHookManager*>(
                lua_touserdata(state, -1));
        lua_pop(state, 1);
        return hooks;
    }

    [[nodiscard]] ue4ss::linux::core::HeadlessLuaState* get_state_owner(lua_State* state) noexcept
    {
        lua_getfield(state, LUA_REGISTRYINDEX, k_state_owner_registry_key);
        auto* owner = static_cast<ue4ss::linux::core::HeadlessLuaState*>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return owner;
    }

    [[nodiscard]] LuaReadOnlyUObject* get_object(lua_State* state, int index) noexcept
    {
        if (lua_type(state, index) != LUA_TUSERDATA)
        {
            return nullptr;
        }
        void* value = luaL_testudata(state, index, k_uobject_metatable);
        return static_cast<LuaReadOnlyUObject*>(value);
    }

    [[nodiscard]] LuaFName* get_fname(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFName*>(luaL_testudata(state, index, k_fname_metatable));
    }

    [[nodiscard]] LuaFString* get_fstring(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFString*>(luaL_testudata(state, index, k_fstring_metatable));
    }

    [[nodiscard]] LuaFURL* get_furl(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFURL*>(luaL_testudata(state, index, k_furl_metatable));
    }

    [[nodiscard]] LuaFOutputDevice* get_output_device(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFOutputDevice*>(
                luaL_testudata(state, index, k_output_device_metatable));
    }

    [[nodiscard]] LuaNarrowString* get_narrow_string(lua_State* state, int index) noexcept
    {
        if (auto* value = static_cast<LuaNarrowString*>(
                    luaL_testudata(state, index, k_futf8string_metatable));
            value != nullptr)
        {
            return value;
        }
        return static_cast<LuaNarrowString*>(
                luaL_testudata(state, index, k_fansistring_metatable));
    }

    [[nodiscard]] LuaFText* get_ftext(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFText*>(luaL_testudata(state, index, k_ftext_metatable));
    }

    [[nodiscard]] LuaTArray* get_tarray(lua_State* state, int index) noexcept
    {
        return static_cast<LuaTArray*>(luaL_testudata(state, index, k_tarray_metatable));
    }

    [[nodiscard]] LuaTSet* get_tset(lua_State* state, int index) noexcept
    {
        return static_cast<LuaTSet*>(luaL_testudata(state, index, k_tset_metatable));
    }

    [[nodiscard]] LuaTMap* get_tmap(lua_State* state, int index) noexcept
    {
        return static_cast<LuaTMap*>(luaL_testudata(state, index, k_tmap_metatable));
    }

    [[nodiscard]] LuaLocalValueParam* get_local_value_param(lua_State* state, int index) noexcept
    {
        return static_cast<LuaLocalValueParam*>(luaL_testudata(state, index, k_local_value_param_metatable));
    }

    [[nodiscard]] LuaFProperty* get_fproperty(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFProperty*>(luaL_testudata(state, index, k_fproperty_metatable));
    }

    [[nodiscard]] LuaUScriptStruct* get_uscript_struct(lua_State* state, int index) noexcept
    {
        return static_cast<LuaUScriptStruct*>(
                luaL_testudata(state, index, k_uscript_struct_metatable));
    }

    [[nodiscard]] LuaFFieldClass* get_ffield_class(lua_State* state, int index) noexcept
    {
        return static_cast<LuaFFieldClass*>(luaL_testudata(state, index, k_ffield_class_metatable));
    }

    [[nodiscard]] LuaWeakObjectPtr* get_weak_object(lua_State* state, int index) noexcept
    {
        return static_cast<LuaWeakObjectPtr*>(luaL_testudata(state, index, k_weak_object_metatable));
    }

    [[nodiscard]] LuaMulticastDelegate* get_multicast_delegate(lua_State* state, int index) noexcept
    {
        if (auto* value = static_cast<LuaMulticastDelegate*>(
                    luaL_testudata(state, index, k_multicast_delegate_metatable));
            value != nullptr)
        {
            return value;
        }
        return static_cast<LuaMulticastDelegate*>(
                luaL_testudata(state, index, k_sparse_multicast_delegate_metatable));
    }

    [[nodiscard]] LuaSoftObjectPtr* get_soft_object(lua_State* state, int index) noexcept
    {
        return static_cast<LuaSoftObjectPtr*>(luaL_testudata(state, index, k_soft_object_metatable));
    }

    [[nodiscard]] LuaSoftObjectPath* get_soft_object_path(lua_State* state, int index) noexcept
    {
        return static_cast<LuaSoftObjectPath*>(
                luaL_testudata(state, index, k_soft_object_path_metatable));
    }

    [[nodiscard]] LuaThreadId* get_thread_id(lua_State* state, int index) noexcept
    {
        return static_cast<LuaThreadId*>(luaL_testudata(state, index, k_thread_id_metatable));
    }

    [[nodiscard]] bool lua_to_untyped_property_value(
            lua_State* state,
            int index,
            ue4ss::linux::core::UnrealPropertyValue& output,
            std::string& error,
            std::size_t depth = 0u) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        const int original_top = lua_gettop(state);
        try
        {
            output = {};
            switch (lua_type(state, index))
            {
            case LUA_TNIL:
                output.kind = UnrealPropertyKind::object;
                output.object_is_null = true;
                return true;
            case LUA_TBOOLEAN:
                output.kind = UnrealPropertyKind::boolean;
                output.boolean = lua_toboolean(state, index) != 0;
                return true;
            case LUA_TNUMBER:
                if (lua_isinteger(state, index))
                {
                    output.kind = UnrealPropertyKind::signed_integer;
                    output.signed_integer = static_cast<std::int64_t>(lua_tointeger(state, index));
                }
                else
                {
                    output.kind = UnrealPropertyKind::floating_point;
                    output.floating_point = static_cast<double>(lua_tonumber(state, index));
                }
                return true;
            case LUA_TSTRING:
            {
                std::size_t length{};
                const char* text = lua_tolstring(state, index, &length);
                output.kind = UnrealPropertyKind::string;
                output.text.assign(text, length);
                return true;
            }
            case LUA_TUSERDATA:
                if (auto* object = get_object(state, index); object != nullptr)
                {
                    output.kind = UnrealPropertyKind::object;
                    output.object = object->handle;
                    return true;
                }
                if (auto* name = get_fname(state, index); name != nullptr)
                {
                    output.kind = UnrealPropertyKind::name;
                    output.name = name->value;
                    output.text = name->text;
                    return true;
                }
                if (auto* string = get_fstring(state, index); string != nullptr)
                {
                    output.kind = UnrealPropertyKind::string;
                    output.text = string->value;
                    return true;
                }
                if (auto* text_value = get_ftext(state, index); text_value != nullptr)
                {
                    output.kind = UnrealPropertyKind::text;
                    output.text = text_value->value;
                    return true;
                }
                if (auto* array = get_tarray(state, index); array != nullptr)
                {
                    output.kind = UnrealPropertyKind::array;
                    output.array_elements = array->elements;
                    return true;
                }
                if (auto* set = get_tset(state, index); set != nullptr)
                {
                    output.kind = UnrealPropertyKind::set;
                    output.set_elements = set->elements;
                    return true;
                }
                if (auto* map = get_tmap(state, index); map != nullptr)
                {
                    output.kind = UnrealPropertyKind::map;
                    output.map_entries = map->entries;
                    return true;
                }
                if (auto* unreal_struct = get_uscript_struct(state, index);
                    unreal_struct != nullptr)
                {
                    auto* runtime = get_runtime(state);
                    if (runtime == nullptr ||
                        !runtime->read_struct_value(
                                unreal_struct->binding, output, error))
                    {
                        error = error.empty()
                                        ? "UScriptStruct userdata no longer maps to live storage"
                                        : error;
                        return false;
                    }
                    return true;
                }
                if (auto* weak = get_weak_object(state, index); weak != nullptr)
                {
                    output.kind = UnrealPropertyKind::weak_object;
                    output.object = weak->handle;
                    output.object_is_null = weak->is_null;
                    return true;
                }
                if (auto* soft = get_soft_object(state, index); soft != nullptr)
                {
                    output.kind = UnrealPropertyKind::soft_object;
                    output.object = soft->weak_object;
                    output.object_is_null = soft->weak_is_null;
                    output.signed_integer = soft->tag_at_last_test;
                    output.name = soft->asset_path_name;
                    output.text = soft->asset_path_text;
                    output.soft_package_name = soft->package_name;
                    output.soft_asset_name = soft->asset_name;
                    output.soft_package_text = soft->package_text;
                    output.soft_asset_text = soft->asset_text;
                    output.soft_sub_path = soft->sub_path;
                    return true;
                }
                if (auto* delegate = get_multicast_delegate(state, index); delegate != nullptr)
                {
                    auto* runtime = get_runtime(state);
                    if (runtime == nullptr ||
                        !runtime->read_property(
                                delegate->owner, delegate->property_name, output, error) ||
                        output.kind != UnrealPropertyKind::multicast_delegate)
                    {
                        error = error.empty()
                                        ? "live multicast delegate wrapper no longer references a compatible property"
                                        : error;
                        return false;
                    }
                    return true;
                }
                error = "unsupported userdata in reflected value";
                return false;
            case LUA_TTABLE:
            {
                if (depth >= 32u)
                {
                    error = "Lua struct table nesting exceeds the safety limit";
                    return false;
                }
                const int absolute_index = lua_absindex(state, index);
                bool saw_named_field{};
                bool saw_indexed_element{};
                std::vector<std::pair<lua_Integer, std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue>>>
                        indexed_elements;
                lua_pushnil(state);
                while (lua_next(state, absolute_index) != 0)
                {
                    const bool named = lua_type(state, -2) == LUA_TSTRING;
                    const bool indexed = lua_isinteger(state, -2);
                    if ((!named && !indexed) || (named && saw_indexed_element) || (indexed && saw_named_field))
                    {
                        lua_settop(state, original_top);
                        error = "reflected tables must use either string struct fields or contiguous integer array indices";
                        return false;
                    }
                    auto value = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                    if (!lua_to_untyped_property_value(state, -1, *value, error, depth + 1u))
                    {
                        lua_settop(state, original_top);
                        return false;
                    }
                    if (named)
                    {
                        saw_named_field = true;
                        std::size_t name_length{};
                        const char* name = lua_tolstring(state, -2, &name_length);
                        output.struct_fields.push_back({std::string{name, name_length}, std::move(value)});
                    }
                    else
                    {
                        saw_indexed_element = true;
                        const lua_Integer element_index = lua_tointeger(state, -2);
                        if (element_index < 1)
                        {
                            lua_settop(state, original_top);
                            error = "ArrayProperty table indices must start at one";
                            return false;
                        }
                        indexed_elements.emplace_back(element_index, std::move(value));
                    }
                    lua_pop(state, 1);
                }
                if (saw_indexed_element)
                {
                    std::sort(indexed_elements.begin(), indexed_elements.end(), [](const auto& left, const auto& right) {
                        return left.first < right.first;
                    });
                    output.kind = UnrealPropertyKind::array;
                    output.array_elements.reserve(indexed_elements.size());
                    for (std::size_t element = 0; element < indexed_elements.size(); ++element)
                    {
                        if (indexed_elements[element].first != static_cast<lua_Integer>(element + 1u))
                        {
                            lua_settop(state, original_top);
                            error = "ArrayProperty table indices must be contiguous";
                            return false;
                        }
                        output.array_elements.push_back(std::move(indexed_elements[element].second));
                    }
                }
                else
                {
                    output.kind = UnrealPropertyKind::structure;
                }
                return true;
            }
            default:
                error = "reflected value type is not supported";
                return false;
            }
        }
        catch (const std::exception& exception)
        {
            lua_settop(state, original_top);
            error = exception.what();
            return false;
        }
        catch (...)
        {
            lua_settop(state, original_top);
            error = "unknown exception while converting a Lua reflected value";
            return false;
        }
    }

    int push_tarray(
            lua_State* state,
            const std::vector<std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue>>& elements) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaTArray), 0);
            new (storage) LuaTArray{elements};
            luaL_getmetatable(state, k_tarray_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "TArray construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "TArray construction failed with an unknown exception");
        }
    }

    int push_tset(
            lua_State* state,
            const std::vector<std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue>>& elements) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaTSet), 0);
            new (storage) LuaTSet{elements};
            luaL_getmetatable(state, k_tset_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "TSet construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "TSet construction failed with an unknown exception");
        }
    }

    int push_tmap(lua_State* state,
                  const std::vector<ue4ss::linux::core::UnrealMapEntryValue>& entries) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaTMap), 0);
            new (storage) LuaTMap{entries};
            luaL_getmetatable(state, k_tmap_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "TMap construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "TMap construction failed with an unknown exception");
        }
    }

    int push_soft_object_path(lua_State* state,
                              ue4ss::linux::core::RawFName asset_path_name,
                              std::string asset_path_text,
                              ue4ss::linux::core::RawFName package_name,
                              ue4ss::linux::core::RawFName asset_name,
                              std::string package_text,
                              std::string asset_text,
                              std::string sub_path) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaSoftObjectPath), 0);
            new (storage) LuaSoftObjectPath{
                    asset_path_name,
                    std::move(asset_path_text),
                    package_name,
                    asset_name,
                    std::move(package_text),
                    std::move(asset_text),
                    std::move(sub_path),
            };
            luaL_getmetatable(state, k_soft_object_path_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "FSoftObjectPath construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "FSoftObjectPath construction failed with an unknown exception");
        }
    }

    int push_soft_object(lua_State* state,
                         const ue4ss::linux::core::UnrealPropertyValue& value) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaSoftObjectPtr), 0);
            new (storage) LuaSoftObjectPtr{
                    value.object,
                    value.object_is_null,
                    static_cast<std::int32_t>(value.signed_integer),
                    value.name,
                    value.text,
                    value.soft_package_name,
                    value.soft_asset_name,
                    value.soft_package_text,
                    value.soft_asset_text,
                    value.soft_sub_path,
            };
            luaL_getmetatable(state, k_soft_object_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "TSoftObjectPtr construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "TSoftObjectPtr construction failed with an unknown exception");
        }
    }

    int lua_soft_object_gc(lua_State* state) noexcept
    {
        if (auto* soft = get_soft_object(state, 1); soft != nullptr) soft->~LuaSoftObjectPtr();
        return 0;
    }

    int lua_soft_object_path_gc(lua_State* state) noexcept
    {
        if (auto* path = get_soft_object_path(state, 1); path != nullptr) path->~LuaSoftObjectPath();
        return 0;
    }

    int lua_soft_object_get_weak_ptr(lua_State* state) noexcept
    {
        auto* soft = get_soft_object(state, 1);
        if (soft == nullptr) return luaL_error(state, "TSoftObjectPtr:GetWeakPtr requires userdata");
        return push_weak_object(state, soft->weak_object, soft->weak_is_null);
    }

    int lua_soft_object_get_tag(lua_State* state) noexcept
    {
        auto* soft = get_soft_object(state, 1);
        if (soft == nullptr) return luaL_error(state, "TSoftObjectPtr:GetTagAtLastTest requires userdata");
        lua_pushinteger(state, static_cast<lua_Integer>(soft->tag_at_last_test));
        return 1;
    }

    int lua_soft_object_get_object_id(lua_State* state) noexcept
    {
        auto* soft = get_soft_object(state, 1);
        if (soft == nullptr) return luaL_error(state, "TSoftObjectPtr:GetObjectID requires userdata");
        return push_soft_object_path(
                state,
                soft->asset_path_name,
                soft->asset_path_text,
                soft->package_name,
                soft->asset_name,
                soft->package_text,
                soft->asset_text,
                soft->sub_path);
    }

    int lua_soft_object_type(lua_State* state) noexcept
    {
        if (get_soft_object(state, 1) == nullptr)
        {
            return luaL_error(state, "TSoftObjectPtr:type requires userdata");
        }
        lua_pushliteral(state, "TSoftObjectPtrUserdata");
        return 1;
    }

    int lua_soft_object_path_get_asset_name(lua_State* state) noexcept
    {
        auto* path = get_soft_object_path(state, 1);
        if (path == nullptr) return luaL_error(state, "FSoftObjectPath:GetAssetPathName requires userdata");
        if (path->asset_path_name.comparison_index == 0u && path->asset_path_name.number == 0u &&
            path->asset_path_text != "None")
        {
            auto* runtime = get_runtime(state);
            std::string error;
            if (runtime == nullptr ||
                !runtime->fname_from_utf8(path->asset_path_text, path->asset_path_name, true, error))
            {
                return luaL_error(state,
                                  "FSoftObjectPath:GetAssetPathName failed: %s",
                                  error.empty() ? "validated FName runtime is unavailable" : error.c_str());
            }
        }
        return push_fname(state, path->asset_path_name, path->asset_path_text);
    }

    int lua_soft_object_path_get_sub_path(lua_State* state) noexcept
    {
        auto* path = get_soft_object_path(state, 1);
        if (path == nullptr) return luaL_error(state, "FSoftObjectPath:GetSubPathString requires userdata");
        return push_fstring(state, path->sub_path);
    }

    int lua_soft_object_path_type(lua_State* state) noexcept
    {
        if (get_soft_object_path(state, 1) == nullptr)
        {
            return luaL_error(state, "FSoftObjectPath:type requires userdata");
        }
        lua_pushliteral(state, "FSoftObjectPathUserdata");
        return 1;
    }

    int push_live_multicast_delegate(
            lua_State* state,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            bool sparse) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaMulticastDelegate), 0);
            new (storage) LuaMulticastDelegate{owner, std::string{property_name}, sparse};
            luaL_getmetatable(
                    state,
                    sparse ? k_sparse_multicast_delegate_metatable
                           : k_multicast_delegate_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "multicast delegate wrapper construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "multicast delegate wrapper construction failed with an unknown exception");
        }
    }

    [[nodiscard]] bool lua_to_delegate_function_name(
            lua_State* state,
            int index,
            ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            ue4ss::linux::core::RawFName& output,
            std::string& error) noexcept
    {
        if (auto* name = get_fname(state, index); name != nullptr)
        {
            output = name->value;
            return true;
        }
        if (lua_type(state, index) == LUA_TSTRING)
        {
            std::size_t length{};
            const char* value = lua_tolstring(state, index, &length);
            return runtime.fname_from_utf8(std::string_view{value, length}, output, true, error);
        }
        error = "delegate function name must be a string or FName";
        return false;
    }

    [[nodiscard]] bool lua_to_delegate_assignment(
            lua_State* state,
            int index,
            ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            ue4ss::linux::core::UnrealPropertyValue& output,
            std::string& error) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        output = {};
        output.kind = UnrealPropertyKind::delegate;
        if (lua_isnil(state, index))
        {
            output.object_is_null = true;
            return true;
        }
        if (lua_type(state, index) != LUA_TTABLE)
        {
            error = "DelegateProperty assignment requires nil or {Object = UObject, FunctionName = FName}";
            return false;
        }
        const int original_top = lua_gettop(state);
        const int table_index = lua_absindex(state, index);
        lua_getfield(state, table_index, "Object");
        if (lua_isnil(state, -1))
        {
            output.object_is_null = true;
            lua_settop(state, original_top);
            return true;
        }
        auto* object = get_object(state, -1);
        if (object == nullptr)
        {
            lua_settop(state, original_top);
            error = "DelegateProperty Object must be nil or a UObject";
            return false;
        }
        output.object = object->handle;
        lua_pop(state, 1);
        lua_getfield(state, table_index, "FunctionName");
        if (auto* name = get_fname(state, -1); name != nullptr)
        {
            output.name = name->value;
            output.text = name->text;
        }
        else if (lua_type(state, -1) == LUA_TSTRING)
        {
            std::size_t length{};
            const char* name = lua_tolstring(state, -1, &length);
            output.text.assign(name, length);
            if (!runtime.fname_from_utf8(output.text, output.name, true, error))
            {
                lua_settop(state, original_top);
                return false;
            }
        }
        else
        {
            lua_settop(state, original_top);
            error = "DelegateProperty FunctionName must be a string or FName";
            return false;
        }
        lua_settop(state, original_top);
        return true;
    }

    int lua_multicast_delegate_gc(lua_State* state) noexcept
    {
        if (auto* delegate = get_multicast_delegate(state, 1); delegate != nullptr)
        {
            delegate->~LuaMulticastDelegate();
        }
        return 0;
    }

    int lua_multicast_delegate_type(lua_State* state) noexcept
    {
        const auto* delegate = get_multicast_delegate(state, 1);
        if (delegate == nullptr)
        {
            return luaL_error(state, "MulticastDelegateProperty:type requires a live wrapper");
        }
        lua_pushstring(
                state,
                delegate->sparse ? "MulticastSparseDelegateProperty"
                                 : "MulticastDelegateProperty");
        return 1;
    }

    int lua_multicast_delegate_get_bindings(lua_State* state) noexcept
    {
        auto* delegate = get_multicast_delegate(state, 1);
        auto* runtime = get_runtime(state);
        ue4ss::linux::core::UnrealPropertyValue value;
        std::string error;
        if (delegate == nullptr || runtime == nullptr ||
            !runtime->read_property(delegate->owner, delegate->property_name, value, error) ||
            value.kind != ue4ss::linux::core::UnrealPropertyKind::multicast_delegate)
        {
            return luaL_error(state,
                              "MulticastDelegateProperty:GetBindings failed: %s",
                              error.empty() ? "the live property is unavailable" : error.c_str());
        }
        return push_property_value(state, value);
    }

    int lua_multicast_delegate_len(lua_State* state) noexcept
    {
        auto* delegate = get_multicast_delegate(state, 1);
        auto* runtime = get_runtime(state);
        ue4ss::linux::core::UnrealPropertyValue value;
        std::string error;
        if (delegate == nullptr || runtime == nullptr ||
            !runtime->read_property(delegate->owner, delegate->property_name, value, error) ||
            value.kind != ue4ss::linux::core::UnrealPropertyKind::multicast_delegate)
        {
            return luaL_error(state,
                              "MulticastDelegateProperty length failed: %s",
                              error.empty() ? "the live property is unavailable" : error.c_str());
        }
        lua_pushinteger(state, static_cast<lua_Integer>(value.array_elements.size()));
        return 1;
    }

    int lua_multicast_delegate_add_or_remove(lua_State* state, bool remove) noexcept
    {
        auto* delegate = get_multicast_delegate(state, 1);
        auto* target = get_object(state, 2);
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        ue4ss::linux::core::RawFName function_name{};
        std::string error;
        if (delegate == nullptr || target == nullptr || runtime == nullptr || scheduler == nullptr)
        {
            error = "MulticastDelegateProperty:Add/Remove requires a live wrapper, target UObject, and game-thread scheduler";
        }
        else if (!lua_to_delegate_function_name(state, 3, *runtime, function_name, error))
        {
            // Preserve the conversion diagnostic.
        }
        else
        {
            const bool changed = remove
                                         ? runtime->remove_multicast_delegate_binding(
                                                   delegate->owner,
                                                   delegate->property_name,
                                                   target->handle,
                                                   function_name,
                                                   *scheduler,
                                                   error)
                                         : runtime->add_multicast_delegate_binding(
                                                   delegate->owner,
                                                   delegate->property_name,
                                                   target->handle,
                                                   function_name,
                                                   *scheduler,
                                                   error);
            if (changed)
            {
                return 0;
            }
        }
        return luaL_error(state,
                          "MulticastDelegateProperty:%s failed: %s",
                          remove ? "Remove" : "Add",
                          error.empty() ? "mutation failed without a diagnostic" : error.c_str());
    }

    int lua_multicast_delegate_add(lua_State* state) noexcept
    {
        return lua_multicast_delegate_add_or_remove(state, false);
    }

    int lua_multicast_delegate_remove(lua_State* state) noexcept
    {
        return lua_multicast_delegate_add_or_remove(state, true);
    }

    int lua_multicast_delegate_clear(lua_State* state) noexcept
    {
        auto* delegate = get_multicast_delegate(state, 1);
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        std::string error;
        if (delegate == nullptr || runtime == nullptr || scheduler == nullptr ||
            !runtime->clear_multicast_delegate_bindings(
                    delegate->owner, delegate->property_name, *scheduler, error))
        {
            return luaL_error(state,
                              "MulticastDelegateProperty:Clear failed: %s",
                              error.empty() ? "the live property or game-thread scheduler is unavailable"
                                            : error.c_str());
        }
        return 0;
    }

    int lua_multicast_delegate_broadcast(lua_State* state) noexcept
    {
        auto* delegate = get_multicast_delegate(state, 1);
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        std::vector<ue4ss::linux::core::UnrealFunctionArgument> arguments;
        std::string error;
        if (delegate == nullptr || runtime == nullptr || scheduler == nullptr)
        {
            error = "MulticastDelegateProperty:Broadcast requires a live wrapper and game-thread scheduler";
        }
        else
        {
            arguments.reserve(static_cast<std::size_t>(std::max(0, lua_gettop(state) - 1)));
            for (int index = 2; index <= lua_gettop(state) && error.empty(); ++index)
            {
                ue4ss::linux::core::UnrealFunctionArgument argument;
                argument.out_placeholder = lua_type(state, index) == LUA_TTABLE;
                if (lua_to_untyped_property_value(state, index, argument.value, error))
                {
                    arguments.push_back(std::move(argument));
                }
            }
            if (error.empty() && runtime->broadcast_multicast_delegate(
                                         delegate->owner,
                                         delegate->property_name,
                                         arguments,
                                         *scheduler,
                                         error))
            {
                return 0;
            }
        }
        return luaL_error(state,
                          "MulticastDelegateProperty:Broadcast failed: %s",
                          error.empty() ? "broadcast failed without a diagnostic" : error.c_str());
    }

    int push_local_value_param(
            lua_State* state,
            std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue> value) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaLocalValueParam), 0);
            new (storage) LuaLocalValueParam{std::move(value)};
            luaL_getmetatable(state, k_local_value_param_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "LocalUnrealParam construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "LocalUnrealParam construction failed with an unknown exception");
        }
    }

    int lua_tarray_gc(lua_State* state) noexcept
    {
        if (auto* array = get_tarray(state, 1); array != nullptr)
        {
            array->~LuaTArray();
        }
        return 0;
    }

    int lua_local_value_param_gc(lua_State* state) noexcept
    {
        if (auto* parameter = get_local_value_param(state, 1); parameter != nullptr)
        {
            parameter->~LuaLocalValueParam();
        }
        return 0;
    }

    int lua_local_value_param_get(lua_State* state) noexcept
    {
        auto* parameter = get_local_value_param(state, 1);
        if (parameter == nullptr || parameter->value == nullptr)
        {
            return luaL_error(state, "LocalUnrealParam:get has no value");
        }
        return push_property_value(state, *parameter->value);
    }

    int lua_local_value_param_set(lua_State* state) noexcept
    {
        auto* parameter = get_local_value_param(state, 1);
        if (parameter == nullptr || parameter->value == nullptr)
        {
            return luaL_error(state, "LocalUnrealParam:set has no value");
        }
        ue4ss::linux::core::UnrealPropertyValue replacement;
        std::string error;
        if (!lua_to_untyped_property_value(state, 2, replacement, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        *parameter->value = std::move(replacement);
        return 0;
    }

    int lua_tarray_index(lua_State* state) noexcept
    {
        auto* array = get_tarray(state, 1);
        if (array == nullptr)
        {
            lua_pushnil(state);
            return 1;
        }
        if (lua_isinteger(state, 2))
        {
            const lua_Integer requested = lua_tointeger(state, 2);
            if (requested < 1 || static_cast<std::uint64_t>(requested) > array->elements.size() ||
                array->elements[static_cast<std::size_t>(requested - 1)] == nullptr)
            {
                return luaL_error(state, "TArray index out of range");
            }
            return push_property_value(state, *array->elements[static_cast<std::size_t>(requested - 1)]);
        }
        if (lua_type(state, 2) == LUA_TSTRING)
        {
            luaL_getmetatable(state, k_tarray_metatable);
            lua_getfield(state, -1, "__methods");
            lua_pushvalue(state, 2);
            lua_rawget(state, -2);
            return 1;
        }
        lua_pushnil(state);
        return 1;
    }

    int lua_tarray_newindex(lua_State* state) noexcept
    {
        auto* array = get_tarray(state, 1);
        if (array == nullptr || !lua_isinteger(state, 2))
        {
            return luaL_error(state, "TArray assignment requires an integer index");
        }
        const lua_Integer requested = lua_tointeger(state, 2);
        if (requested < 1 || static_cast<std::uint64_t>(requested) > array->elements.size() + 1u)
        {
            return luaL_error(state, "TArray assignment index is not contiguous");
        }
        auto replacement = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
        std::string error;
        if (!lua_to_untyped_property_value(state, 3, *replacement, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        if (static_cast<std::size_t>(requested) == array->elements.size() + 1u)
        {
            array->elements.push_back(std::move(replacement));
        }
        else
        {
            array->elements[static_cast<std::size_t>(requested - 1)] = std::move(replacement);
        }
        return 0;
    }

    int lua_tarray_len(lua_State* state) noexcept
    {
        auto* array = get_tarray(state, 1);
        lua_pushinteger(state, array == nullptr ? 0 : static_cast<lua_Integer>(array->elements.size()));
        return 1;
    }

    int lua_tarray_get_num(lua_State* state) noexcept
    {
        return lua_tarray_len(state);
    }

    int lua_tarray_get_max(lua_State* state) noexcept
    {
        return lua_tarray_len(state);
    }

    int lua_tarray_get_unavailable_address(lua_State* state) noexcept
    {
        lua_pushinteger(state, 0);
        return 1;
    }

    int lua_tarray_empty(lua_State* state) noexcept
    {
        auto* array = get_tarray(state, 1);
        if (array == nullptr)
        {
            return luaL_error(state, "TArray:Empty requires TArray userdata");
        }
        array->elements.clear();
        return 0;
    }

    int lua_tarray_is_valid(lua_State* state) noexcept
    {
        lua_pushboolean(state, get_tarray(state, 1) != nullptr);
        return 1;
    }

    int lua_tarray_type(lua_State* state) noexcept
    {
        lua_pushliteral(state, "TArray");
        return 1;
    }

    int lua_tarray_for_each(lua_State* state) noexcept
    {
        auto* array = get_tarray(state, 1);
        if (array == nullptr || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "TArray:ForEach requires a callback");
        }
        for (std::size_t index = 0; index < array->elements.size(); ++index)
        {
            if (array->elements[index] == nullptr)
            {
                continue;
            }
            lua_pushvalue(state, 2);
            lua_pushinteger(state, static_cast<lua_Integer>(index + 1u));
            push_local_value_param(state, array->elements[index]);
            if (lua_pcall(state, 2, 1, 0) != LUA_OK)
            {
                return lua_error(state);
            }
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop)
            {
                break;
            }
        }
        return 0;
    }

    [[nodiscard]] bool property_values_equal(
            const ue4ss::linux::core::UnrealPropertyValue& left,
            const ue4ss::linux::core::UnrealPropertyValue& right,
            std::size_t depth = 0u) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        if (depth >= 32u)
        {
            return false;
        }
        const auto is_numeric = [](UnrealPropertyKind kind) noexcept {
            return kind == UnrealPropertyKind::signed_integer ||
                   kind == UnrealPropertyKind::unsigned_integer ||
                   kind == UnrealPropertyKind::floating_point;
        };
        if (left.kind != right.kind)
        {
            if (!is_numeric(left.kind) || !is_numeric(right.kind)) return false;
            if (left.kind == UnrealPropertyKind::signed_integer &&
                right.kind == UnrealPropertyKind::unsigned_integer)
            {
                return left.signed_integer >= 0 &&
                       static_cast<std::uint64_t>(left.signed_integer) == right.unsigned_integer;
            }
            if (left.kind == UnrealPropertyKind::unsigned_integer &&
                right.kind == UnrealPropertyKind::signed_integer)
            {
                return right.signed_integer >= 0 &&
                       left.unsigned_integer == static_cast<std::uint64_t>(right.signed_integer);
            }
            const long double left_number = left.kind == UnrealPropertyKind::floating_point
                                                    ? static_cast<long double>(left.floating_point)
                                            : left.kind == UnrealPropertyKind::signed_integer
                                                    ? static_cast<long double>(left.signed_integer)
                                                    : static_cast<long double>(left.unsigned_integer);
            const long double right_number = right.kind == UnrealPropertyKind::floating_point
                                                     ? static_cast<long double>(right.floating_point)
                                             : right.kind == UnrealPropertyKind::signed_integer
                                                     ? static_cast<long double>(right.signed_integer)
                                                     : static_cast<long double>(right.unsigned_integer);
            return left_number == right_number;
        }
        switch (left.kind)
        {
        case UnrealPropertyKind::boolean:
            return left.boolean == right.boolean;
        case UnrealPropertyKind::signed_integer:
            return left.signed_integer == right.signed_integer;
        case UnrealPropertyKind::unsigned_integer:
            return left.unsigned_integer == right.unsigned_integer;
        case UnrealPropertyKind::floating_point:
            return left.floating_point == right.floating_point;
        case UnrealPropertyKind::name:
            return left.name.comparison_index == right.name.comparison_index &&
                   left.name.number == right.name.number;
        case UnrealPropertyKind::string:
        case UnrealPropertyKind::text:
            return left.text == right.text;
        case UnrealPropertyKind::object:
            return left.object_is_null == right.object_is_null &&
                   (left.object_is_null ||
                    (left.object.address == right.object.address &&
                     left.object.internal_index == right.object.internal_index &&
                     left.object.serial_number == right.object.serial_number));
        case UnrealPropertyKind::structure:
            if (left.struct_fields.size() != right.struct_fields.size()) return false;
            for (const auto& a : left.struct_fields)
            {
                const auto found = std::find_if(
                        right.struct_fields.begin(), right.struct_fields.end(), [&a](const auto& field) {
                            return field.name == a.name;
                        });
                if (a.value == nullptr || found == right.struct_fields.end() || found->value == nullptr ||
                    !property_values_equal(*a.value, *found->value, depth + 1u))
                {
                    return false;
                }
            }
            return true;
        case UnrealPropertyKind::array:
            if (left.array_elements.size() != right.array_elements.size()) return false;
            for (std::size_t index = 0; index < left.array_elements.size(); ++index)
            {
                if (left.array_elements[index] == nullptr || right.array_elements[index] == nullptr ||
                    !property_values_equal(
                            *left.array_elements[index], *right.array_elements[index], depth + 1u))
                {
                    return false;
                }
            }
            return true;
        case UnrealPropertyKind::set:
            if (left.set_elements.size() != right.set_elements.size()) return false;
            for (std::size_t index = 0; index < left.set_elements.size(); ++index)
            {
                if (left.set_elements[index] == nullptr || right.set_elements[index] == nullptr ||
                    !property_values_equal(*left.set_elements[index], *right.set_elements[index], depth + 1u))
                {
                    return false;
                }
            }
            return true;
        case UnrealPropertyKind::map:
            if (left.map_entries.size() != right.map_entries.size()) return false;
            for (std::size_t index = 0; index < left.map_entries.size(); ++index)
            {
                const auto& a = left.map_entries[index];
                const auto& b = right.map_entries[index];
                if (a.key == nullptr || b.key == nullptr || a.value == nullptr || b.value == nullptr ||
                    !property_values_equal(*a.key, *b.key, depth + 1u) ||
                    !property_values_equal(*a.value, *b.value, depth + 1u))
                {
                    return false;
                }
            }
            return true;
        case UnrealPropertyKind::weak_object:
            return left.object_is_null == right.object_is_null &&
                   (left.object_is_null ||
                    (left.object.address == right.object.address &&
                     left.object.internal_index == right.object.internal_index &&
                     left.object.serial_number == right.object.serial_number));
        case UnrealPropertyKind::soft_object:
            return left.soft_package_name.comparison_index == right.soft_package_name.comparison_index &&
                   left.soft_package_name.number == right.soft_package_name.number &&
                   left.soft_asset_name.comparison_index == right.soft_asset_name.comparison_index &&
                   left.soft_asset_name.number == right.soft_asset_name.number &&
                   left.soft_sub_path == right.soft_sub_path &&
                   left.object_is_null == right.object_is_null &&
                   (left.object_is_null ||
                    (left.object.address == right.object.address &&
                     left.object.internal_index == right.object.internal_index &&
                     left.object.serial_number == right.object.serial_number));
        case UnrealPropertyKind::delegate:
            return left.name.comparison_index == right.name.comparison_index &&
                   left.name.number == right.name.number && left.object_is_null == right.object_is_null &&
                   (left.object_is_null || left.object.address == right.object.address);
        case UnrealPropertyKind::multicast_delegate:
            if (left.multicast_sparse != right.multicast_sparse) return false;
            if (left.array_elements.size() != right.array_elements.size()) return false;
            for (std::size_t index = 0; index < left.array_elements.size(); ++index)
            {
                if (left.array_elements[index] == nullptr || right.array_elements[index] == nullptr ||
                    !property_values_equal(
                            *left.array_elements[index], *right.array_elements[index], depth + 1u))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    int lua_tset_gc(lua_State* state) noexcept
    {
        if (auto* set = get_tset(state, 1); set != nullptr) set->~LuaTSet();
        return 0;
    }

    int lua_tmap_gc(lua_State* state) noexcept
    {
        if (auto* map = get_tmap(state, 1); map != nullptr) map->~LuaTMap();
        return 0;
    }

    int lua_tset_len(lua_State* state) noexcept
    {
        const auto* set = get_tset(state, 1);
        lua_pushinteger(state, set == nullptr ? 0 : static_cast<lua_Integer>(set->elements.size()));
        return 1;
    }

    int lua_tmap_len(lua_State* state) noexcept
    {
        const auto* map = get_tmap(state, 1);
        lua_pushinteger(state, map == nullptr ? 0 : static_cast<lua_Integer>(map->entries.size()));
        return 1;
    }

    int lua_tset_is_valid(lua_State* state) noexcept
    {
        lua_pushboolean(state, get_tset(state, 1) != nullptr);
        return 1;
    }

    int lua_tmap_is_valid(lua_State* state) noexcept
    {
        lua_pushboolean(state, get_tmap(state, 1) != nullptr);
        return 1;
    }

    int lua_tset_type(lua_State* state) noexcept
    {
        lua_pushliteral(state, "TSet");
        return 1;
    }

    int lua_tmap_type(lua_State* state) noexcept
    {
        lua_pushliteral(state, "TMap");
        return 1;
    }

    int lua_tset_add(lua_State* state) noexcept
    {
        try
        {
            auto* set = get_tset(state, 1);
            if (set == nullptr) return luaL_error(state, "TSet:Add requires TSet userdata");
            auto replacement = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
            std::string error;
            if (!lua_to_untyped_property_value(state, 2, *replacement, error))
            {
                return luaL_error(state, "%s", error.c_str());
            }
            const auto found = std::find_if(set->elements.begin(), set->elements.end(), [&replacement](const auto& element) {
                return element != nullptr && property_values_equal(*element, *replacement);
            });
            if (found == set->elements.end())
            {
                set->elements.push_back(std::move(replacement));
            }
            else
            {
                *found = std::move(replacement);
            }
            lua_pushvalue(state, 2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "TSet:Add failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "TSet:Add failed with an unknown exception");
        }
    }

    int lua_tset_remove(lua_State* state) noexcept
    {
        auto* set = get_tset(state, 1);
        if (set == nullptr) return luaL_error(state, "TSet:Remove requires TSet userdata");
        ue4ss::linux::core::UnrealPropertyValue requested;
        std::string error;
        if (!lua_to_untyped_property_value(state, 2, requested, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const auto found = std::find_if(set->elements.begin(), set->elements.end(), [&requested](const auto& element) {
            return element != nullptr && property_values_equal(*element, requested);
        });
        if (found != set->elements.end()) set->elements.erase(found);
        lua_pushvalue(state, 2);
        return 1;
    }

    int lua_tset_empty(lua_State* state) noexcept
    {
        auto* set = get_tset(state, 1);
        if (set == nullptr) return luaL_error(state, "TSet:Empty requires TSet userdata");
        set->elements.clear();
        lua_pushvalue(state, 1);
        return 1;
    }

    int lua_tmap_add(lua_State* state) noexcept
    {
        try
        {
            auto* map = get_tmap(state, 1);
            if (map == nullptr) return luaL_error(state, "TMap:Add requires TMap userdata");
            auto key = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
            auto value = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
            std::string error;
            if (!lua_to_untyped_property_value(state, 2, *key, error) ||
                !lua_to_untyped_property_value(state, 3, *value, error))
            {
                return luaL_error(state, "%s", error.c_str());
            }
            const auto found = std::find_if(map->entries.begin(), map->entries.end(), [&key](const auto& entry) {
                return entry.key != nullptr && property_values_equal(*entry.key, *key);
            });
            if (found == map->entries.end())
            {
                map->entries.push_back({std::move(key), std::move(value)});
            }
            else
            {
                found->key = std::move(key);
                found->value = std::move(value);
            }
            lua_pushvalue(state, 3);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "TMap:Add failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "TMap:Add failed with an unknown exception");
        }
    }

    int lua_tmap_remove(lua_State* state) noexcept
    {
        auto* map = get_tmap(state, 1);
        if (map == nullptr) return luaL_error(state, "TMap:Remove requires TMap userdata");
        ue4ss::linux::core::UnrealPropertyValue requested;
        std::string error;
        if (!lua_to_untyped_property_value(state, 2, requested, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const auto found = std::find_if(map->entries.begin(), map->entries.end(), [&requested](const auto& entry) {
            return entry.key != nullptr && property_values_equal(*entry.key, requested);
        });
        if (found != map->entries.end()) map->entries.erase(found);
        lua_pushvalue(state, 2);
        return 1;
    }

    int lua_tmap_empty(lua_State* state) noexcept
    {
        auto* map = get_tmap(state, 1);
        if (map == nullptr) return luaL_error(state, "TMap:Empty requires TMap userdata");
        map->entries.clear();
        lua_pushvalue(state, 1);
        return 1;
    }

    int lua_tset_contains(lua_State* state) noexcept
    {
        const auto* set = get_tset(state, 1);
        if (set == nullptr) return luaL_error(state, "TSet:Contains requires TSet userdata");
        ue4ss::linux::core::UnrealPropertyValue requested;
        std::string error;
        if (!lua_to_untyped_property_value(state, 2, requested, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const bool found = std::any_of(set->elements.begin(), set->elements.end(), [&requested](const auto& element) {
            return element != nullptr && property_values_equal(*element, requested);
        });
        lua_pushboolean(state, found);
        return 1;
    }

    int lua_tmap_contains(lua_State* state) noexcept
    {
        const auto* map = get_tmap(state, 1);
        if (map == nullptr) return luaL_error(state, "TMap:Contains requires TMap userdata");
        ue4ss::linux::core::UnrealPropertyValue requested;
        std::string error;
        if (!lua_to_untyped_property_value(state, 2, requested, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const bool found = std::any_of(map->entries.begin(), map->entries.end(), [&requested](const auto& entry) {
            return entry.key != nullptr && property_values_equal(*entry.key, requested);
        });
        lua_pushboolean(state, found);
        return 1;
    }

    int lua_tmap_find(lua_State* state) noexcept
    {
        const auto* map = get_tmap(state, 1);
        if (map == nullptr) return luaL_error(state, "TMap:Find requires TMap userdata");
        ue4ss::linux::core::UnrealPropertyValue requested;
        std::string error;
        if (!lua_to_untyped_property_value(state, 2, requested, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const auto found = std::find_if(map->entries.begin(), map->entries.end(), [&requested](const auto& entry) {
            return entry.key != nullptr && property_values_equal(*entry.key, requested);
        });
        if (found == map->entries.end() || found->value == nullptr)
        {
            return luaL_error(state, "Map key not found.");
        }
        return push_property_value(state, *found->value);
    }

    int lua_tset_for_each(lua_State* state) noexcept
    {
        const auto* set = get_tset(state, 1);
        if (set == nullptr || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "TSet:ForEach requires a callback");
        }
        for (const auto& element : set->elements)
        {
            if (element == nullptr) continue;
            lua_pushvalue(state, 2);
            push_property_value(state, *element);
            if (lua_pcall(state, 1, 1, 0) != LUA_OK) return lua_error(state);
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop) break;
        }
        return 0;
    }

    int lua_tmap_for_each(lua_State* state) noexcept
    {
        const auto* map = get_tmap(state, 1);
        if (map == nullptr || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "TMap:ForEach requires a callback");
        }
        for (const auto& entry : map->entries)
        {
            if (entry.key == nullptr || entry.value == nullptr) continue;
            lua_pushvalue(state, 2);
            push_property_value(state, *entry.key);
            push_property_value(state, *entry.value);
            if (lua_pcall(state, 2, 1, 0) != LUA_OK) return lua_error(state);
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop) break;
        }
        return 0;
    }

    int push_fname(lua_State* state,
                   ue4ss::linux::core::RawFName value,
                   std::string text) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaFName), 0);
        new (storage) LuaFName{value, std::move(text)};
        luaL_getmetatable(state, k_fname_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int push_fstring(lua_State* state, std::string value) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaFString), 0);
        new (storage) LuaFString{std::move(value)};
        luaL_getmetatable(state, k_fstring_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int push_furl(lua_State* state,
                  const ue4ss::linux::core::UnrealFURLSnapshot& value) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaFURL), 0);
            new (storage) LuaFURL{value};
            luaL_getmetatable(state, k_furl_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "FURL snapshot construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "FURL snapshot construction failed with an unknown exception");
        }
    }

    int push_output_device(lua_State* state,
                           ue4ss::linux::core::HeadlessLuaState& owner,
                           std::uint64_t epoch,
                           std::uint64_t address) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaFOutputDevice), 0);
        new (storage) LuaFOutputDevice{&owner, epoch, address};
        luaL_getmetatable(state, k_output_device_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int push_narrow_string(lua_State* state,
                           std::string value,
                           NarrowStringKind kind) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaNarrowString), 0);
        new (storage) LuaNarrowString{std::move(value), kind};
        luaL_getmetatable(
                state,
                kind == NarrowStringKind::utf8 ? k_futf8string_metatable
                                                : k_fansistring_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int push_ftext(lua_State* state, std::string value) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaFText), 0);
        new (storage) LuaFText{std::move(value)};
        luaL_getmetatable(state, k_ftext_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int lua_fname_gc(lua_State* state) noexcept
    {
        if (auto* value = get_fname(state, 1); value != nullptr)
        {
            value->~LuaFName();
        }
        return 0;
    }

    int lua_fname_constructor(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* runtime = get_runtime(state);
            ue4ss::linux::core::RawFName value{};
            std::string text;
            std::string error;
            if (runtime == nullptr)
            {
                error = "FName constructor requires the validated Unreal runtime";
            }
            else if (lua_type(state, 1) == LUA_TSTRING)
            {
                std::size_t length{};
                const char* input = lua_tolstring(state, 1, &length);
                const lua_Integer find_type = luaL_optinteger(state, 2, 1);
                if (find_type != 0 && find_type != 1)
                {
                    error = "FName EFindName must be FNAME_Find (0) or FNAME_Add (1)";
                }
                else if (runtime->fname_from_utf8(
                                 std::string_view{input, length}, value, find_type == 1, error) &&
                         !runtime->fname_to_utf8(value, text, error))
                {
                    // Preserve the constructor diagnostic below.
                }
            }
            else if (lua_isinteger(state, 1))
            {
                const lua_Integer comparison_index = lua_tointeger(state, 1);
                const lua_Integer number = luaL_optinteger(state, 2, 0);
                if (comparison_index < 0 || comparison_index > std::numeric_limits<std::uint32_t>::max() ||
                    number < 0 || number > std::numeric_limits<std::uint32_t>::max())
                {
                    error = "FName comparison index and number must fit uint32";
                }
                else
                {
                    value = {static_cast<std::uint32_t>(comparison_index), static_cast<std::uint32_t>(number)};
                    static_cast<void>(runtime->fname_to_utf8(value, text, error));
                }
            }
            else
            {
                error = "FName requires a string or comparison-index integer";
            }
            if (error.empty())
            {
                return push_fname(state, value, std::move(text));
            }
            lua_pushlstring(state, error.data(), error.size());
            failed = true;
        }
        return failed ? lua_error(state) : 0;
    }

    int lua_fname_to_string(lua_State* state) noexcept
    {
        auto* value = get_fname(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FName:ToString requires FName userdata");
        }
        lua_pushlstring(state, value->text.data(), value->text.size());
        return 1;
    }

    int lua_fname_get_comparison_index(lua_State* state) noexcept
    {
        auto* value = get_fname(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FName:GetComparisonIndex requires FName userdata");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(value->value.comparison_index));
        return 1;
    }

    int lua_fname_equals(lua_State* state) noexcept
    {
        const auto* left = get_fname(state, 1);
        const auto* right = get_fname(state, 2);
        lua_pushboolean(state,
                        left != nullptr && right != nullptr &&
                                left->value.comparison_index == right->value.comparison_index &&
                                left->value.number == right->value.number);
        return 1;
    }

    int lua_fname_type(lua_State* state) noexcept
    {
        if (get_fname(state, 1) == nullptr)
        {
            return luaL_error(state, "FName:type requires FName userdata");
        }
        lua_pushliteral(state, "FName");
        return 1;
    }

    int lua_fstring_gc(lua_State* state) noexcept
    {
        if (auto* value = get_fstring(state, 1); value != nullptr)
        {
            value->~LuaFString();
        }
        return 0;
    }

    int lua_furl_gc(lua_State* state) noexcept
    {
        if (auto* value = get_furl(state, 1); value != nullptr)
        {
            value->~LuaFURL();
        }
        return 0;
    }

    int lua_furl_type(lua_State* state) noexcept
    {
        if (get_furl(state, 1) == nullptr)
        {
            return luaL_error(state, "FURL:type requires FURL userdata");
        }
        lua_pushliteral(state, "FURL");
        return 1;
    }

    int lua_output_device_gc(lua_State* state) noexcept
    {
        if (auto* value = get_output_device(state, 1); value != nullptr)
        {
            value->~LuaFOutputDevice();
        }
        return 0;
    }

    int lua_output_device_type(lua_State* state) noexcept
    {
        if (get_output_device(state, 1) == nullptr)
        {
            return luaL_error(state, "FOutputDevice:type requires FOutputDevice userdata");
        }
        lua_pushliteral(state, "FOutputDevice");
        return 1;
    }

    int lua_output_device_log(lua_State* state) noexcept
    {
        auto* output = get_output_device(state, 1);
        if (output == nullptr || output->owner == nullptr || lua_type(state, 2) != LUA_TSTRING)
        {
            return luaL_error(state, "FOutputDevice:Log requires a UTF-8 string");
        }
        std::size_t length{};
        const char* message = lua_tolstring(state, 2, &length);
        std::string error;
        if (!output->owner->log_output_device(
                    output->epoch,
                    output->address,
                    std::string_view{message, length},
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_fstring_constructor(lua_State* state) noexcept
    {
        if (lua_type(state, 1) != LUA_TSTRING)
        {
            return luaL_error(state, "FString constructor requires a UTF-8 Lua string");
        }
        std::size_t length{};
        const char* value = lua_tolstring(state, 1, &length);
        try
        {
            // Validate now so malformed UTF-8 cannot reach the engine allocator.
            static_cast<void>(ue4ss::linux::utf8_to_unreal(std::string_view{value, length}));
            return push_fstring(state, std::string{value, length});
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "%s", exception.what());
        }
    }

    int lua_ftext_gc(lua_State* state) noexcept
    {
        if (auto* value = get_ftext(state, 1); value != nullptr)
        {
            value->~LuaFText();
        }
        return 0;
    }

    int lua_ftext_constructor(lua_State* state) noexcept
    {
        if (lua_type(state, 1) != LUA_TSTRING)
        {
            return luaL_error(state, "FText constructor requires a UTF-8 Lua string");
        }
        std::size_t length{};
        const char* value = lua_tolstring(state, 1, &length);
        try
        {
            static_cast<void>(ue4ss::linux::utf8_to_unreal(std::string_view{value, length}));
            return push_ftext(state, std::string{value, length});
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "%s", exception.what());
        }
    }

    int lua_ftext_to_string(lua_State* state) noexcept
    {
        const auto* value = get_ftext(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FText:ToString requires FText userdata");
        }
        lua_pushlstring(state, value->value.data(), value->value.size());
        return 1;
    }

    int lua_ftext_type(lua_State* state) noexcept
    {
        if (get_ftext(state, 1) == nullptr)
        {
            return luaL_error(state, "FText:type requires FText userdata");
        }
        lua_pushliteral(state, "FText");
        return 1;
    }

    int lua_ftext_equal(lua_State* state) noexcept
    {
        const auto* left = get_ftext(state, 1);
        const auto* right = get_ftext(state, 2);
        lua_pushboolean(state, left != nullptr && right != nullptr && left->value == right->value);
        return 1;
    }

    int lua_fstring_to_string(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FString:ToString requires FString userdata");
        }
        lua_pushlstring(state, value->value.data(), value->value.size());
        return 1;
    }

    int lua_fstring_empty(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FString:Empty requires FString userdata");
        }
        value->value.clear();
        return 0;
    }

    int lua_fstring_len(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FString:Len requires FString userdata");
        }
        try
        {
            lua_pushinteger(state, static_cast<lua_Integer>(ue4ss::linux::utf8_to_unreal(value->value).size()));
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "%s", exception.what());
        }
    }

    int lua_fstring_is_empty(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        lua_pushboolean(state, value != nullptr && value->value.empty());
        return 1;
    }

    int lua_fstring_append(lua_State* state) noexcept
    {
        auto* destination = get_fstring(state, 1);
        if (destination == nullptr)
        {
            return luaL_error(state, "FString:Append requires FString userdata");
        }
        if (auto* source = get_fstring(state, 2); source != nullptr)
        {
            destination->value.append(source->value);
            return 0;
        }
        if (lua_type(state, 2) == LUA_TSTRING)
        {
            std::size_t length{};
            const char* source = lua_tolstring(state, 2, &length);
            try
            {
                static_cast<void>(ue4ss::linux::utf8_to_unreal(std::string_view{source, length}));
                destination->value.append(source, length);
                return 0;
            }
            catch (const std::exception& exception)
            {
                return luaL_error(state, "%s", exception.what());
            }
        }
        return luaL_error(state, "FString:Append requires a string or FString");
    }

    int lua_fstring_find(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        std::size_t length{};
        const char* needle = lua_tolstring(state, 2, &length);
        if (value == nullptr || needle == nullptr)
        {
            return luaL_error(state, "FString:Find requires a UTF-8 string");
        }
        const std::size_t position = value->value.find(std::string_view{needle, length});
        if (position == std::string::npos)
        {
            lua_pushnil(state);
        }
        else
        {
            lua_pushinteger(state, static_cast<lua_Integer>(position + 1u));
        }
        return 1;
    }

    int lua_fstring_starts_with(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        std::size_t length{};
        const char* prefix = lua_tolstring(state, 2, &length);
        lua_pushboolean(state,
                        value != nullptr && prefix != nullptr &&
                                value->value.starts_with(std::string_view{prefix, length}));
        return 1;
    }

    int lua_fstring_ends_with(lua_State* state) noexcept
    {
        auto* value = get_fstring(state, 1);
        std::size_t length{};
        const char* suffix = lua_tolstring(state, 2, &length);
        lua_pushboolean(state,
                        value != nullptr && suffix != nullptr &&
                                value->value.ends_with(std::string_view{suffix, length}));
        return 1;
    }

    int lua_fstring_case(lua_State* state, bool upper) noexcept
    {
        auto* value = get_fstring(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "FString case conversion requires FString userdata");
        }
        std::string converted = value->value;
        for (char& byte : converted)
        {
            const auto current = static_cast<unsigned char>(byte);
            if (current < 0x80u)
            {
                byte = static_cast<char>(upper ? std::toupper(current) : std::tolower(current));
            }
        }
        return push_fstring(state, std::move(converted));
    }

    int lua_fstring_to_upper(lua_State* state) noexcept
    {
        return lua_fstring_case(state, true);
    }

    int lua_fstring_to_lower(lua_State* state) noexcept
    {
        return lua_fstring_case(state, false);
    }

    [[nodiscard]] const char* narrow_string_type_name(NarrowStringKind kind) noexcept
    {
        return kind == NarrowStringKind::utf8 ? "FUtf8String" : "FAnsiString";
    }

    int lua_narrow_string_gc(lua_State* state) noexcept
    {
        if (auto* value = get_narrow_string(state, 1); value != nullptr)
        {
            value->~LuaNarrowString();
        }
        return 0;
    }

    int lua_narrow_string_constructor(lua_State* state, NarrowStringKind kind) noexcept
    {
        if (lua_type(state, 1) != LUA_TSTRING)
        {
            return luaL_error(state,
                              "%s constructor requires a Lua string",
                              narrow_string_type_name(kind));
        }
        std::size_t length{};
        const char* input = lua_tolstring(state, 1, &length);
        if (kind == NarrowStringKind::utf8)
        {
            try
            {
                static_cast<void>(
                        ue4ss::linux::utf8_to_unreal(std::string_view{input, length}));
            }
            catch (const std::exception& exception)
            {
                return luaL_error(state, "%s", exception.what());
            }
        }
        return push_narrow_string(state, std::string{input, length}, kind);
    }

    int lua_futf8string_constructor(lua_State* state) noexcept
    {
        return lua_narrow_string_constructor(state, NarrowStringKind::utf8);
    }

    int lua_fansistring_constructor(lua_State* state) noexcept
    {
        return lua_narrow_string_constructor(state, NarrowStringKind::ansi);
    }

    int lua_narrow_string_to_string(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "narrow Unreal string ToString requires userdata");
        }
        lua_pushlstring(state, value->value.data(), value->value.size());
        return 1;
    }

    int lua_narrow_string_empty(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "narrow Unreal string Empty requires userdata");
        }
        value->value.clear();
        return 0;
    }

    int lua_narrow_string_len(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "narrow Unreal string Len requires userdata");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(value->value.size()));
        return 1;
    }

    int lua_narrow_string_is_empty(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        lua_pushboolean(state, value != nullptr && value->value.empty());
        return 1;
    }

    int lua_narrow_string_append(lua_State* state) noexcept
    {
        auto* destination = get_narrow_string(state, 1);
        if (destination == nullptr)
        {
            return luaL_error(state, "narrow Unreal string Append requires userdata");
        }
        if (auto* source = get_narrow_string(state, 2); source != nullptr)
        {
            if (source->kind != destination->kind)
            {
                return luaL_error(state,
                                  "%s:Append requires a Lua string or matching %s userdata",
                                  narrow_string_type_name(destination->kind),
                                  narrow_string_type_name(destination->kind));
            }
            destination->value.append(source->value);
            return 0;
        }
        if (lua_type(state, 2) != LUA_TSTRING)
        {
            return luaL_error(state,
                              "%s:Append requires a Lua string or matching userdata",
                              narrow_string_type_name(destination->kind));
        }
        std::size_t length{};
        const char* source = lua_tolstring(state, 2, &length);
        if (destination->kind == NarrowStringKind::utf8)
        {
            try
            {
                static_cast<void>(
                        ue4ss::linux::utf8_to_unreal(std::string_view{source, length}));
            }
            catch (const std::exception& exception)
            {
                return luaL_error(state, "%s", exception.what());
            }
        }
        destination->value.append(source, length);
        return 0;
    }

    int lua_narrow_string_find(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        std::size_t length{};
        const char* needle = lua_tolstring(state, 2, &length);
        if (value == nullptr || needle == nullptr)
        {
            return luaL_error(state, "narrow Unreal string Find requires a Lua string");
        }
        const std::size_t position = value->value.find(std::string_view{needle, length});
        if (position == std::string::npos)
        {
            lua_pushnil(state);
        }
        else
        {
            lua_pushinteger(state, static_cast<lua_Integer>(position + 1u));
        }
        return 1;
    }

    int lua_narrow_string_starts_with(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        std::size_t length{};
        const char* prefix = lua_tolstring(state, 2, &length);
        lua_pushboolean(state,
                        value != nullptr && prefix != nullptr &&
                                value->value.starts_with(std::string_view{prefix, length}));
        return 1;
    }

    int lua_narrow_string_ends_with(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        std::size_t length{};
        const char* suffix = lua_tolstring(state, 2, &length);
        lua_pushboolean(state,
                        value != nullptr && suffix != nullptr &&
                                value->value.ends_with(std::string_view{suffix, length}));
        return 1;
    }

    int lua_narrow_string_case(lua_State* state, bool upper) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "narrow Unreal string case conversion requires userdata");
        }
        std::string converted = value->value;
        for (char& byte : converted)
        {
            const auto current = static_cast<unsigned char>(byte);
            if (current < 0x80u)
            {
                byte = static_cast<char>(
                        upper ? std::toupper(current) : std::tolower(current));
            }
        }
        return push_narrow_string(state, std::move(converted), value->kind);
    }

    int lua_narrow_string_to_upper(lua_State* state) noexcept
    {
        return lua_narrow_string_case(state, true);
    }

    int lua_narrow_string_to_lower(lua_State* state) noexcept
    {
        return lua_narrow_string_case(state, false);
    }

    int lua_narrow_string_type(lua_State* state) noexcept
    {
        auto* value = get_narrow_string(state, 1);
        if (value == nullptr)
        {
            return luaL_error(state, "narrow Unreal string type requires userdata");
        }
        lua_pushstring(state, narrow_string_type_name(value->kind));
        return 1;
    }

    int push_fproperty(lua_State* state,
                       const ue4ss::linux::core::ReflectedPropertyDescription& description) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaFProperty), 0);
            new (storage) LuaFProperty{description};
            luaL_getmetatable(state, k_fproperty_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "FProperty construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "FProperty construction failed with an unknown exception");
        }
    }

    int push_ffield_class(lua_State* state, std::string name) noexcept
    {
        try
        {
            void* storage = lua_newuserdatauv(state, sizeof(LuaFFieldClass), 0);
            new (storage) LuaFFieldClass{std::move(name)};
            luaL_getmetatable(state, k_ffield_class_metatable);
            lua_setmetatable(state, -2);
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "FFieldClass construction failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "FFieldClass construction failed with an unknown exception");
        }
    }

    int lua_fproperty_gc(lua_State* state) noexcept
    {
        if (auto* property = get_fproperty(state, 1); property != nullptr) property->~LuaFProperty();
        return 0;
    }

    int lua_ffield_class_gc(lua_State* state) noexcept
    {
        if (auto* field_class = get_ffield_class(state, 1); field_class != nullptr)
        {
            field_class->~LuaFFieldClass();
        }
        return 0;
    }

    int push_object(lua_State* state,
                    const ue4ss::linux::core::ReadOnlyUObjectHandle& handle) noexcept;

    int push_struct_binding(
            lua_State* state,
            const ue4ss::linux::core::UnrealStructBinding& binding) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaUScriptStruct), 0);
        new (storage) LuaUScriptStruct{binding};
        luaL_getmetatable(state, k_uscript_struct_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int lua_uscript_struct_gc(lua_State* state) noexcept
    {
        if (auto* unreal_struct = get_uscript_struct(state, 1); unreal_struct != nullptr)
        {
            unreal_struct->~LuaUScriptStruct();
        }
        return 0;
    }

    int lua_uscript_struct_is_valid(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* unreal_struct = get_uscript_struct(state, 1);
        std::string error;
        lua_pushboolean(
                state,
                runtime != nullptr && unreal_struct != nullptr &&
                        runtime->validate_struct_binding(
                                unreal_struct->binding, error));
        return 1;
    }

    int lua_uscript_struct_type(lua_State* state) noexcept
    {
        if (get_uscript_struct(state, 1) == nullptr)
        {
            return luaL_error(state, "UScriptStruct:type requires UScriptStruct userdata");
        }
        lua_pushliteral(state, "UScriptStruct");
        return 1;
    }

    int lua_uscript_struct_get_struct_address(lua_State* state) noexcept
    {
        const auto* unreal_struct = get_uscript_struct(state, 1);
        if (unreal_struct == nullptr)
        {
            return luaL_error(
                    state,
                    "UScriptStruct:GetStructAddress requires UScriptStruct userdata");
        }
        lua_pushinteger(
                state,
                static_cast<lua_Integer>(unreal_struct->binding.storage_address));
        return 1;
    }

    int lua_uscript_struct_get_property_address(lua_State* state) noexcept
    {
        const auto* unreal_struct = get_uscript_struct(state, 1);
        if (unreal_struct == nullptr)
        {
            return luaL_error(
                    state,
                    "UScriptStruct:GetPropertyAddress requires UScriptStruct userdata");
        }
        lua_pushinteger(
                state,
                static_cast<lua_Integer>(unreal_struct->binding.property_address));
        return 1;
    }

    int lua_uscript_struct_get_property(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* unreal_struct = get_uscript_struct(state, 1);
        ue4ss::linux::core::ReflectedPropertyDescription property;
        std::string error;
        if (runtime == nullptr || unreal_struct == nullptr ||
            unreal_struct->binding.property_address == 0u ||
            !runtime->describe_property(
                    unreal_struct->binding.property_address, property, error))
        {
            return push_fproperty(state, {});
        }
        return push_fproperty(state, property);
    }

    int lua_uscript_struct_get_struct(lua_State* state) noexcept
    {
        const auto* unreal_struct = get_uscript_struct(state, 1);
        if (unreal_struct == nullptr)
        {
            return luaL_error(state, "UScriptStruct:GetStruct requires UScriptStruct userdata");
        }
        return push_object(state, unreal_struct->binding.script_struct);
    }

    int lua_uscript_struct_is_mapped_to_object(lua_State* state) noexcept
    {
        const auto* unreal_struct = get_uscript_struct(state, 1);
        lua_pushboolean(
                state,
                unreal_struct != nullptr &&
                        unreal_struct->binding.storage_address != 0u);
        return 1;
    }

    int lua_uscript_struct_is_mapped_to_property(lua_State* state) noexcept
    {
        const auto* unreal_struct = get_uscript_struct(state, 1);
        lua_pushboolean(
                state,
                unreal_struct != nullptr &&
                        unreal_struct->binding.property_address != 0u);
        return 1;
    }

    int lua_uscript_struct_get_base_address_unavailable(lua_State* state) noexcept
    {
        return luaL_error(
                state,
                "WARNING! Use of deprecated & removed function 'UScriptStruct::GetBaseAddress'!");
    }

    int lua_uscript_struct_index(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* unreal_struct = get_uscript_struct(state, 1);
        if (runtime == nullptr || unreal_struct == nullptr ||
            lua_type(state, 2) != LUA_TSTRING)
        {
            return luaL_error(
                    state,
                    "UScriptStruct member access requires live userdata and a string key");
        }

        luaL_getmetatable(state, k_uscript_struct_metatable);
        lua_getfield(state, -1, "__methods");
        lua_pushvalue(state, 2);
        lua_rawget(state, -2);
        if (!lua_isnil(state, -1))
        {
            return 1;
        }
        lua_pop(state, 3);

        std::size_t length{};
        const char* name = lua_tolstring(state, 2, &length);
        ue4ss::linux::core::UnrealPropertyValue value;
        std::string error;
        if (!runtime->read_struct_property(
                    unreal_struct->binding,
                    std::string_view{name, length},
                    value,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        if (value.kind == ue4ss::linux::core::UnrealPropertyKind::structure)
        {
            ue4ss::linux::core::UnrealStructBinding nested;
            if (!runtime->bind_nested_struct_property(
                        unreal_struct->binding,
                        std::string_view{name, length},
                        nested,
                        error))
            {
                return luaL_error(state, "%s", error.c_str());
            }
            return push_struct_binding(state, nested);
        }
        return push_property_value(state, value);
    }

    int lua_uscript_struct_newindex(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        auto* unreal_struct = get_uscript_struct(state, 1);
        if (runtime == nullptr || scheduler == nullptr || unreal_struct == nullptr ||
            lua_type(state, 2) != LUA_TSTRING)
        {
            return luaL_error(
                    state,
                    "UScriptStruct field write requires live userdata, scheduler, and string key");
        }
        std::size_t length{};
        const char* name = lua_tolstring(state, 2, &length);
        const std::string_view property_name{name, length};
        ue4ss::linux::core::UnrealPropertyValue current;
        ue4ss::linux::core::UnrealPropertyValue value;
        std::string error;
        if (!runtime->read_struct_property(
                    unreal_struct->binding, property_name, current, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        if (current.kind == ue4ss::linux::core::UnrealPropertyKind::floating_point &&
            lua_isnumber(state, 3))
        {
            value.kind = ue4ss::linux::core::UnrealPropertyKind::floating_point;
            value.floating_point = static_cast<double>(lua_tonumber(state, 3));
        }
        else if (!lua_to_untyped_property_value(state, 3, value, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        if (!runtime->write_struct_property(
                    unreal_struct->binding,
                    property_name,
                    value,
                    *scheduler,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_fproperty_get_name(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr) return luaL_error(state, "FProperty:GetName requires FProperty userdata");
        lua_pushlstring(state, property->description.name.data(), property->description.name.size());
        return 1;
    }

    int lua_fproperty_get_address(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr) return luaL_error(state, "FProperty:GetAddress requires FProperty userdata");
        lua_pushinteger(state, static_cast<lua_Integer>(property->description.address));
        return 1;
    }

    int lua_fproperty_is_valid(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* property = get_fproperty(state, 1);
        ue4ss::linux::core::ReflectedPropertyDescription current;
        std::string error;
        lua_pushboolean(
                state,
                runtime != nullptr && property != nullptr &&
                        property->description.address != 0u &&
                        runtime->describe_property(
                                property->description.address, current, error) &&
                        current.name == property->description.name &&
                        current.type_name == property->description.type_name);
        return 1;
    }

    int lua_fproperty_get_full_name(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr)
        {
            return luaL_error(state, "FProperty:GetFullName requires FProperty userdata");
        }
        std::string path;
        if (runtime != nullptr && property->description.owner_address != 0u)
        {
            ue4ss::linux::core::ReadOnlyUObjectHandle owner;
            ue4ss::linux::core::ReadOnlyUObjectDescription owner_description;
            std::string error;
            if (!runtime->handle_from_address(property->description.owner_address, owner) ||
                !runtime->describe_object(owner, owner_description, error))
            {
                return luaL_error(state,
                                  "FProperty owner could not be described: %s",
                                  error.empty() ? "stale owner" : error.c_str());
            }
            path = owner_description.path_name + ":";
        }
        path += property->description.name;
        const std::string full_name = property->description.type_name + " " + path;
        lua_pushlstring(state, full_name.data(), full_name.size());
        return 1;
    }

    int lua_fproperty_get_fname(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr) return luaL_error(state, "FProperty:GetFName requires FProperty userdata");
        return push_fname(state, property->description.name_private, property->description.name);
    }

    int lua_fproperty_get_class(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr) return luaL_error(state, "FProperty:GetClass requires FProperty userdata");
        return push_ffield_class(state, property->description.type_name);
    }

    int lua_fproperty_get_offset(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        lua_pushinteger(state, property == nullptr ? 0 : property->description.offset);
        return 1;
    }

    int lua_fproperty_get_size(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        lua_pushinteger(state, property == nullptr ? 0 : property->description.element_size);
        return 1;
    }

    int lua_fproperty_get_bool_metadata(lua_State* state, int field) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr || property->description.type_name != "BoolProperty")
        {
            return luaL_error(state, "FBoolProperty metadata method requires BoolProperty userdata");
        }
        const std::uint8_t value = field == 0   ? property->description.bool_byte_mask
                                   : field == 1 ? property->description.bool_byte_offset
                                   : field == 2 ? property->description.bool_field_mask
                                                : property->description.bool_field_size;
        lua_pushinteger(state, static_cast<lua_Integer>(value));
        return 1;
    }

    int lua_fproperty_get_byte_mask(lua_State* state) noexcept
    {
        return lua_fproperty_get_bool_metadata(state, 0);
    }

    int lua_fproperty_get_byte_offset(lua_State* state) noexcept
    {
        return lua_fproperty_get_bool_metadata(state, 1);
    }

    int lua_fproperty_get_field_mask(lua_State* state) noexcept
    {
        return lua_fproperty_get_bool_metadata(state, 2);
    }

    int lua_fproperty_get_field_size(lua_State* state) noexcept
    {
        return lua_fproperty_get_bool_metadata(state, 3);
    }

    int lua_fproperty_push_nested(lua_State* state,
                                  std::uint64_t address,
                                  const char* method_name) noexcept
    {
        auto* runtime = get_runtime(state);
        ue4ss::linux::core::ReflectedPropertyDescription nested;
        std::string error;
        if (runtime == nullptr || address == 0u ||
            !runtime->describe_property(address, nested, error))
        {
            return luaL_error(state,
                              "%s could not describe its referenced FProperty: %s",
                              method_name,
                              error.empty() ? "null property" : error.c_str());
        }
        return push_fproperty(state, nested);
    }

    int lua_fproperty_get_inner(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr ||
            (property->description.type_name != "ArrayProperty" &&
             property->description.type_name != "SetProperty"))
        {
            return luaL_error(state, "GetInner requires ArrayProperty or SetProperty userdata");
        }
        return lua_fproperty_push_nested(
                state, property->description.inner_property_address, "GetInner");
    }

    int lua_fproperty_push_object_reference(lua_State* state,
                                            std::uint64_t address,
                                            const char* method_name) noexcept
    {
        auto* runtime = get_runtime(state);
        ue4ss::linux::core::ReadOnlyUObjectHandle object;
        if (runtime == nullptr || address == 0u || !runtime->handle_from_address(address, object))
        {
            return luaL_error(state, "%s references no live UObject", method_name);
        }
        return push_object(state, object);
    }

    int lua_fproperty_get_property_class(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr)
        {
            return luaL_error(state, "GetPropertyClass requires FObjectPropertyBase userdata");
        }
        return lua_fproperty_push_object_reference(
                state, property->description.property_class_address, "GetPropertyClass");
    }

    int lua_fproperty_get_struct(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr || property->description.type_name != "StructProperty")
        {
            return luaL_error(state, "GetStruct requires StructProperty userdata");
        }
        return lua_fproperty_push_object_reference(
                state, property->description.struct_address, "GetStruct");
    }

    int lua_fproperty_get_enum(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr || property->description.type_name != "EnumProperty")
        {
            return luaL_error(state, "GetEnum requires EnumProperty userdata");
        }
        return lua_fproperty_push_object_reference(
                state, property->description.enum_address, "GetEnum");
    }

    int lua_fproperty_container_ptr_to_value_ptr(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* property = get_fproperty(state, 1);
        const auto* container = get_object(state, 2);
        const lua_Integer requested_index = lua_isnoneornil(state, 3) ? 0 : lua_tointeger(state, 3);
        if (runtime == nullptr || property == nullptr || container == nullptr ||
            (!lua_isnoneornil(state, 3) && !lua_isinteger(state, 3)) ||
            requested_index < 0 ||
            requested_index > std::numeric_limits<std::int32_t>::max())
        {
            return luaL_error(
                    state,
                    "ContainerPtrToValuePtr requires FProperty, UObject, and optional non-negative integer index");
        }
        std::uint64_t address{};
        std::string error;
        if (!runtime->container_ptr_to_value_ptr(
                    property->description,
                    container->handle,
                    static_cast<std::int32_t>(requested_index),
                    address,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_pushlightuserdata(
                state, reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)));
        return 1;
    }

    int lua_fproperty_import_text(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        const auto* property = get_fproperty(state, 1);
        const auto* owner = get_object(state, 5);
        if (scheduler == nullptr)
        {
            return luaL_error(
                    state,
                    "FProperty:ImportText requires the validated game thread scheduler");
        }
        if (runtime == nullptr || scheduler == nullptr || property == nullptr ||
            lua_type(state, 2) != LUA_TSTRING || !lua_islightuserdata(state, 3) ||
            !lua_isinteger(state, 4) || owner == nullptr ||
            lua_tointeger(state, 4) < std::numeric_limits<std::int32_t>::min() ||
            lua_tointeger(state, 4) > std::numeric_limits<std::int32_t>::max())
        {
            return luaL_error(
                    state,
                    "FProperty:ImportText requires text, value lightuserdata, integer port flags, and owner UObject");
        }
        std::size_t text_length{};
        const char* text = lua_tolstring(state, 2, &text_length);
        const auto value_address = static_cast<std::uint64_t>(
                reinterpret_cast<std::uintptr_t>(lua_touserdata(state, 3)));
        std::string error;
        if (!runtime->import_property_text(
                    property->description.address,
                    std::string_view{text, text_length},
                    value_address,
                    static_cast<std::int32_t>(lua_tointeger(state, 4)),
                    owner->handle,
                    *scheduler,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_fproperty_get_interface_class(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* property = get_fproperty(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectHandle interface_class;
        if (runtime == nullptr || property == nullptr ||
            property->description.type_name != "InterfaceProperty" ||
            property->description.interface_class_address == 0u ||
            !runtime->handle_from_address(
                    property->description.interface_class_address, interface_class))
        {
            return luaL_error(
                    state,
                    "FInterfaceProperty:GetInterfaceClass requires validated InterfaceProperty userdata");
        }
        return push_object(state, interface_class);
    }

    int lua_fproperty_is_a(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr || lua_type(state, 2) != LUA_TTABLE)
        {
            return luaL_error(
                    state,
                    "FProperty:IsA requires FProperty userdata and a PropertyTypes entry");
        }
        lua_getfield(state, 2, "Name");
        std::size_t length{};
        const char* name = lua_tolstring(state, -1, &length);
        const bool matches =
                name != nullptr &&
                property->description.type_name == std::string_view{name, length};
        lua_pop(state, 1);
        lua_pushboolean(state, matches ? 1 : 0);
        return 1;
    }

    int lua_fproperty_type(lua_State* state) noexcept
    {
        const auto* property = get_fproperty(state, 1);
        if (property == nullptr)
        {
            lua_pushliteral(state, "FProperty");
        }
        else
        {
            lua_pushlstring(
                    state, property->description.type_name.data(), property->description.type_name.size());
        }
        return 1;
    }

    int lua_ffield_class_get_name(lua_State* state) noexcept
    {
        const auto* field_class = get_ffield_class(state, 1);
        if (field_class == nullptr) return luaL_error(state, "FFieldClass:GetName requires FFieldClass userdata");
        lua_pushlstring(state, field_class->name.data(), field_class->name.size());
        return 1;
    }

    int push_object(lua_State* state, const ue4ss::linux::core::ReadOnlyUObjectHandle& handle) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaReadOnlyUObject), 0);
        new (storage) LuaReadOnlyUObject{handle};
        luaL_getmetatable(state, k_uobject_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int push_thread_id(lua_State* state, std::thread::id value) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaThreadId), 0);
        new (storage) LuaThreadId{value};
        luaL_getmetatable(state, k_thread_id_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int lua_thread_id_gc(lua_State* state) noexcept
    {
        if (auto* thread_id = get_thread_id(state, 1); thread_id != nullptr)
        {
            thread_id->~LuaThreadId();
        }
        return 0;
    }

    int lua_thread_id_equal(lua_State* state) noexcept
    {
        const auto* left = get_thread_id(state, 1);
        const auto* right = get_thread_id(state, 2);
        lua_pushboolean(state,
                        left != nullptr && right != nullptr && left->value == right->value);
        return 1;
    }

    int lua_thread_id_to_string(lua_State* state) noexcept
    {
        const auto* thread_id = get_thread_id(state, 1);
        if (thread_id == nullptr)
        {
            return luaL_error(state, "ThreadId:ToString requires ThreadId userdata");
        }
        try
        {
            std::ostringstream stream;
            stream << thread_id->value;
            const std::string value = stream.str();
            lua_pushlstring(state, value.data(), value.size());
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "ThreadId:ToString failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "ThreadId:ToString failed");
        }
    }

    int lua_thread_id_type(lua_State* state) noexcept
    {
        if (get_thread_id(state, 1) == nullptr)
        {
            return luaL_error(state, "ThreadId:type requires ThreadId userdata");
        }
        lua_pushliteral(state, "ThreadId");
        return 1;
    }

    int lua_create_invalid_object(lua_State* state) noexcept
    {
        return push_object(state, {});
    }

    int push_weak_object(lua_State* state,
                         const ue4ss::linux::core::ReadOnlyUObjectHandle& handle,
                         bool is_null) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaWeakObjectPtr), 0);
        new (storage) LuaWeakObjectPtr{handle, is_null};
        luaL_getmetatable(state, k_weak_object_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int lua_weak_object_get(lua_State* state) noexcept
    {
        const auto* weak = get_weak_object(state, 1);
        if (weak == nullptr || weak->is_null)
        {
            lua_pushnil(state);
            return 1;
        }
        auto* runtime = get_runtime(state);
        if (runtime == nullptr || !runtime->is_valid(weak->handle))
        {
            lua_pushnil(state);
            return 1;
        }
        return push_object(state, weak->handle);
    }

    int lua_weak_object_type(lua_State* state) noexcept
    {
        lua_pushliteral(state, "FWeakObjectPtr");
        return 1;
    }

    int push_remote_context_param(lua_State* state,
                                  ue4ss::linux::core::HeadlessLuaState& owner,
                                  std::uint64_t epoch,
                                  const ue4ss::linux::core::ReadOnlyUObjectHandle& handle) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaRemoteObjectParam), 0);
        new (storage) LuaRemoteObjectParam{&owner, epoch, true, handle, {}};
        luaL_getmetatable(state, k_remote_param_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int push_remote_reflected_param(lua_State* state,
                                    ue4ss::linux::core::HeadlessLuaState& owner,
                                    std::uint64_t epoch,
                                    const ue4ss::linux::core::RemoteUnrealParameter& parameter) noexcept
    {
        void* storage = lua_newuserdatauv(state, sizeof(LuaRemoteObjectParam), 0);
        new (storage) LuaRemoteObjectParam{&owner, epoch, false, {}, parameter};
        luaL_getmetatable(state, k_remote_param_metatable);
        lua_setmetatable(state, -2);
        return 1;
    }

    int lua_remote_param_get(lua_State* state) noexcept
    {
        bool failed{};
        int return_count{};
        {
            auto* runtime = get_runtime(state);
            auto* param = static_cast<LuaRemoteObjectParam*>(luaL_testudata(state, 1, k_remote_param_metatable));
            std::string error;
            if (runtime == nullptr || param == nullptr || param->owner == nullptr)
            {
                error = "RemoteUnrealParam lost its validated runtime context";
            }
            else if (!param->owner->is_hook_callback_scope_active(param->epoch))
            {
                error = "RemoteUnrealParam is no longer valid outside its synchronous hook callback";
            }
            else if (param->context_parameter)
            {
                if (!runtime->is_valid(param->handle))
                {
                    lua_pushnil(state);
                    return_count = 1;
                }
                else
                {
                    return_count = push_object(state, param->handle);
                }
            }
            else
            {
                ue4ss::linux::core::UnrealPropertyValue value;
                if (runtime->read_hook_parameter(param->parameter, value, error))
                {
                    return_count = push_property_value(state, value);
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : return_count;
    }

    [[nodiscard]] bool lua_to_remote_property_value(
            lua_State* state,
            int index,
            ue4ss::linux::core::UnrealPropertyKind expected,
            ue4ss::linux::core::UnrealPropertyValue& output,
            std::string& error) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        output = {};
        output.kind = expected;
        switch (expected)
        {
        case UnrealPropertyKind::boolean:
            if (!lua_isboolean(state, index))
            {
                error = "Remote BoolProperty assignment requires a Lua boolean";
                return false;
            }
            output.boolean = lua_toboolean(state, index) != 0;
            return true;
        case UnrealPropertyKind::signed_integer:
            if (!lua_isinteger(state, index))
            {
                error = "Remote signed integer assignment requires a Lua integer";
                return false;
            }
            output.signed_integer = static_cast<std::int64_t>(lua_tointeger(state, index));
            return true;
        case UnrealPropertyKind::unsigned_integer:
            if (!lua_isinteger(state, index) || lua_tointeger(state, index) < 0)
            {
                error = "Remote unsigned integer assignment requires a non-negative Lua integer";
                return false;
            }
            output.unsigned_integer = static_cast<std::uint64_t>(lua_tointeger(state, index));
            return true;
        case UnrealPropertyKind::floating_point:
            if (!lua_isnumber(state, index))
            {
                error = "Remote floating-point assignment requires a Lua number";
                return false;
            }
            output.floating_point = static_cast<double>(lua_tonumber(state, index));
            return true;
        case UnrealPropertyKind::object:
            if (lua_isnil(state, index))
            {
                output.object_is_null = true;
                return true;
            }
            if (auto* object = get_object(state, index); object != nullptr)
            {
                output.object = object->handle;
                return true;
            }
            error = "Remote UObject assignment requires nil or a UObject";
            return false;
        case UnrealPropertyKind::name:
            if (auto* name = get_fname(state, index); name != nullptr)
            {
                output.name = name->value;
                output.text = name->text;
                return true;
            }
            error = "Remote NameProperty assignment requires FName userdata";
            return false;
        case UnrealPropertyKind::string:
            if (auto* string = get_fstring(state, index); string != nullptr)
            {
                output.text = string->value;
                return true;
            }
            if (lua_type(state, index) == LUA_TSTRING)
            {
                std::size_t length{};
                const char* string = lua_tolstring(state, index, &length);
                output.text.assign(string, length);
                return true;
            }
            error = "Remote StrProperty assignment requires a string or FString";
            return false;
        case UnrealPropertyKind::text:
            if (auto* text_value = get_ftext(state, index); text_value != nullptr)
            {
                output.text = text_value->value;
                return true;
            }
            error = "Remote TextProperty assignment requires FText userdata";
            return false;
        case UnrealPropertyKind::structure:
            if (!lua_to_untyped_property_value(state, index, output, error))
            {
                return false;
            }
            if (output.kind != UnrealPropertyKind::structure)
            {
                error = "Remote StructProperty assignment requires a Lua table";
                return false;
            }
            return true;
        case UnrealPropertyKind::array:
            if (auto* array = get_tarray(state, index); array != nullptr)
            {
                output.kind = UnrealPropertyKind::array;
                output.array_elements = array->elements;
                return true;
            }
            if (lua_type(state, index) == LUA_TTABLE &&
                lua_to_untyped_property_value(state, index, output, error) &&
                (output.kind == UnrealPropertyKind::array ||
                 (output.kind == UnrealPropertyKind::structure && output.struct_fields.empty())))
            {
                output.kind = UnrealPropertyKind::array;
                return true;
            }
            error = error.empty() ? "Remote ArrayProperty assignment requires TArray userdata or an array table" : error;
            return false;
        case UnrealPropertyKind::set:
            if (auto* set = get_tset(state, index); set != nullptr)
            {
                output.set_elements = set->elements;
                return true;
            }
            error = "Remote SetProperty assignment requires TSet userdata";
            return false;
        case UnrealPropertyKind::map:
            if (auto* map = get_tmap(state, index); map != nullptr)
            {
                output.map_entries = map->entries;
                return true;
            }
            error = "Remote MapProperty assignment requires TMap userdata";
            return false;
        case UnrealPropertyKind::weak_object:
            error = "Remote WeakObjectProperty assignment is not supported";
            return false;
        case UnrealPropertyKind::soft_object:
            if (auto* soft = get_soft_object(state, index); soft != nullptr)
            {
                output.object = soft->weak_object;
                output.object_is_null = soft->weak_is_null;
                output.signed_integer = soft->tag_at_last_test;
                output.name = soft->asset_path_name;
                output.text = soft->asset_path_text;
                output.soft_package_name = soft->package_name;
                output.soft_asset_name = soft->asset_name;
                output.soft_package_text = soft->package_text;
                output.soft_asset_text = soft->asset_text;
                output.soft_sub_path = soft->sub_path;
                return true;
            }
            error = "Remote SoftObjectProperty assignment requires TSoftObjectPtr userdata";
            return false;
        case UnrealPropertyKind::delegate:
            error = "Remote DelegateProperty assignment requires the weak-pointer assignment bridge";
            return false;
        case UnrealPropertyKind::multicast_delegate:
            error = "Remote multicast delegates must be changed through Add/Remove/Clear";
            return false;
        }
        error = "RemoteUnrealParam has an invalid reflected kind";
        return false;
    }

    int lua_remote_param_set(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* runtime = get_runtime(state);
            auto* param = static_cast<LuaRemoteObjectParam*>(luaL_testudata(state, 1, k_remote_param_metatable));
            std::string error;
            if (runtime == nullptr || param == nullptr || param->owner == nullptr)
            {
                error = "RemoteUnrealParam lost its validated runtime context";
            }
            else if (!param->owner->is_hook_callback_scope_active(param->epoch))
            {
                error = "RemoteUnrealParam is no longer valid outside its synchronous hook callback";
            }
            else if (param->context_parameter)
            {
                error = "the hook Context pointer cannot be replaced";
            }
            else
            {
                ue4ss::linux::core::UnrealPropertyValue value;
                if (lua_to_remote_property_value(state, 2, param->parameter.kind, value, error))
                {
                    static_cast<void>(runtime->write_hook_parameter(param->parameter, value, error));
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int lua_remote_param_type(lua_State* state) noexcept
    {
        if (luaL_testudata(state, 1, k_remote_param_metatable) == nullptr)
        {
            return luaL_error(state, "RemoteUnrealParam:type requires RemoteUnrealParam userdata");
        }
        lua_pushliteral(state, "RemoteUnrealParam");
        return 1;
    }

    int lua_uobject_is_valid(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            auto* object = get_object(state, 1);
            lua_pushboolean(state, runtime != nullptr && object != nullptr && runtime->is_valid(object->handle));
            return 1;
        }
        catch (...)
        {
            lua_pushboolean(state, 0);
            return 1;
        }
    }

    int lua_uobject_get_address(lua_State* state) noexcept
    {
        const auto* object = get_object(state, 1);
        if (object == nullptr)
        {
            return luaL_error(state, "UObject:GetAddress requires UObject userdata");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(object->handle.address));
        return 1;
    }

    int push_description_field(lua_State* state, int field) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            auto* object = get_object(state, 1);
            ue4ss::linux::core::ReadOnlyUObjectDescription description;
            std::string error;
            if (runtime == nullptr || object == nullptr || !runtime->describe_object(object->handle, description, error))
            {
                lua_pushnil(state);
                return 1;
            }
            const std::string* value = field == 0 ? &description.name : field == 1 ? &description.path_name : &description.full_name;
            lua_pushlstring(state, value->data(), value->size());
            return 1;
        }
        catch (...)
        {
            lua_pushnil(state);
            return 1;
        }
    }

    int lua_uobject_get_name(lua_State* state) noexcept
    {
        return push_description_field(state, 0);
    }

    int lua_uobject_get_path_name(lua_State* state) noexcept
    {
        return push_description_field(state, 1);
    }

    int lua_uobject_get_full_name(lua_State* state) noexcept
    {
        return push_description_field(state, 2);
    }

    int lua_uobject_get_class(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            auto* object = get_object(state, 1);
            ue4ss::linux::core::ReadOnlyUObjectDescription description;
            ue4ss::linux::core::ReadOnlyUObjectHandle class_handle;
            std::string error;
            if (runtime == nullptr || object == nullptr || !runtime->describe_object(object->handle, description, error) ||
                !runtime->handle_from_address(description.class_address, class_handle))
            {
                lua_pushnil(state);
                return 1;
            }
            return push_object(state, class_handle);
        }
        catch (...)
        {
            lua_pushnil(state);
            return 1;
        }
    }

    int lua_uobject_get_fname(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectDescription description;
        std::string error;
        if (runtime == nullptr || object == nullptr ||
            !runtime->describe_object(object->handle, description, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UObject:GetFName requires a live UObject" : error.c_str());
        }
        return push_fname(state, description.name_private, description.name);
    }

    int lua_uobject_get_outer(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectDescription description;
        std::string error;
        if (runtime == nullptr || object == nullptr ||
            !runtime->describe_object(object->handle, description, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UObject:GetOuter requires a live UObject" : error.c_str());
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle outer;
        if (description.outer_address != 0u &&
            !runtime->handle_from_address(description.outer_address, outer))
        {
            return luaL_error(state, "UObject::OuterPrivate is outside the validated object registry");
        }
        return push_object(state, outer);
    }

    int lua_uobject_get_world(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectHandle world;
        std::string error;
        if (runtime == nullptr || object == nullptr ||
            !runtime->find_outer_of_class(object->handle, "World", true, world, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UObject:GetWorld requires a live UObject" : error.c_str());
        }
        return push_object(state, world);
    }

    int lua_aactor_get_level(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* actor = get_object(state, 1);
        std::string error;
        if (runtime == nullptr || actor == nullptr ||
            !runtime->is_a(actor->handle, "Actor", error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "AActor:GetLevel requires a live AActor" : error.c_str());
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle level;
        if (!runtime->find_outer_of_class(actor->handle, "Level", false, level, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return push_object(state, level);
    }

    int lua_uobject_is_class(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        std::string error;
        const bool result = runtime != nullptr && object != nullptr &&
                            runtime->is_a(object->handle, "Class", error);
        if (!error.empty())
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_pushboolean(state, result);
        return 1;
    }

    int lua_uobject_has_flags(lua_State* state, bool require_all, bool internal) noexcept
    {
        if (!lua_isinteger(state, 2))
        {
            return luaL_error(state, "UObject flag query requires an integer flag mask");
        }
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectDescription description;
        std::string error;
        if (runtime == nullptr || object == nullptr ||
            !runtime->describe_object(object->handle, description, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UObject flag query requires a live UObject" : error.c_str());
        }
        const std::uint32_t requested = static_cast<std::uint32_t>(lua_tointeger(state, 2));
        const std::uint32_t flags = internal
                                            ? static_cast<std::uint32_t>(description.internal_object_flags)
                                            : description.object_flags;
        lua_pushboolean(state, require_all ? (flags & requested) == requested
                                           : (flags & requested) != 0u);
        return 1;
    }

    int lua_uobject_has_all_flags(lua_State* state) noexcept
    {
        return lua_uobject_has_flags(state, true, false);
    }

    int lua_uobject_has_any_flags(lua_State* state) noexcept
    {
        return lua_uobject_has_flags(state, false, false);
    }

    int lua_uobject_has_any_internal_flags(lua_State* state) noexcept
    {
        return lua_uobject_has_flags(state, false, true);
    }

    int lua_uobject_type(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        if (runtime == nullptr || object == nullptr)
        {
            return luaL_error(state, "UObject:type requires UObject userdata");
        }
        if (!runtime->is_valid(object->handle))
        {
            lua_pushliteral(state, "UObject");
            return 1;
        }
        struct Candidate
        {
            const char* unreal_name;
            const char* lua_name;
        };
        constexpr std::array candidates{
                Candidate{"Function", "UFunction"},
                Candidate{"Class", "UClass"},
                Candidate{"ScriptStruct", "UScriptStruct"},
                Candidate{"DataTable", "UDataTable"},
                Candidate{"Struct", "UStruct"},
                Candidate{"Enum", "UEnum"},
                Candidate{"World", "UWorld"},
                Candidate{"Actor", "AActor"},
        };
        for (const auto& candidate : candidates)
        {
            std::string error;
            if (runtime->is_a(object->handle, candidate.unreal_name, error))
            {
                lua_pushstring(state, candidate.lua_name);
                return 1;
            }
            if (!error.empty())
            {
                return luaL_error(state, "%s", error.c_str());
            }
        }
        lua_pushliteral(state, "UObject");
        return 1;
    }

    int lua_uclass_get_cdo(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* unreal_class = get_object(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectHandle cdo;
        std::string error;
        if (runtime == nullptr || unreal_class == nullptr ||
            !runtime->get_class_default_object(unreal_class->handle, cdo, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UClass:GetCDO requires a live UClass" : error.c_str());
        }
        return push_object(state, cdo);
    }

    int lua_uobject_is_a(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            auto* object = get_object(state, 1);
            auto* requested_class = get_object(state, 2);
            std::string error;
            const bool matches = runtime != nullptr && object != nullptr && requested_class != nullptr &&
                                 runtime->is_a(object->handle, requested_class->handle, error);
            lua_pushboolean(state, matches);
            return 1;
        }
        catch (...)
        {
            lua_pushboolean(state, 0);
            return 1;
        }
    }

    int lua_uobject_implements_interface(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            auto* object = get_object(state, 1);
            auto* interface_class = get_object(state, 2);
            std::string error;
            const bool matches = runtime != nullptr && object != nullptr && interface_class != nullptr &&
                                 runtime->implements_interface(
                                         object->handle, interface_class->handle, error);
            lua_pushboolean(state, matches);
            return 1;
        }
        catch (...)
        {
            lua_pushboolean(state, 0);
            return 1;
        }
    }

    int lua_ustruct_for_each_property(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* unreal_struct = get_object(state, 1);
        if (runtime == nullptr || unreal_struct == nullptr || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "UStruct:ForEachProperty requires a live UStruct and callback");
        }
        std::vector<ue4ss::linux::core::ReflectedPropertyDescription> properties;
        std::string error;
        if (!runtime->enumerate_properties(unreal_struct->handle, properties, error, true))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        for (const auto& property : properties)
        {
            lua_pushvalue(state, 2);
            push_fproperty(state, property);
            if (lua_pcall(state, 1, 1, 0) != LUA_OK) return lua_error(state);
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop) break;
        }
        return 0;
    }

    int lua_uobject_get_property_value(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        if (runtime == nullptr || object == nullptr || lua_type(state, 2) != LUA_TSTRING)
        {
            return luaL_error(
                    state,
                    "UObject:GetPropertyValue requires a live UObject and property name");
        }
        std::size_t length{};
        const char* name = lua_tolstring(state, 2, &length);
        const std::string_view property_name{name, length};
        ue4ss::linux::core::UnrealPropertyValue value;
        std::string error;
        bool read = runtime->read_property(object->handle, property_name, value, error);
        const bool reflected = read;
        bool custom_found{};
        if (!read)
        {
            const std::string reflected_error = error;
            error.clear();
            read = runtime->read_registered_custom_property(
                    object->handle,
                    property_name,
                    value,
                    custom_found,
                    error);
            if (!custom_found)
            {
                error = reflected_error;
            }
        }
        if (!read)
        {
            return luaL_error(
                    state,
                    "%s",
                    error.empty() ? "reflected property read failed" : error.c_str());
        }
        if (value.kind == ue4ss::linux::core::UnrealPropertyKind::multicast_delegate)
        {
            return push_live_multicast_delegate(
                    state, object->handle, property_name, value.multicast_sparse);
        }
        if (reflected && value.kind == ue4ss::linux::core::UnrealPropertyKind::structure)
        {
            ue4ss::linux::core::UnrealStructBinding binding;
            if (!runtime->bind_struct_property(
                        object->handle, property_name, binding, error))
            {
                return luaL_error(state, "%s", error.c_str());
            }
            return push_struct_binding(state, binding);
        }
        return push_property_value(state, value);
    }

    int lua_reflection_get_property(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        if (runtime == nullptr || lua_type(state, 1) != LUA_TTABLE ||
            lua_type(state, 2) != LUA_TSTRING)
        {
            return luaL_error(state, "Reflection:GetProperty requires a reflection table and property name");
        }
        lua_getfield(state, 1, "ReflectedObject");
        const auto* reflected_object = get_object(state, -1);
        if (reflected_object == nullptr)
        {
            lua_pop(state, 1);
            return luaL_error(state, "Reflection table lost its ReflectedObject");
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle unreal_struct = reflected_object->handle;
        std::string error;
        const bool is_struct = runtime->is_a(unreal_struct, "Struct", error);
        error.clear();
        const bool is_class = runtime->is_a(unreal_struct, "Class", error);
        if (!is_struct && !is_class)
        {
            ue4ss::linux::core::ReadOnlyUObjectDescription description;
            if (!runtime->describe_object(reflected_object->handle, description, error) ||
                !runtime->handle_from_address(description.class_address, unreal_struct))
            {
                lua_pop(state, 1);
                return luaL_error(
                        state,
                        "Reflection:GetProperty could not resolve the UObject class: %s",
                        error.c_str());
            }
        }
        lua_pop(state, 1);
        std::size_t length{};
        const char* name = lua_tolstring(state, 2, &length);
        std::vector<ue4ss::linux::core::ReflectedPropertyDescription> properties;
        if (!runtime->enumerate_properties(unreal_struct, properties, error, true))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const std::string_view requested{name, length};
        const auto found = std::find_if(properties.begin(), properties.end(), [&](const auto& property) {
            return property.name == requested;
        });
        return push_fproperty(
                state,
                found == properties.end()
                        ? ue4ss::linux::core::ReflectedPropertyDescription{}
                        : *found);
    }

    int lua_uobject_reflection(lua_State* state) noexcept
    {
        auto* object = get_object(state, 1);
        if (object == nullptr)
        {
            return luaL_error(state, "UObject:Reflection requires UObject userdata");
        }
        lua_createtable(state, 0, 2);
        lua_pushvalue(state, 1);
        lua_setfield(state, -2, "ReflectedObject");
        lua_pushcfunction(state, lua_reflection_get_property);
        lua_setfield(state, -2, "GetProperty");
        return 1;
    }

    [[nodiscard]] bool read_data_table_rows(
            lua_State* state,
            ue4ss::linux::core::ReadOnlyUObjectHandle& row_struct,
            std::vector<ue4ss::linux::core::UnrealDataTableRow>& rows,
            std::string& error) noexcept
    {
        auto* runtime = get_runtime(state);
        const auto* data_table = get_object(state, 1);
        return runtime != nullptr && data_table != nullptr &&
               runtime->enumerate_data_table_rows(
                       data_table->handle, row_struct, rows, error);
    }

    int lua_udata_table_get_row_struct(lua_State* state) noexcept
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return push_object(state, row_struct);
    }

    int lua_udata_table_get_row_map(lua_State* state) noexcept
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_createtable(
                state,
                0,
                static_cast<int>(std::min<std::size_t>(
                        rows.size(),
                        static_cast<std::size_t>(std::numeric_limits<int>::max()))));
        for (const auto& row : rows)
        {
            lua_pushlstring(state, row.name_text.data(), row.name_text.size());
            push_struct_binding(state, row.data);
            lua_rawset(state, -3);
        }
        return 1;
    }

    int lua_udata_table_find_row(lua_State* state) noexcept
    {
        std::string_view requested;
        if (const auto* name = get_fname(state, 2); name != nullptr)
        {
            requested = name->text;
        }
        else if (lua_type(state, 2) == LUA_TSTRING)
        {
            std::size_t length{};
            const char* text = lua_tolstring(state, 2, &length);
            requested = std::string_view{text, length};
        }
        else
        {
            return luaL_error(state, "UDataTable:FindRow expects a row name string or FName");
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        const auto found = std::find_if(rows.begin(), rows.end(), [&](const auto& row) {
            return row.name_text == requested;
        });
        if (found == rows.end())
        {
            lua_pushnil(state);
            return 1;
        }
        return push_struct_binding(state, found->data);
    }

    int lua_udata_table_get_row_names(lua_State* state) noexcept
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_createtable(
                state,
                static_cast<int>(std::min<std::size_t>(
                        rows.size(),
                        static_cast<std::size_t>(std::numeric_limits<int>::max()))),
                0);
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            lua_pushlstring(
                    state, rows[index].name_text.data(), rows[index].name_text.size());
            lua_rawseti(state, -2, static_cast<lua_Integer>(index + 1u));
        }
        return 1;
    }

    int lua_udata_table_get_all_rows(lua_State* state) noexcept
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_createtable(
                state,
                static_cast<int>(std::min<std::size_t>(
                        rows.size(),
                        static_cast<std::size_t>(std::numeric_limits<int>::max()))),
                0);
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            lua_createtable(state, 0, 2);
            lua_pushlstring(
                    state, rows[index].name_text.data(), rows[index].name_text.size());
            lua_setfield(state, -2, "Name");
            push_struct_binding(state, rows[index].data);
            lua_setfield(state, -2, "Data");
            lua_rawseti(state, -2, static_cast<lua_Integer>(index + 1u));
        }
        return 1;
    }

    int lua_udata_table_for_each_row(lua_State* state) noexcept
    {
        if (lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "UDataTable:ForEachRow requires a callback");
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        for (const auto& row : rows)
        {
            lua_pushvalue(state, 2);
            lua_pushlstring(state, row.name_text.data(), row.name_text.size());
            push_struct_binding(state, row.data);
            if (lua_pcall(state, 2, 1, 0) != LUA_OK) return lua_error(state);
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop) break;
        }
        return 0;
    }

    int lua_udata_table_len(lua_State* state) noexcept
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle row_struct;
        std::vector<ue4ss::linux::core::UnrealDataTableRow> rows;
        std::string error;
        if (!read_data_table_rows(state, row_struct, rows, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_pushinteger(state, static_cast<lua_Integer>(rows.size()));
        return 1;
    }

    [[nodiscard]] bool lua_data_table_row_name(
            lua_State* state,
            int index,
            ue4ss::linux::core::RawFName& output,
            std::string& error) noexcept
    {
        if (const auto* name = get_fname(state, index); name != nullptr)
        {
            output = name->value;
            return true;
        }
        auto* runtime = get_runtime(state);
        if (runtime == nullptr || lua_type(state, index) != LUA_TSTRING)
        {
            error = "UDataTable row name must be a string or FName";
            return false;
        }
        std::size_t length{};
        const char* name = lua_tolstring(state, index, &length);
        return runtime->fname_from_utf8(
                std::string_view{name, length}, output, true, error);
    }

    int lua_udata_table_add_row(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        const auto* data_table = get_object(state, 1);
        if (runtime == nullptr || scheduler == nullptr || data_table == nullptr)
        {
            return luaL_error(
                    state,
                    "UDataTable:AddRow requires a live UDataTable and game-thread scheduler");
        }
        ue4ss::linux::core::RawFName row_name;
        ue4ss::linux::core::UnrealPropertyValue row_value;
        std::string error;
        if (!lua_data_table_row_name(state, 2, row_name, error) ||
            !lua_to_untyped_property_value(state, 3, row_value, error) ||
            !runtime->add_data_table_row(
                    data_table->handle,
                    row_name,
                    row_value,
                    *scheduler,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_udata_table_remove_row(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        const auto* data_table = get_object(state, 1);
        if (runtime == nullptr || scheduler == nullptr || data_table == nullptr)
        {
            return luaL_error(
                    state,
                    "UDataTable:RemoveRow requires a live UDataTable and game-thread scheduler");
        }
        ue4ss::linux::core::RawFName row_name;
        std::string error;
        if (!lua_data_table_row_name(state, 2, row_name, error) ||
            !runtime->remove_data_table_row(
                    data_table->handle, row_name, *scheduler, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_udata_table_empty(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        const auto* data_table = get_object(state, 1);
        std::string error;
        if (runtime == nullptr || scheduler == nullptr || data_table == nullptr)
        {
            return luaL_error(
                    state,
                    "UDataTable:EmptyTable requires a live UDataTable and game-thread scheduler");
        }
        if (!runtime->empty_data_table(data_table->handle, *scheduler, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_ustruct_get_super_struct(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* unreal_struct = get_object(state, 1);
        ue4ss::linux::core::ReadOnlyUObjectHandle super_struct;
        std::string error;
        if (runtime == nullptr || unreal_struct == nullptr ||
            !runtime->get_super_struct(unreal_struct->handle, super_struct, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UStruct:GetSuperStruct requires a live UStruct" : error.c_str());
        }
        return push_object(state, super_struct);
    }

    int lua_uclass_is_child_of(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* unreal_class = get_object(state, 1);
        auto* requested_parent = get_object(state, 2);
        bool output{};
        std::string error;
        if (runtime == nullptr || unreal_class == nullptr || requested_parent == nullptr ||
            !runtime->struct_is_child_of(
                    unreal_class->handle, requested_parent->handle, output, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UClass:IsChildOf requires two live UClass values" : error.c_str());
        }
        lua_pushboolean(state, output);
        return 1;
    }

    int lua_ustruct_for_each_function(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* unreal_struct = get_object(state, 1);
        if (runtime == nullptr || unreal_struct == nullptr || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "UStruct:ForEachFunction requires a live UStruct and callback");
        }
        std::vector<ue4ss::linux::core::ReadOnlyUObjectHandle> functions;
        std::string error;
        if (!runtime->enumerate_functions(unreal_struct->handle, functions, error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        for (const auto& function : functions)
        {
            lua_pushvalue(state, 2);
            push_object(state, function);
            if (lua_pcall(state, 1, 1, 0) != LUA_OK)
            {
                return lua_error(state);
            }
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop)
            {
                break;
            }
        }
        return 0;
    }

    int lua_ufunction_get_flags(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* function = get_object(state, 1);
        std::uint32_t flags{};
        std::string error;
        if (runtime == nullptr || function == nullptr ||
            !runtime->get_function_flags(function->handle, flags, error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UFunction:GetFunctionFlags requires a live UFunction" : error.c_str());
        }
        lua_pushinteger(state, static_cast<lua_Integer>(flags));
        return 1;
    }

    int lua_ufunction_set_flags(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 2))
        {
            return luaL_error(state, "UFunction:SetFunctionFlags requires an integer flag value");
        }
        auto* runtime = get_runtime(state);
        auto* function = get_object(state, 1);
        std::string error;
        if (runtime == nullptr || function == nullptr ||
            !runtime->set_function_flags(
                    function->handle,
                    static_cast<std::uint32_t>(lua_tointeger(state, 2)),
                    error))
        {
            return luaL_error(state,
                              "%s",
                              error.empty() ? "UFunction:SetFunctionFlags requires a live UFunction" : error.c_str());
        }
        return 0;
    }

    [[nodiscard]] bool read_lua_uenum(lua_State* state,
                                      int object_index,
                                      std::vector<ue4ss::linux::core::UnrealEnumEntry>& entries,
                                      std::string& error) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, object_index);
        if (runtime == nullptr || object == nullptr)
        {
            error = "UEnum method requires a live UObject runtime";
            return false;
        }
        return runtime->enumerate_enum_names(object->handle, entries, error);
    }

    int lua_uenum_get_name_by_value(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 2))
        {
            return luaL_error(state, "UEnum:GetNameByValue requires an integer value");
        }
        std::vector<ue4ss::linux::core::UnrealEnumEntry> entries;
        std::string error;
        if (!read_lua_uenum(state, 1, entries, error)) return luaL_error(state, "%s", error.c_str());
        const std::int64_t requested = static_cast<std::int64_t>(lua_tointeger(state, 2));
        const auto found = std::find_if(entries.begin(), entries.end(), [requested](const auto& entry) {
            return entry.value == requested;
        });
        return found == entries.end() ? push_fname(state, {}, "None")
                                      : push_fname(state, found->name, found->text);
    }

    int lua_uenum_get_name_by_index(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 2) || lua_tointeger(state, 2) < 0)
        {
            return luaL_error(state, "UEnum:GetEnumNameByIndex requires a non-negative integer index");
        }
        std::vector<ue4ss::linux::core::UnrealEnumEntry> entries;
        std::string error;
        if (!read_lua_uenum(state, 1, entries, error)) return luaL_error(state, "%s", error.c_str());
        const auto index = static_cast<std::size_t>(lua_tointeger(state, 2));
        if (index >= entries.size()) return luaL_error(state, "UEnum index is out of bounds");
        push_fname(state, entries[index].name, entries[index].text);
        lua_pushinteger(state, static_cast<lua_Integer>(entries[index].value));
        return 2;
    }

    int lua_uenum_for_each_name(lua_State* state) noexcept
    {
        if (lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "UEnum:ForEachName requires a Lua callback");
        }
        std::vector<ue4ss::linux::core::UnrealEnumEntry> entries;
        std::string error;
        if (!read_lua_uenum(state, 1, entries, error)) return luaL_error(state, "%s", error.c_str());
        for (const auto& entry : entries)
        {
            lua_pushvalue(state, 2);
            push_fname(state, entry.name, entry.text);
            lua_pushinteger(state, static_cast<lua_Integer>(entry.value));
            if (lua_pcall(state, 2, 1, 0) != LUA_OK) return lua_error(state);
            const bool stop = lua_isboolean(state, -1) && lua_toboolean(state, -1) != 0;
            lua_pop(state, 1);
            if (stop) break;
        }
        return 0;
    }

    int lua_uenum_num_enums(lua_State* state) noexcept
    {
        std::vector<ue4ss::linux::core::UnrealEnumEntry> entries;
        std::string error;
        if (!read_lua_uenum(state, 1, entries, error)) return luaL_error(state, "%s", error.c_str());
        lua_pushinteger(state, static_cast<lua_Integer>(entries.size()));
        return 1;
    }

    int lua_uenum_edit_name_at(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        auto* unreal_enum = get_object(state, 1);
        if (scheduler == nullptr)
        {
            return luaL_error(state, "UEnum mutation requires the validated game-thread scheduler");
        }
        if (runtime == nullptr || scheduler == nullptr || unreal_enum == nullptr ||
            !lua_isinteger(state, 2) || lua_tointeger(state, 2) < 0 ||
            lua_type(state, 3) != LUA_TSTRING)
        {
            return luaL_error(state, "UEnum:EditNameAt requires index and new name string");
        }
        std::size_t length{};
        const char* name = lua_tolstring(state, 3, &length);
        ue4ss::linux::core::RawFName unreal_name;
        std::string error;
        if (!runtime->fname_from_utf8(
                    std::string_view{name, length}, unreal_name, true, error) ||
            !runtime->edit_enum_name(
                    unreal_enum->handle,
                    static_cast<std::int32_t>(lua_tointeger(state, 2)),
                    unreal_name,
                    *scheduler,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_uenum_edit_value_at(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        auto* unreal_enum = get_object(state, 1);
        std::string error;
        if (scheduler == nullptr)
        {
            return luaL_error(state, "UEnum mutation requires the validated game-thread scheduler");
        }
        if (runtime == nullptr || scheduler == nullptr || unreal_enum == nullptr ||
            !lua_isinteger(state, 2) || lua_tointeger(state, 2) < 0 ||
            !lua_isinteger(state, 3) ||
            !runtime->edit_enum_value(
                    unreal_enum->handle,
                    static_cast<std::int32_t>(lua_tointeger(state, 2)),
                    static_cast<std::int64_t>(lua_tointeger(state, 3)),
                    *scheduler,
                    error))
        {
            return luaL_error(
                    state,
                    "%s",
                    error.empty() ? "UEnum:EditValueAt requires index and integer value"
                                  : error.c_str());
        }
        return 0;
    }

    int lua_uenum_insert_into_names(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        auto* unreal_enum = get_object(state, 1);
        if (scheduler == nullptr)
        {
            return luaL_error(state, "UEnum mutation requires the validated game-thread scheduler");
        }
        if (runtime == nullptr || scheduler == nullptr || unreal_enum == nullptr ||
            lua_type(state, 2) != LUA_TSTRING || !lua_isinteger(state, 3) ||
            !lua_isinteger(state, 4) || lua_tointeger(state, 4) < 0 ||
            (lua_gettop(state) >= 5 && !lua_isboolean(state, 5)))
        {
            return luaL_error(
                    state,
                    "UEnum:InsertIntoNames requires name, value, index, and optional shift boolean");
        }
        std::size_t length{};
        const char* name = lua_tolstring(state, 2, &length);
        ue4ss::linux::core::RawFName unreal_name;
        std::int32_t inserted_index{-1};
        std::string error;
        if (!runtime->fname_from_utf8(
                    std::string_view{name, length}, unreal_name, true, error) ||
            !runtime->insert_enum_name(
                    unreal_enum->handle,
                    unreal_name,
                    static_cast<std::int64_t>(lua_tointeger(state, 3)),
                    static_cast<std::int32_t>(lua_tointeger(state, 4)),
                    lua_gettop(state) >= 5 && lua_toboolean(state, 5) != 0,
                    *scheduler,
                    inserted_index,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        lua_pushinteger(state, static_cast<lua_Integer>(inserted_index));
        return 1;
    }

    int lua_uenum_remove_from_names_at(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* scheduler = get_scheduler(state);
        auto* unreal_enum = get_object(state, 1);
        if (scheduler == nullptr)
        {
            return luaL_error(state, "UEnum mutation requires the validated game-thread scheduler");
        }
        if (runtime == nullptr || scheduler == nullptr || unreal_enum == nullptr ||
            !lua_isinteger(state, 2) || lua_tointeger(state, 2) < 0 ||
            (lua_gettop(state) >= 3 &&
             (!lua_isinteger(state, 3) || lua_tointeger(state, 3) < 0)) ||
            (lua_gettop(state) >= 4 && !lua_isboolean(state, 4)))
        {
            return luaL_error(
                    state,
                    "UEnum:RemoveFromNamesAt requires index, optional count, and optional shrink boolean");
        }
        const auto count = lua_gettop(state) >= 3
                                   ? static_cast<std::int32_t>(lua_tointeger(state, 3))
                                   : 1;
        const bool allow_shrinking =
                lua_gettop(state) < 4 || lua_toboolean(state, 4) != 0;
        std::string error;
        if (!runtime->remove_enum_names(
                    unreal_enum->handle,
                    static_cast<std::int32_t>(lua_tointeger(state, 2)),
                    count,
                    allow_shrinking,
                    *scheduler,
                    error))
        {
            return luaL_error(state, "%s", error.c_str());
        }
        return 0;
    }

    int lua_uobject_equal(lua_State* state) noexcept
    {
        const auto* left = get_object(state, 1);
        const auto* right = get_object(state, 2);
        lua_pushboolean(state,
                        left != nullptr && right != nullptr && left->handle.address == right->handle.address &&
                                left->handle.internal_index == right->handle.internal_index &&
                                left->handle.serial_number == right->handle.serial_number);
        return 1;
    }

    int lua_uobject_to_string(lua_State* state) noexcept
    {
        return push_description_field(state, 2);
    }

    int materialize_enum_global(lua_State* state,
                                const ue4ss::linux::core::UnrealPropertyValue& value) noexcept
    {
        if (value.enum_object.address == 0u || value.enum_property_name.empty()) return 0;
        auto* runtime = get_runtime(state);
        std::vector<ue4ss::linux::core::UnrealEnumEntry> entries;
        std::string error;
        if (runtime == nullptr || !runtime->enumerate_enum_names(value.enum_object, entries, error))
        {
            return luaL_error(state, "Enum_%s could not be materialized: %s",
                              value.enum_property_name.c_str(), error.c_str());
        }
        lua_createtable(state, 0, static_cast<int>(std::min<std::size_t>(
                                             entries.size(),
                                             static_cast<std::size_t>(std::numeric_limits<int>::max()))));
        for (const auto& entry : entries)
        {
            lua_pushlstring(state, entry.text.data(), entry.text.size());
            lua_pushinteger(state, static_cast<lua_Integer>(entry.value));
            lua_rawset(state, -3);
        }
        const std::string global_name = "Enum_" + value.enum_property_name;
        lua_setglobal(state, global_name.c_str());
        return 0;
    }

    int push_property_value(lua_State* state,
                            const ue4ss::linux::core::UnrealPropertyValue& value) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        switch (value.kind)
        {
        case UnrealPropertyKind::boolean:
            lua_pushboolean(state, value.boolean);
            return 1;
        case UnrealPropertyKind::signed_integer:
            materialize_enum_global(state, value);
            lua_pushinteger(state, static_cast<lua_Integer>(value.signed_integer));
            return 1;
        case UnrealPropertyKind::unsigned_integer:
            materialize_enum_global(state, value);
            if (value.unsigned_integer <= static_cast<std::uint64_t>(std::numeric_limits<lua_Integer>::max()))
            {
                lua_pushinteger(state, static_cast<lua_Integer>(value.unsigned_integer));
            }
            else
            {
                lua_pushnumber(state, static_cast<lua_Number>(value.unsigned_integer));
            }
            return 1;
        case UnrealPropertyKind::floating_point:
            lua_pushnumber(state, static_cast<lua_Number>(value.floating_point));
            return 1;
        case UnrealPropertyKind::name:
            return push_fname(state, value.name, value.text);
        case UnrealPropertyKind::string:
            return push_fstring(state, value.text);
        case UnrealPropertyKind::text:
            return push_ftext(state, value.text);
        case UnrealPropertyKind::object:
            if (value.object_is_null)
            {
                lua_pushnil(state);
                return 1;
            }
            return push_object(state, value.object);
        case UnrealPropertyKind::structure:
        {
            const int field_count = static_cast<int>(std::min<std::size_t>(
                    value.struct_fields.size(), static_cast<std::size_t>(std::numeric_limits<int>::max())));
            lua_createtable(state, 0, field_count);
            for (const auto& field : value.struct_fields)
            {
                if (field.value == nullptr)
                {
                    continue;
                }
                lua_pushlstring(state, field.name.data(), field.name.size());
                push_property_value(state, *field.value);
                lua_rawset(state, -3);
            }
            return 1;
        }
        case UnrealPropertyKind::array:
            return push_tarray(state, value.array_elements);
        case UnrealPropertyKind::set:
            return push_tset(state, value.set_elements);
        case UnrealPropertyKind::map:
            return push_tmap(state, value.map_entries);
        case UnrealPropertyKind::weak_object:
            return push_weak_object(state, value.object, value.object_is_null);
        case UnrealPropertyKind::soft_object:
            return push_soft_object(state, value);
        case UnrealPropertyKind::delegate:
            lua_createtable(state, 0, 2);
            lua_pushliteral(state, "Object");
            if (value.object_is_null) lua_pushnil(state);
            else push_object(state, value.object);
            lua_rawset(state, -3);
            lua_pushliteral(state, "FunctionName");
            push_fname(state, value.name, value.text);
            lua_rawset(state, -3);
            return 1;
        case UnrealPropertyKind::multicast_delegate:
            lua_createtable(state,
                            static_cast<int>(std::min<std::size_t>(
                                    value.array_elements.size(),
                                    static_cast<std::size_t>(std::numeric_limits<int>::max()))),
                            0);
            for (std::size_t index = 0; index < value.array_elements.size(); ++index)
            {
                if (value.array_elements[index] == nullptr) continue;
                push_property_value(state, *value.array_elements[index]);
                lua_rawseti(state, -2, static_cast<lua_Integer>(index + 1u));
            }
            return 1;
        }
        lua_pushnil(state);
        return 1;
    }

    int lua_reflected_function_call(lua_State* state) noexcept;

    int lua_uobject_index(lua_State* state) noexcept
    {
        if (get_object(state, 1) == nullptr || lua_type(state, 2) != LUA_TSTRING)
        {
            lua_pushliteral(state, "UObject member access requires a live object and string key");
            return lua_error(state);
        }

        // Resolve compatibility methods before treating the key as a
        // reflected FProperty name.
        luaL_getmetatable(state, k_uobject_metatable);
        lua_getfield(state, -1, "__methods");
        lua_pushvalue(state, 2);
        lua_rawget(state, -2);
        if (!lua_isnil(state, -1))
        {
            return 1;
        }
        lua_pop(state, 3);

        bool failed{};
        {
            auto* runtime = get_runtime(state);
            auto* object = get_object(state, 1);
            std::size_t length{};
            const char* name = lua_tolstring(state, 2, &length);
            ue4ss::linux::core::UnrealPropertyValue value;
            std::string error;
            const std::string_view property_name{name, length};
            const bool reflected =
                    runtime != nullptr && object != nullptr &&
                    runtime->read_property(
                            object->handle, property_name, value, error);
            bool custom_found{};
            bool custom{};
            std::string reflected_error = error;
            if (!reflected && runtime != nullptr && object != nullptr)
            {
                error.clear();
                custom = runtime->read_registered_custom_property(
                        object->handle,
                        property_name,
                        value,
                        custom_found,
                        error);
            }
            if (!reflected && !custom)
            {
                if (custom_found)
                {
                    if (error.empty())
                    {
                        error = "registered custom property read failed";
                    }
                    lua_pushlstring(state, error.data(), error.size());
                    failed = true;
                }
                else
                {
                    ue4ss::linux::core::ReadOnlyUObjectHandle function;
                    std::string function_error;
                    if (runtime != nullptr && object != nullptr &&
                        runtime->find_function(
                                object->handle,
                                property_name,
                                function,
                                function_error))
                    {
                        lua_pushvalue(state, 1);
                        push_object(state, function);
                        lua_pushcclosure(state, lua_reflected_function_call, 2);
                        return 1;
                    }
                    error = std::move(reflected_error);
                    if (error.empty())
                    {
                        error = "reflected property read failed";
                    }
                    lua_pushlstring(state, error.data(), error.size());
                    failed = true;
                }
            }
            else
            {
                if (value.kind == ue4ss::linux::core::UnrealPropertyKind::multicast_delegate)
                {
                    return push_live_multicast_delegate(
                            state,
                            object->handle,
                            std::string_view{name, length},
                            value.multicast_sparse);
                }
                if (reflected &&
                    value.kind == ue4ss::linux::core::UnrealPropertyKind::structure)
                {
                    ue4ss::linux::core::UnrealStructBinding binding;
                    if (!runtime->bind_struct_property(
                                object->handle, property_name, binding, error))
                    {
                        lua_pushlstring(state, error.data(), error.size());
                        failed = true;
                    }
                    else
                    {
                        return push_struct_binding(state, binding);
                    }
                }
                if (!failed)
                {
                    return push_property_value(state, value);
                }
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int lua_reflected_function_call(lua_State* state) noexcept
    {
        bool failed{};
        int return_count{};
        {
            auto* runtime = get_runtime(state);
            auto* scheduler = get_scheduler(state);
            auto* context = get_object(state, lua_upvalueindex(1));
            auto* function = get_object(state, lua_upvalueindex(2));
            std::vector<ue4ss::linux::core::UnrealFunctionArgument> arguments;
            std::optional<ue4ss::linux::core::UnrealPropertyValue> result;
            std::vector<ue4ss::linux::core::UnrealFunctionOutValue> out_values;
            std::string error;
            if (runtime == nullptr || scheduler == nullptr || context == nullptr || function == nullptr)
            {
                error = "reflected UFunction closure lost its validated runtime context";
            }
            else
            {
                int first_argument = 1;
                if (auto* explicit_self = get_object(state, 1); explicit_self != nullptr &&
                    explicit_self->handle.address == context->handle.address)
                {
                    first_argument = 2;
                }
                arguments.reserve(static_cast<std::size_t>(std::max(0, lua_gettop(state) - first_argument + 1)));
                for (int index = first_argument; index <= lua_gettop(state) && error.empty(); ++index)
                {
                    ue4ss::linux::core::UnrealFunctionArgument argument;
                    auto& value = argument.value;
                    switch (lua_type(state, index))
                    {
                    case LUA_TNIL:
                        value.kind = ue4ss::linux::core::UnrealPropertyKind::object;
                        value.object_is_null = true;
                        break;
                    case LUA_TBOOLEAN:
                        value.kind = ue4ss::linux::core::UnrealPropertyKind::boolean;
                        value.boolean = lua_toboolean(state, index) != 0;
                        break;
                    case LUA_TNUMBER:
                        if (lua_isinteger(state, index))
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::signed_integer;
                            value.signed_integer = static_cast<std::int64_t>(lua_tointeger(state, index));
                        }
                        else
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::floating_point;
                            value.floating_point = static_cast<double>(lua_tonumber(state, index));
                        }
                        break;
                    case LUA_TTABLE:
                        argument.out_placeholder = true;
                        if (!lua_to_untyped_property_value(state, index, value, error))
                        {
                            // Preserve the detailed conversion error.
                        }
                        break;
                    case LUA_TSTRING:
                    {
                        std::size_t length{};
                        const char* string = lua_tolstring(state, index, &length);
                        value.kind = ue4ss::linux::core::UnrealPropertyKind::string;
                        value.text.assign(string, length);
                        break;
                    }
                    case LUA_TUSERDATA:
                        if (auto* object = get_object(state, index); object != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::object;
                            value.object = object->handle;
                        }
                        else if (auto* name = get_fname(state, index); name != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::name;
                            value.name = name->value;
                            value.text = name->text;
                        }
                        else if (auto* string = get_fstring(state, index); string != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::string;
                            value.text = string->value;
                        }
                        else if (auto* text_value = get_ftext(state, index); text_value != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::text;
                            value.text = text_value->value;
                        }
                        else if (auto* array = get_tarray(state, index); array != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::array;
                            value.array_elements = array->elements;
                        }
                        else if (auto* set = get_tset(state, index); set != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::set;
                            value.set_elements = set->elements;
                        }
                        else if (auto* map = get_tmap(state, index); map != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::map;
                            value.map_entries = map->entries;
                        }
                        else if (auto* soft = get_soft_object(state, index); soft != nullptr)
                        {
                            value.kind = ue4ss::linux::core::UnrealPropertyKind::soft_object;
                            value.object = soft->weak_object;
                            value.object_is_null = soft->weak_is_null;
                            value.signed_integer = soft->tag_at_last_test;
                            value.name = soft->asset_path_name;
                            value.text = soft->asset_path_text;
                            value.soft_package_name = soft->package_name;
                            value.soft_asset_name = soft->asset_name;
                            value.soft_package_text = soft->package_text;
                            value.soft_asset_text = soft->asset_text;
                            value.soft_sub_path = soft->sub_path;
                        }
                        else
                        {
                            error = "unsupported userdata in reflected function argument";
                        }
                        break;
                    default:
                        error = "reflected function argument type is not supported";
                        break;
                    }
                    if (error.empty())
                    {
                        arguments.push_back(std::move(argument));
                    }
                }
                if (error.empty() && runtime->invoke_function(
                                             context->handle,
                                             function->handle,
                                             arguments,
                                             result,
                                             out_values,
                                             *scheduler,
                                             error))
                {
                    for (const auto& out : out_values)
                    {
                        const int table_index = first_argument + static_cast<int>(out.argument_index);
                        if (lua_type(state, table_index) != LUA_TTABLE)
                        {
                            error = "UFunction OutParm table disappeared during synchronous invocation";
                            break;
                        }
                        if (out.value.kind == ue4ss::linux::core::UnrealPropertyKind::structure)
                        {
                            for (const auto& field : out.value.struct_fields)
                            {
                                if (field.value == nullptr)
                                {
                                    continue;
                                }
                                lua_pushlstring(state, field.name.data(), field.name.size());
                                push_property_value(state, *field.value);
                                lua_rawset(state, table_index);
                            }
                        }
                        else
                        {
                            lua_pushlstring(state, out.name.data(), out.name.size());
                            push_property_value(state, out.value);
                            lua_rawset(state, table_index);
                        }
                    }
                    if (result.has_value())
                    {
                        return_count = push_property_value(state, *result);
                    }
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : return_count;
    }

    int lua_uobject_call_function(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        auto* object = get_object(state, 1);
        auto* function = get_object(state, 2);
        std::string error;
        if (runtime == nullptr || object == nullptr || function == nullptr)
        {
            return luaL_error(
                    state,
                    "UObject:CallFunction requires a live UObject and UFunction");
        }
        if (!runtime->is_a(function->handle, "Function", error))
        {
            return luaL_error(
                    state,
                    "%s",
                    error.empty() ? "UObject:CallFunction second argument is not a UFunction"
                                  : error.c_str());
        }

        const int original_top = lua_gettop(state);
        lua_pushvalue(state, 1);
        lua_pushvalue(state, 2);
        lua_pushcclosure(state, lua_reflected_function_call, 2);
        lua_pushvalue(state, 1);
        for (int index = 3; index <= original_top; ++index)
        {
            lua_pushvalue(state, index);
        }
        if (lua_pcall(state, original_top - 1, LUA_MULTRET, 0) != LUA_OK)
        {
            return lua_error(state);
        }
        return lua_gettop(state) - original_top;
    }

    int lua_uobject_newindex(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* runtime = get_runtime(state);
            auto* scheduler = get_scheduler(state);
            auto* object = get_object(state, 1);
            std::string error;
            if (runtime == nullptr || scheduler == nullptr || object == nullptr || lua_type(state, 2) != LUA_TSTRING)
            {
                error = "UObject property write requires the validated runtime, scheduler, object, and string key";
            }
            else
            {
                std::size_t length{};
                const char* name = lua_tolstring(state, 2, &length);
                const std::string_view property_name{name, length};
                ue4ss::linux::core::UnrealPropertyValue current;
                bool custom_property{};
                bool custom_found{};
                bool property_read = runtime->read_property(
                        object->handle, property_name, current, error);
                if (!property_read)
                {
                    std::string reflected_error = std::move(error);
                    error.clear();
                    property_read = runtime->read_registered_custom_property(
                            object->handle,
                            property_name,
                            current,
                            custom_found,
                            error);
                    custom_property = custom_found;
                    if (!custom_found)
                    {
                        error = std::move(reflected_error);
                    }
                    else if (!property_read && error.empty())
                    {
                        error = "registered custom property read failed";
                    }
                }
                if (property_read)
                {
                    ue4ss::linux::core::UnrealPropertyValue value;
                    value.kind = current.kind;
                    using ue4ss::linux::core::UnrealPropertyKind;
                    switch (current.kind)
                    {
                    case UnrealPropertyKind::boolean:
                        if (!lua_isboolean(state, 3))
                        {
                            error = "BoolProperty assignment requires a Lua boolean";
                        }
                        else
                        {
                            value.boolean = lua_toboolean(state, 3) != 0;
                        }
                        break;
                    case UnrealPropertyKind::signed_integer:
                        if (!lua_isinteger(state, 3))
                        {
                            error = "signed integer property assignment requires a Lua integer";
                        }
                        else
                        {
                            value.signed_integer = static_cast<std::int64_t>(lua_tointeger(state, 3));
                        }
                        break;
                    case UnrealPropertyKind::unsigned_integer:
                        if (!lua_isinteger(state, 3) || lua_tointeger(state, 3) < 0)
                        {
                            error = "unsigned integer property assignment requires a non-negative Lua integer";
                        }
                        else
                        {
                            value.unsigned_integer = static_cast<std::uint64_t>(lua_tointeger(state, 3));
                        }
                        break;
                    case UnrealPropertyKind::floating_point:
                        if (!lua_isnumber(state, 3))
                        {
                            error = "floating-point property assignment requires a Lua number";
                        }
                        else
                        {
                            value.floating_point = static_cast<double>(lua_tonumber(state, 3));
                        }
                        break;
                    case UnrealPropertyKind::object:
                        if (lua_isnil(state, 3))
                        {
                            value.object_is_null = true;
                        }
                        else if (auto* assigned = get_object(state, 3); assigned != nullptr)
                        {
                            value.object = assigned->handle;
                        }
                        else
                        {
                            error = "ObjectProperty assignment requires nil or a UObject";
                        }
                        break;
                    case UnrealPropertyKind::name:
                        if (auto* assigned = get_fname(state, 3); assigned != nullptr)
                        {
                            value.name = assigned->value;
                            value.text = assigned->text;
                        }
                        else
                        {
                            error = "NameProperty assignment requires FName userdata";
                        }
                        break;
                    case UnrealPropertyKind::string:
                        if (auto* assigned = get_fstring(state, 3); assigned != nullptr)
                        {
                            value.text = assigned->value;
                        }
                        else if (lua_type(state, 3) == LUA_TSTRING)
                        {
                            std::size_t value_length{};
                            const char* assigned = lua_tolstring(state, 3, &value_length);
                            value.text.assign(assigned, value_length);
                        }
                        else
                        {
                            error = "StrProperty assignment requires a string or FString";
                        }
                        break;
                    case UnrealPropertyKind::text:
                        if (auto* assigned = get_ftext(state, 3); assigned != nullptr)
                        {
                            value.text = assigned->value;
                        }
                        else
                        {
                            error = "TextProperty assignment requires FText userdata";
                        }
                        break;
                    case UnrealPropertyKind::structure:
                        if (!lua_to_untyped_property_value(state, 3, value, error))
                        {
                            // Preserve the detailed conversion error.
                        }
                        else if (value.kind != UnrealPropertyKind::structure)
                        {
                            error = "StructProperty assignment requires a Lua table";
                        }
                        break;
                    case UnrealPropertyKind::array:
                        if (auto* assigned = get_tarray(state, 3); assigned != nullptr)
                        {
                            value.array_elements = assigned->elements;
                        }
                        else if (lua_type(state, 3) == LUA_TTABLE &&
                                 lua_to_untyped_property_value(state, 3, value, error) &&
                                 (value.kind == UnrealPropertyKind::array ||
                                  (value.kind == UnrealPropertyKind::structure && value.struct_fields.empty())))
                        {
                            value.kind = UnrealPropertyKind::array;
                        }
                        else
                        {
                            error = "ArrayProperty assignment requires TArray userdata";
                        }
                        break;
                    case UnrealPropertyKind::set:
                        if (auto* assigned = get_tset(state, 3); assigned != nullptr)
                        {
                            value.set_elements = assigned->elements;
                        }
                        else
                        {
                            error = "SetProperty assignment requires TSet userdata";
                        }
                        break;
                    case UnrealPropertyKind::map:
                        if (auto* assigned = get_tmap(state, 3); assigned != nullptr)
                        {
                            value.map_entries = assigned->entries;
                        }
                        else
                        {
                            error = "MapProperty assignment requires TMap userdata";
                        }
                        break;
                    case UnrealPropertyKind::weak_object:
                        error = "WeakObjectProperty writes are not supported by upstream UE4SS";
                        break;
                    case UnrealPropertyKind::soft_object:
                        if (!lua_to_untyped_property_value(state, 3, value, error))
                        {
                            // Preserve the detailed conversion error.
                        }
                        else if (value.kind != UnrealPropertyKind::soft_object)
                        {
                            error = "SoftObjectProperty assignment requires TSoftObjectPtr userdata";
                        }
                        break;
                    case UnrealPropertyKind::delegate:
                        if (!lua_to_delegate_assignment(state, 3, *runtime, value, error))
                        {
                            // Preserve the detailed conversion error.
                        }
                        break;
                    case UnrealPropertyKind::multicast_delegate:
                        error = "multicast delegates must be changed through Add/Remove/Clear";
                        break;
                    }
                    bool written{};
                    if (error.empty())
                    {
                        if (custom_property)
                        {
                            bool write_found{};
                            written = runtime->write_registered_custom_property(
                                    object->handle,
                                    property_name,
                                    value,
                                    *scheduler,
                                    write_found,
                                    error);
                            if (!write_found && error.empty())
                            {
                                error = "registered custom property disappeared before write";
                            }
                        }
                        else
                        {
                            written = runtime->write_property(
                                    object->handle,
                                    property_name,
                                    value,
                                    *scheduler,
                                    error);
                        }
                    }
                    if (error.empty() && !written)
                    {
                        error = custom_property
                                        ? "custom property write failed without a diagnostic"
                                        : "reflected property write failed without a diagnostic";
                    }
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int lua_find_first_of(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            if (runtime == nullptr || lua_type(state, 1) != LUA_TSTRING)
            {
                return push_object(state, {});
            }
            std::size_t length{};
            const char* name = lua_tolstring(state, 1, &length);
            const auto result = runtime->find_all_of(std::string_view{name, length}, 1u);
            if (!result.success || result.objects.empty())
            {
                return push_object(state, {});
            }
            return push_object(state, result.objects.front());
        }
        catch (...)
        {
            return push_object(state, {});
        }
    }

    int lua_find_all_of(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            if (runtime == nullptr || lua_type(state, 1) != LUA_TSTRING)
            {
                lua_pushnil(state);
                return 1;
            }
            std::size_t length{};
            const char* name = lua_tolstring(state, 1, &length);
            const auto result = runtime->find_all_of(std::string_view{name, length});
            if (!result.success || result.objects.empty())
            {
                lua_pushnil(state);
                return 1;
            }
            lua_createtable(state, static_cast<int>(result.objects.size()), 0);
            int index = 1;
            for (const auto& object : result.objects)
            {
                push_object(state, object);
                lua_rawseti(state, -2, index++);
            }
            return 1;
        }
        catch (...)
        {
            lua_pushnil(state);
            return 1;
        }
    }

    int register_begin_play_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterBeginPlayPostHook requires a callback"
                         : "RegisterBeginPlayPreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_begin_play_hook(state, post);
    }

    int lua_register_begin_play_pre_hook(lua_State* state) noexcept
    {
        return register_begin_play_hook(state, false);
    }

    int lua_register_begin_play_post_hook(lua_State* state) noexcept
    {
        return register_begin_play_hook(state, true);
    }

    int register_end_play_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterEndPlayPostHook requires a callback"
                         : "RegisterEndPlayPreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_end_play_hook(state, post);
    }

    int lua_register_end_play_pre_hook(lua_State* state) noexcept
    {
        return register_end_play_hook(state, false);
    }

    int lua_register_end_play_post_hook(lua_State* state) noexcept
    {
        return register_end_play_hook(state, true);
    }

    int register_init_game_state_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterInitGameStatePostHook requires a callback"
                         : "RegisterInitGameStatePreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_init_game_state_hook(state, post);
    }

    int lua_register_init_game_state_pre_hook(lua_State* state) noexcept
    {
        return register_init_game_state_hook(state, false);
    }

    int lua_register_init_game_state_post_hook(lua_State* state) noexcept
    {
        return register_init_game_state_hook(state, true);
    }

    int register_load_map_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterLoadMapPostHook requires a callback"
                         : "RegisterLoadMapPreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_load_map_hook(state, post);
    }

    int lua_register_load_map_pre_hook(lua_State* state) noexcept
    {
        return register_load_map_hook(state, false);
    }

    int lua_register_load_map_post_hook(lua_State* state) noexcept
    {
        return register_load_map_hook(state, true);
    }

    int register_process_console_exec_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterProcessConsoleExecPostHook requires a callback"
                         : "RegisterProcessConsoleExecPreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_process_console_exec_hook(state, post);
    }

    int lua_register_process_console_exec_pre_hook(lua_State* state) noexcept
    {
        return register_process_console_exec_hook(state, false);
    }

    int lua_register_process_console_exec_post_hook(lua_State* state) noexcept
    {
        return register_process_console_exec_hook(state, true);
    }

    int register_call_function_by_name_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterCallFunctionByNameWithArgumentsPostHook requires a callback"
                         : "RegisterCallFunctionByNameWithArgumentsPreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_call_function_by_name_hook(
                state, post);
    }

    int lua_register_call_function_by_name_pre_hook(lua_State* state) noexcept
    {
        return register_call_function_by_name_hook(state, false);
    }

    int lua_register_call_function_by_name_post_hook(lua_State* state) noexcept
    {
        return register_call_function_by_name_hook(state, true);
    }

    int register_local_player_exec_hook(lua_State* state, bool post) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    post ? "RegisterULocalPlayerExecPostHook requires a callback"
                         : "RegisterULocalPlayerExecPreHook requires a callback");
        }
        return ue4ss::linux::core::register_lua_local_player_exec_hook(
                state, post);
    }

    int lua_register_local_player_exec_pre_hook(lua_State* state) noexcept
    {
        return register_local_player_exec_hook(state, false);
    }

    int lua_register_local_player_exec_post_hook(lua_State* state) noexcept
    {
        return register_local_player_exec_hook(state, true);
    }

    int register_console_command(lua_State* state, bool global) noexcept
    {
        if (lua_type(state, 1) != LUA_TSTRING || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(
                    state,
                    global ? "RegisterConsoleCommandGlobalHandler requires a command name and callback"
                           : "RegisterConsoleCommandHandler requires a command name and callback");
        }
        return ue4ss::linux::core::register_lua_console_command(state, global);
    }

    int lua_register_console_command_global_handler(lua_State* state) noexcept
    {
        return register_console_command(state, true);
    }

    int lua_register_console_command_handler(lua_State* state) noexcept
    {
        return register_console_command(state, false);
    }

    [[nodiscard]] bool lua_find_objects_name(lua_State* state,
                                             int index,
                                             bool allow_uclass,
                                             std::optional<std::string>& output,
                                             std::string& error) noexcept
    {
        output.reset();
        const int value_type = lua_type(state, index);
        if (value_type == LUA_TNONE || value_type == LUA_TNIL)
        {
            return true;
        }
        if (value_type == LUA_TSTRING)
        {
            std::size_t length{};
            const char* value = lua_tolstring(state, index, &length);
            output.emplace(value, length);
            return !output->empty();
        }
        if (value_type == LUA_TUSERDATA)
        {
            if (auto* fname = get_fname(state, index); fname != nullptr)
            {
                output = fname->text;
                return !output->empty();
            }
            if (allow_uclass)
            {
                auto* runtime = get_runtime(state);
                auto* unreal_class = get_object(state, index);
                std::string class_error;
                if (runtime != nullptr && unreal_class != nullptr &&
                    runtime->is_a(unreal_class->handle, "Class", class_error))
                {
                    ue4ss::linux::core::ReadOnlyUObjectDescription description;
                    if (runtime->describe_object(unreal_class->handle, description, class_error))
                    {
                        output = std::move(description.name);
                        return !output->empty();
                    }
                }
                error = class_error.empty() ? "FindObjects class userdata is not a live UClass" : class_error;
                return false;
            }
        }
        error = allow_uclass ? "FindObjects class filter requires string, FName, UClass, or nil"
                             : "FindObjects object-name filter requires string, FName, or nil";
        return false;
    }

    int lua_find_objects(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            if (runtime == nullptr)
            {
                return luaL_error(state, "FindObjects requires the validated UObject runtime");
            }

            std::size_t limit{};
            if (lua_type(state, 1) != LUA_TNONE && lua_type(state, 1) != LUA_TNIL)
            {
                if (!lua_isinteger(state, 1) || lua_tointeger(state, 1) < 0)
                {
                    return luaL_error(state, "FindObjects result limit must be a non-negative integer or nil");
                }
                limit = static_cast<std::size_t>(lua_tointeger(state, 1));
            }

            std::optional<std::string> class_name;
            std::optional<std::string> object_name;
            std::string error;
            if (!lua_find_objects_name(state, 2, true, class_name, error) ||
                !lua_find_objects_name(state, 3, false, object_name, error))
            {
                return luaL_error(state, "%s", error.c_str());
            }
            if (!class_name.has_value() && !object_name.has_value())
            {
                return luaL_error(state, "FindObjects class and object-name filters cannot both be nil");
            }

            const auto read_flags = [state](int index, std::uint32_t& output) noexcept {
                if (lua_type(state, index) == LUA_TNONE || lua_type(state, index) == LUA_TNIL)
                {
                    output = 0u;
                    return true;
                }
                if (!lua_isinteger(state, index))
                {
                    return false;
                }
                const lua_Integer value = lua_tointeger(state, index);
                if (value < 0 || static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max())
                {
                    return false;
                }
                output = static_cast<std::uint32_t>(value);
                return true;
            };
            std::uint32_t required_flags{};
            std::uint32_t banned_flags{};
            if (!read_flags(4, required_flags) || !read_flags(5, banned_flags))
            {
                return luaL_error(state, "FindObjects required/banned flags must be EObjectFlags integers or nil");
            }

            bool exact_class = true;
            if (lua_type(state, 6) != LUA_TNONE && lua_type(state, 6) != LUA_TNIL)
            {
                if (lua_isboolean(state, 6))
                {
                    exact_class = lua_toboolean(state, 6) != 0;
                }
                else if (lua_isinteger(state, 6))
                {
                    exact_class = lua_tointeger(state, 6) != 0;
                }
                else
                {
                    return luaL_error(state, "FindObjects exact-class argument must be boolean, integer, or nil");
                }
            }

            const std::optional<std::string_view> class_view = class_name.has_value()
                    ? std::optional<std::string_view>{*class_name}
                    : std::nullopt;
            const std::optional<std::string_view> object_view = object_name.has_value()
                    ? std::optional<std::string_view>{*object_name}
                    : std::nullopt;
            const auto result = runtime->find_objects(
                    limit, class_view, object_view, required_flags, banned_flags, exact_class);
            if (!result.success)
            {
                return luaL_error(state, "%s", result.detail.c_str());
            }
            lua_createtable(state,
                            static_cast<int>(std::min<std::size_t>(
                                    result.objects.size(),
                                    static_cast<std::size_t>(std::numeric_limits<int>::max()))),
                            0);
            for (std::size_t index = 0; index < result.objects.size(); ++index)
            {
                push_object(state, result.objects[index]);
                lua_rawseti(state, -2, static_cast<lua_Integer>(index + 1u));
            }
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "FindObjects failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "FindObjects failed with an unknown exception");
        }
    }

    [[nodiscard]] bool lua_read_object_flags(lua_State* state,
                                             int index,
                                             std::uint32_t& output) noexcept
    {
        if (lua_isnoneornil(state, index))
        {
            output = 0u;
            return true;
        }
        if (!lua_isinteger(state, index))
        {
            return false;
        }
        const lua_Integer value = lua_tointeger(state, index);
        if (value < 0 ||
            static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max())
        {
            return false;
        }
        output = static_cast<std::uint32_t>(value);
        return true;
    }

    [[nodiscard]] bool object_is_within_outer(
            ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& object,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& requested_outer) noexcept
    {
        ue4ss::linux::core::ReadOnlyUObjectHandle current = object;
        for (std::size_t depth = 0; depth < 256u; ++depth)
        {
            ue4ss::linux::core::ReadOnlyUObjectDescription description;
            std::string error;
            if (!runtime.describe_object(current, description, error) ||
                description.outer_address == 0u)
            {
                return false;
            }
            if (description.outer_address == requested_outer.address)
            {
                return true;
            }
            if (!runtime.handle_from_address(description.outer_address, current))
            {
                return false;
            }
        }
        return false;
    }

    [[nodiscard]] bool find_object_in_outer(
            ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const std::optional<ue4ss::linux::core::ReadOnlyUObjectHandle>& requested_class,
            const std::optional<ue4ss::linux::core::ReadOnlyUObjectHandle>& requested_outer,
            bool any_package,
            std::string_view name,
            bool exact_class,
            ue4ss::linux::core::ReadOnlyUObjectHandle& output,
            std::string& error) noexcept
    {
        output = {};
        error.clear();
        if (name.empty())
        {
            error = "FindObject name cannot be empty";
            return false;
        }

        const bool long_name = name.front() == '/' || name.find('.') != std::string_view::npos ||
                               name.find(':') != std::string_view::npos;
        ue4ss::linux::core::ObjectQueryResult candidates;
        if (long_name)
        {
            candidates = runtime.find_by_path(name, 1u);
        }
        else
        {
            std::optional<std::string> class_name;
            if (requested_class.has_value())
            {
                ue4ss::linux::core::ReadOnlyUObjectDescription description;
                if (!runtime.describe_object(*requested_class, description, error))
                {
                    return false;
                }
                class_name = std::move(description.name);
            }
            candidates = runtime.find_objects(
                    0u,
                    class_name.has_value()
                            ? std::optional<std::string_view>{*class_name}
                            : std::nullopt,
                    name,
                    0u,
                    0u,
                    exact_class);
        }
        if (!candidates.success)
        {
            error = candidates.detail;
            return false;
        }

        for (const auto& candidate : candidates.objects)
        {
            if (requested_class.has_value())
            {
                bool class_matches{};
                if (exact_class)
                {
                    ue4ss::linux::core::ReadOnlyUObjectDescription description;
                    class_matches = runtime.describe_object(candidate, description, error) &&
                                    description.class_address == requested_class->address;
                }
                else
                {
                    class_matches = runtime.is_a(candidate, *requested_class, error);
                }
                if (!class_matches)
                {
                    error.clear();
                    continue;
                }
            }

            if (any_package ||
                (requested_outer.has_value() &&
                 object_is_within_outer(runtime, candidate, *requested_outer)) ||
                (!requested_outer.has_value() && long_name))
            {
                output = candidate;
                return true;
            }
        }
        return true;
    }

    int lua_find_object(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            if (runtime == nullptr)
            {
                return luaL_error(state, "FindObject requires the validated UObject runtime");
            }

            const bool outer_overload = lua_type(state, 3) == LUA_TSTRING &&
                                        (lua_isnoneornil(state, 4) || lua_isboolean(state, 4)) &&
                                        (lua_isnoneornil(state, 1) || get_object(state, 1) != nullptr) &&
                                        (lua_isnoneornil(state, 2) || get_object(state, 2) != nullptr ||
                                         (lua_isinteger(state, 2) && lua_tointeger(state, 2) == -1));
            if (outer_overload)
            {
                std::optional<ue4ss::linux::core::ReadOnlyUObjectHandle> requested_class;
                std::optional<ue4ss::linux::core::ReadOnlyUObjectHandle> requested_outer;
                std::string error;
                if (!lua_isnoneornil(state, 1))
                {
                    auto* unreal_class = get_object(state, 1);
                    if (unreal_class == nullptr ||
                        !runtime->is_a(unreal_class->handle, "Class", error))
                    {
                        return luaL_error(state,
                                          "FindObject first argument must be a live UClass or nil: %s",
                                          error.c_str());
                    }
                    requested_class = unreal_class->handle;
                }
                bool any_package = lua_isinteger(state, 2) && lua_tointeger(state, 2) == -1;
                if (!lua_isnoneornil(state, 2) && !any_package)
                {
                    auto* outer = get_object(state, 2);
                    if (outer == nullptr || !runtime->is_valid(outer->handle))
                    {
                        return luaL_error(state,
                                          "FindObject second argument must be a live UObject, -1, or nil");
                    }
                    requested_outer = outer->handle;
                }
                std::size_t length{};
                const char* data = lua_tolstring(state, 3, &length);
                ue4ss::linux::core::ReadOnlyUObjectHandle found;
                if (!find_object_in_outer(*runtime,
                                          requested_class,
                                          requested_outer,
                                          any_package,
                                          std::string_view{data, length},
                                          lua_toboolean(state, 4) != 0,
                                          found,
                                          error))
                {
                    return luaL_error(state, "FindObject failed: %s", error.c_str());
                }
                return push_object(state, found);
            }

            std::optional<std::string> class_name;
            std::optional<std::string> object_name;
            std::string error;
            if (!lua_find_objects_name(state, 1, true, class_name, error) ||
                !lua_find_objects_name(state, 2, false, object_name, error))
            {
                return luaL_error(state, "FindObject: %s", error.c_str());
            }
            if (!class_name.has_value() && !object_name.has_value())
            {
                return luaL_error(state, "FindObject class and object name cannot both be nil");
            }
            std::uint32_t required_flags{};
            std::uint32_t banned_flags{};
            if (!lua_read_object_flags(state, 3, required_flags) ||
                !lua_read_object_flags(state, 4, banned_flags))
            {
                return luaL_error(state, "FindObject flags must be EObjectFlags integers or nil");
            }
            const auto result = runtime->find_objects(
                    1u,
                    class_name.has_value() ? std::optional<std::string_view>{*class_name}
                                           : std::nullopt,
                    object_name.has_value() ? std::optional<std::string_view>{*object_name}
                                            : std::nullopt,
                    required_flags,
                    banned_flags,
                    false);
            if (!result.success)
            {
                return luaL_error(state, "FindObject failed: %s", result.detail.c_str());
            }
            return push_object(state, result.objects.empty()
                                              ? ue4ss::linux::core::ReadOnlyUObjectHandle{}
                                              : result.objects.front());
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "FindObject failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "FindObject failed with an unknown exception");
        }
    }

    int lua_for_each_uobject(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        if (runtime == nullptr || lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(state, "ForEachUObject requires a Lua callback");
        }
        const int callback_index = lua_absindex(state, 1);
        std::string callback_error;
        std::string runtime_error;
        std::int32_t visited{};
        const bool iterated = runtime->for_each_uobject(
                [state, callback_index, &callback_error](
                        const ue4ss::linux::core::ReadOnlyUObjectHandle& object,
                        std::int32_t chunk_index,
                        std::int32_t object_index) {
                    lua_pushvalue(state, callback_index);
                    push_object(state, object);
                    lua_pushinteger(state, static_cast<lua_Integer>(chunk_index));
                    lua_pushinteger(state, static_cast<lua_Integer>(object_index));
                    if (lua_pcall(state, 3, 1, 0) != LUA_OK)
                    {
                        const char* message = lua_tostring(state, -1);
                        callback_error = message != nullptr ? message : "unknown Lua callback error";
                        lua_pop(state, 1);
                        return false;
                    }
                    lua_pop(state, 1);
                    return true;
                },
                visited,
                runtime_error);
        if (!callback_error.empty())
        {
            return luaL_error(state, "ForEachUObject callback failed: %s", callback_error.c_str());
        }
        if (!iterated)
        {
            return luaL_error(state, "ForEachUObject failed: %s", runtime_error.c_str());
        }
        return 0;
    }

    int lua_static_find_object(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            if (runtime == nullptr)
            {
                return luaL_error(state, "StaticFindObject requires the validated UObject runtime");
            }
            if (lua_type(state, 1) == LUA_TSTRING)
            {
                std::size_t length{};
                const char* path = lua_tolstring(state, 1, &length);
                const auto result = runtime->find_by_path(std::string_view{path, length}, 1u);
                if (!result.success)
                {
                    return luaL_error(state, "StaticFindObject failed: %s", result.detail.c_str());
                }
                return push_object(state, result.objects.empty()
                                                  ? ue4ss::linux::core::ReadOnlyUObjectHandle{}
                                                  : result.objects.front());
            }

            if ((!lua_isnoneornil(state, 1) && get_object(state, 1) == nullptr) ||
                (!lua_isnoneornil(state, 2) && get_object(state, 2) == nullptr) ||
                lua_type(state, 3) != LUA_TSTRING)
            {
                return luaL_error(
                        state,
                        "StaticFindObject expects a path string or (UClass|nil, UObject|nil, name, exactClass?)");
            }
            std::optional<ue4ss::linux::core::ReadOnlyUObjectHandle> requested_class;
            std::optional<ue4ss::linux::core::ReadOnlyUObjectHandle> requested_outer;
            std::string error;
            if (!lua_isnoneornil(state, 1))
            {
                auto* unreal_class = get_object(state, 1);
                if (!runtime->is_a(unreal_class->handle, "Class", error))
                {
                    return luaL_error(state, "StaticFindObject first argument is not a live UClass: %s", error.c_str());
                }
                requested_class = unreal_class->handle;
            }
            if (!lua_isnoneornil(state, 2))
            {
                auto* outer = get_object(state, 2);
                if (!runtime->is_valid(outer->handle))
                {
                    return luaL_error(state, "StaticFindObject second argument is not a live UObject");
                }
                requested_outer = outer->handle;
            }
            std::size_t length{};
            const char* name = lua_tolstring(state, 3, &length);
            ue4ss::linux::core::ReadOnlyUObjectHandle found;
            if (!find_object_in_outer(*runtime,
                                      requested_class,
                                      requested_outer,
                                      !requested_outer.has_value(),
                                      std::string_view{name, length},
                                      lua_toboolean(state, 4) != 0,
                                      found,
                                      error))
            {
                return luaL_error(state, "StaticFindObject failed: %s", error.c_str());
            }
            return push_object(state, found);
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "StaticFindObject failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "StaticFindObject failed with an unknown exception");
        }
    }

    int lua_static_construct_object(lua_State* state) noexcept
    {
        bool failed{};
        ue4ss::linux::core::ReadOnlyUObjectHandle constructed;
        {
            auto* runtime = get_runtime(state);
            auto* scheduler = get_scheduler(state);
            ue4ss::linux::core::UnrealObjectConstructionRequest request;
            std::string error;

            if (runtime == nullptr)
            {
                error = "StaticConstructObject requires the validated UObject runtime";
            }
            else if (scheduler == nullptr || !scheduler->is_ready())
            {
                error = "StaticConstructObject requires the validated game-thread scheduler";
            }
            else if (auto* unreal_class = get_object(state, 1); unreal_class == nullptr)
            {
                error = "StaticConstructObject first argument must be a live UClass";
            }
            else
            {
                request.unreal_class = unreal_class->handle;
            }

            if (error.empty())
            {
                if (auto* outer = get_object(state, 2); outer == nullptr)
                {
                    error = "StaticConstructObject second argument must be a live outer UObject";
                }
                else
                {
                    request.outer = outer->handle;
                }
            }

            if (error.empty() && !lua_isnoneornil(state, 3))
            {
                if (auto* name = get_fname(state, 3); name != nullptr)
                {
                    request.name = name->value;
                }
                else if (lua_isinteger(state, 3))
                {
                    const auto packed_name = static_cast<std::uint64_t>(lua_tointeger(state, 3));
                    request.name = {
                            .comparison_index = static_cast<std::uint32_t>(packed_name),
                            .number = static_cast<std::uint32_t>(packed_name >> 32u),
                    };
                }
                else
                {
                    error = "StaticConstructObject name must be FName, packed int64, or nil";
                }
            }

            if (error.empty() && !lua_read_object_flags(state, 4, request.object_flags))
            {
                error = "StaticConstructObject flags must be an EObjectFlags uint32 integer or nil";
            }
            if (error.empty() &&
                !lua_read_object_flags(state, 5, request.internal_object_flags))
            {
                error = "StaticConstructObject internal flags must be an EInternalObjectFlags uint32 integer or nil";
            }

            const auto read_optional_bool = [state, &error](int index,
                                                            const char* name,
                                                            bool& output) noexcept {
                if (lua_isnoneornil(state, index))
                {
                    output = false;
                    return;
                }
                if (!lua_isboolean(state, index))
                {
                    error = std::string{"StaticConstructObject "} + name +
                            " must be boolean or nil";
                    return;
                }
                output = lua_toboolean(state, index) != 0;
            };
            if (error.empty())
            {
                read_optional_bool(6,
                                   "CopyTransientsFromClassDefaults",
                                   request.copy_transients_from_class_defaults);
            }
            if (error.empty())
            {
                read_optional_bool(7,
                                   "AssumeTemplateIsArchetype",
                                   request.assume_template_is_archetype);
            }

            if (error.empty() && !lua_isnoneornil(state, 8))
            {
                if (auto* object_template = get_object(state, 8); object_template != nullptr)
                {
                    request.object_template = object_template->handle;
                }
                else
                {
                    error = "StaticConstructObject template must be UObject userdata or nil";
                }
            }

            if (error.empty() && !lua_isnoneornil(state, 9) &&
                (!lua_isinteger(state, 9) || lua_tointeger(state, 9) != 0))
            {
                error = "StaticConstructObject non-null FObjectInstancingGraph is not available in headless Linux mode";
            }

            if (error.empty() && !lua_isnoneornil(state, 10))
            {
                if (auto* external_package = get_object(state, 10); external_package != nullptr)
                {
                    request.external_package = external_package->handle;
                }
                else if (!lua_isinteger(state, 10) || lua_tointeger(state, 10) != 0)
                {
                    error = "StaticConstructObject external package must be UPackage userdata, zero, or nil";
                }
            }

            if (error.empty() && !lua_isnoneornil(state, 11) &&
                (!lua_isinteger(state, 11) || lua_tointeger(state, 11) != 0))
            {
                error = "StaticConstructObject non-null SubobjectOverrides is not available in headless Linux mode";
            }

            if (error.empty() &&
                !runtime->construct_object(request, *scheduler, constructed, error) &&
                error.empty())
            {
                error = "StaticConstructObject failed without a diagnostic";
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : push_object(state, constructed);
    }

    [[nodiscard]] std::optional<std::filesystem::path> lua_game_executable_directory(
            lua_State* state) noexcept
    {
        try
        {
            lua_getfield(
                    state, LUA_REGISTRYINDEX, k_game_executable_directory_registry_key);
            std::optional<std::filesystem::path> result;
            if (lua_type(state, -1) == LUA_TSTRING)
            {
                std::size_t length{};
                const char* value = lua_tolstring(state, -1, &length);
                if (value != nullptr && length > 0u)
                {
                    result.emplace(std::string{value, length});
                }
            }
            lua_pop(state, 1);
            return result;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<std::filesystem::path> lua_ue4ss_root_directory(
            lua_State* state) noexcept
    {
        try
        {
            lua_getfield(
                    state, LUA_REGISTRYINDEX, k_ue4ss_root_directory_registry_key);
            std::optional<std::filesystem::path> result;
            if (lua_type(state, -1) == LUA_TSTRING)
            {
                std::size_t length{};
                const char* value = lua_tolstring(state, -1, &length);
                if (value != nullptr && length > 0u)
                {
                    result.emplace(std::string{value, length});
                }
            }
            lua_pop(state, 1);
            return result;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    [[nodiscard]] bool collect_plain_directory_entries(
            const std::filesystem::path& directory,
            bool directories,
            std::vector<std::filesystem::path>& output) noexcept
    {
        output.clear();
        try
        {
            std::error_code error;
            std::filesystem::directory_iterator iterator{
                    directory,
                    std::filesystem::directory_options::skip_permission_denied,
                    error};
            const std::filesystem::directory_iterator end;
            while (!error && iterator != end)
            {
                const std::filesystem::path path = iterator->path();
                std::error_code status_error;
                const auto status = std::filesystem::symlink_status(path, status_error);
                if (!status_error && !std::filesystem::is_symlink(status) &&
                    (directories ? std::filesystem::is_directory(status)
                                 : std::filesystem::is_regular_file(status)))
                {
                    output.push_back(path);
                }
                iterator.increment(error);
            }
            if (error)
            {
                output.clear();
                return false;
            }
            std::sort(output.begin(), output.end(), [](const auto& left, const auto& right) {
                return left.filename().native() < right.filename().native();
            });
            return true;
        }
        catch (...)
        {
            output.clear();
            return false;
        }
    }

    int push_directory_files(lua_State* state,
                             const std::filesystem::path& directory) noexcept
    {
        const int original_top = lua_gettop(state);
        try
        {
            std::vector<std::filesystem::path> files;
            if (!collect_plain_directory_entries(directory, false, files))
            {
                lua_pushnil(state);
                return 1;
            }
            lua_createtable(
                    state,
                    static_cast<int>(std::min<std::size_t>(
                            files.size(),
                            static_cast<std::size_t>(std::numeric_limits<int>::max()))),
                    0);
            const int files_table = lua_absindex(state, -1);
            lua_Integer index = 1;
            for (const auto& file : files)
            {
                lua_createtable(state, 0, 2);
                const std::string name = file.filename().string();
                const std::string absolute_path = file.string();
                lua_pushlstring(state, name.data(), name.size());
                lua_setfield(state, -2, "__name");
                lua_pushlstring(
                        state, absolute_path.data(), absolute_path.size());
                lua_setfield(state, -2, "__absolute_path");
                lua_rawseti(state, files_table, index++);
            }
            return 1;
        }
        catch (...)
        {
            lua_settop(state, original_top);
            lua_pushnil(state);
            return 1;
        }
    }

    int lua_directory_index(lua_State* state) noexcept
    {
        try
        {
            std::size_t key_length{};
            const char* key = lua_tolstring(state, 2, &key_length);
            if (key == nullptr)
            {
                lua_pushnil(state);
                return 1;
            }
            const std::string_view requested{key, key_length};
            if (!lua_getmetatable(state, 1))
            {
                lua_pushnil(state);
                return 1;
            }
            if (requested == "__name" || requested == "__absolute_path")
            {
                lua_pushvalue(state, 2);
                lua_rawget(state, -2);
                lua_remove(state, -2);
                return 1;
            }
            if (requested == "__files")
            {
                lua_getfield(state, -1, "__absolute_path");
                std::size_t path_length{};
                const char* path_value = lua_tolstring(state, -1, &path_length);
                if (path_value == nullptr)
                {
                    lua_pop(state, 2);
                    lua_pushnil(state);
                    return 1;
                }
                const std::filesystem::path path{
                        std::string{path_value, path_length}};
                lua_pop(state, 2);
                return push_directory_files(state, path);
            }
            lua_pop(state, 1);
            lua_pushnil(state);
            return 1;
        }
        catch (...)
        {
            lua_settop(state, 2);
            lua_pushnil(state);
            return 1;
        }
    }

    [[nodiscard]] bool populate_directory_table(
            lua_State* state,
            const std::filesystem::path& directory,
            std::string_view game_directory_name,
            std::size_t depth = 0u) noexcept
    {
        if (depth > 128u || lua_type(state, -1) != LUA_TTABLE)
        {
            return false;
        }
        try
        {
            const int directory_table = lua_absindex(state, -1);
            std::vector<std::filesystem::path> directories;
            if (!collect_plain_directory_entries(directory, true, directories))
            {
                return false;
            }
            for (const auto& child : directories)
            {
                const std::string filename = child.filename().string();
                const std::string key = filename == game_directory_name
                        ? "Game"
                        : filename;
                lua_pushlstring(state, key.data(), key.size());
                lua_newtable(state);
                if (!populate_directory_table(
                            state, child, game_directory_name, depth + 1u))
                {
                    lua_pop(state, 2);
                    return false;
                }
                lua_rawset(state, directory_table);
            }

            lua_createtable(state, 0, 3);
            const std::string name = directory.filename().string();
            const std::string absolute_path = directory.string();
            lua_pushlstring(state, name.data(), name.size());
            lua_setfield(state, -2, "__name");
            lua_pushlstring(state, absolute_path.data(), absolute_path.size());
            lua_setfield(state, -2, "__absolute_path");
            lua_pushcfunction(state, lua_directory_index);
            lua_setfield(state, -2, "__index");
            lua_setmetatable(state, directory_table);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    int lua_iterate_game_directories(lua_State* state) noexcept
    {
        const auto executable_directory = lua_game_executable_directory(state);
        if (!executable_directory.has_value())
        {
            lua_pushnil(state);
            return 1;
        }
        try
        {
            const std::filesystem::path game_directory =
                    executable_directory->parent_path().parent_path();
            const std::filesystem::path content_directory = game_directory / "Content";
            std::error_code error;
            const auto content_status =
                    std::filesystem::symlink_status(content_directory, error);
            if (error || !std::filesystem::is_directory(content_status) ||
                std::filesystem::is_symlink(content_status))
            {
                lua_pushnil(state);
                return 1;
            }
            const std::filesystem::path root_directory =
                    game_directory.parent_path();
            lua_newtable(state);
            if (!populate_directory_table(
                        state,
                        root_directory,
                        game_directory.filename().string()))
            {
                lua_pop(state, 1);
                lua_pushnil(state);
            }
            return 1;
        }
        catch (...)
        {
            lua_pushnil(state);
            return 1;
        }
    }

    int lua_create_logic_mods_directory(lua_State* state) noexcept
    {
        const auto executable_directory = lua_game_executable_directory(state);
        if (!executable_directory.has_value())
        {
            return luaL_error(
                    state,
                    "CreateLogicModsDirectory requires a validated game executable directory");
        }
        try
        {
            const std::filesystem::path content_directory =
                    executable_directory->parent_path().parent_path() / "Content";
            std::error_code error;
            const auto content_status =
                    std::filesystem::symlink_status(content_directory, error);
            if (error || !std::filesystem::is_directory(content_status) ||
                std::filesystem::is_symlink(content_status))
            {
                return luaL_error(
                        state,
                        "CreateLogicModsDirectory could not validate the game Content directory");
            }
            const std::filesystem::path logic_mods =
                    content_directory / "Paks" / "LogicMods";
            const bool created = std::filesystem::create_directories(logic_mods, error);
            if (error)
            {
                return luaL_error(
                        state,
                        "CreateLogicModsDirectory failed: %s",
                        error.message().c_str());
            }
            lua_pushboolean(
                    state, created || std::filesystem::is_directory(logic_mods, error));
            return 1;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(
                    state, "CreateLogicModsDirectory failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(
                    state, "CreateLogicModsDirectory failed with an unknown exception");
        }
    }

    [[nodiscard]] bool make_soft_object_argument(
            ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            std::string_view input,
            ue4ss::linux::core::UnrealPropertyValue& output,
            std::string& error) noexcept
    {
        error.clear();
        output = {};
        if (input.empty())
        {
            error = "asset path cannot be empty";
            return false;
        }

        const std::size_t sub_path_separator = input.find(':');
        const std::string_view top_level_path = input.substr(0, sub_path_separator);
        const std::string_view sub_path = sub_path_separator == std::string_view::npos
                ? std::string_view{}
                : input.substr(sub_path_separator + 1u);
        std::string_view package_path;
        std::string_view asset_name;
        const std::size_t asset_separator = top_level_path.rfind('.');
        if (asset_separator != std::string_view::npos)
        {
            package_path = top_level_path.substr(0, asset_separator);
            asset_name = top_level_path.substr(asset_separator + 1u);
        }
        else
        {
            package_path = top_level_path;
            const std::size_t final_slash = top_level_path.rfind('/');
            asset_name = final_slash == std::string_view::npos
                    ? std::string_view{}
                    : top_level_path.substr(final_slash + 1u);
        }
        if (package_path.empty() || package_path.front() != '/' || asset_name.empty())
        {
            error = "asset path must use '/Package/Path.AssetName' syntax";
            return false;
        }

        output.kind = ue4ss::linux::core::UnrealPropertyKind::soft_object;
        output.object_is_null = true;
        output.signed_integer = 0;
        output.text.assign(top_level_path);
        output.soft_package_text.assign(package_path);
        output.soft_asset_text.assign(asset_name);
        output.soft_sub_path.assign(sub_path);
        if (!runtime.fname_from_utf8(
                    output.soft_package_text, output.soft_package_name, true, error) ||
            !runtime.fname_from_utf8(
                    output.soft_asset_text, output.soft_asset_name, true, error) ||
            !runtime.fname_from_utf8(output.text, output.name, true, error))
        {
            return false;
        }
        return true;
    }

    int lua_load_asset(lua_State* state) noexcept
    {
        try
        {
            auto* runtime = get_runtime(state);
            auto* scheduler = get_scheduler(state);
            if (runtime == nullptr || scheduler == nullptr || !scheduler->is_ready() ||
                !scheduler->is_game_thread())
            {
                return luaL_error(state, "Function 'LoadAsset' can only be called from within the game thread");
            }
            if (lua_type(state, 1) != LUA_TSTRING)
            {
                return luaL_error(state, "LoadAsset requires an Unreal asset path string");
            }

            std::size_t path_length{};
            const char* path_data = lua_tolstring(state, 1, &path_length);
            const std::string_view path{path_data, path_length};
            const auto already_loaded = runtime->find_by_path(path, 1u);
            if (already_loaded.success && !already_loaded.objects.empty())
            {
                push_object(state, already_loaded.objects.front());
                lua_pushboolean(state, 1);
                lua_pushboolean(state, 1);
                return 3;
            }

            std::string error;
            ue4ss::linux::core::UnrealPropertyValue soft_object;
            if (!make_soft_object_argument(*runtime, path, soft_object, error))
            {
                return luaL_error(state, "LoadAsset failed: %s", error.c_str());
            }
            const auto libraries = runtime->find_by_path(
                    "/Script/Engine.Default__KismetSystemLibrary", 1u);
            if (!libraries.success || libraries.objects.empty())
            {
                return luaL_error(state, "LoadAsset failed: KismetSystemLibrary default object was not found");
            }
            ue4ss::linux::core::ReadOnlyUObjectHandle function;
            if (!runtime->find_function(
                        libraries.objects.front(), "LoadAsset_Blocking", function, error))
            {
                return luaL_error(state, "LoadAsset failed: %s", error.c_str());
            }

            std::vector<ue4ss::linux::core::UnrealFunctionArgument> arguments;
            arguments.push_back({.value = std::move(soft_object)});
            std::optional<ue4ss::linux::core::UnrealPropertyValue> result;
            std::vector<ue4ss::linux::core::UnrealFunctionOutValue> out_values;
            if (!runtime->invoke_function(
                        libraries.objects.front(),
                        function,
                        arguments,
                        result,
                        out_values,
                        *scheduler,
                        error))
            {
                return luaL_error(state, "LoadAsset failed: %s", error.c_str());
            }
            if (!result.has_value() ||
                result->kind != ue4ss::linux::core::UnrealPropertyKind::object ||
                result->object_is_null || !runtime->is_valid(result->object))
            {
                lua_pushnil(state);
                lua_pushboolean(state, 0);
                lua_pushboolean(state, 0);
                return 3;
            }
            push_object(state, result->object);
            lua_pushboolean(state, 1);
            lua_pushboolean(state, 1);
            return 3;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "LoadAsset failed: %s", exception.what());
        }
        catch (...)
        {
            return luaL_error(state, "LoadAsset failed with an unknown exception");
        }
    }

    int lua_load_export(lua_State* state) noexcept
    {
        // Preserve the public UE4SS contract: invalid arguments return nil,
        // while a missing export is represented by the integer address 0.
        if (lua_gettop(state) != 1 || lua_type(state, 1) != LUA_TSTRING)
        {
            lua_pushnil(state);
            return 1;
        }

        std::size_t symbol_name_length{};
        const char* symbol_name = lua_tolstring(state, 1, &symbol_name_length);
        if (symbol_name == nullptr || symbol_name_length == 0u ||
            std::memchr(symbol_name, '\0', symbol_name_length) != nullptr)
        {
            lua_pushinteger(state, 0);
            return 1;
        }

        // RTLD_DEFAULT is the ELF equivalent of resolving a process export:
        // it searches the main executable and all globally loaded DSOs.  Do
        // not dlopen arbitrary paths here; LoadExport takes a symbol, not a
        // module name, and must not mutate the process loader state.
        (void)dlerror();
        void* address = dlsym(RTLD_DEFAULT, symbol_name);
        (void)dlerror();
        lua_pushinteger(
                state,
                static_cast<lua_Integer>(reinterpret_cast<std::uintptr_t>(address)));
        return 1;
    }

    int lua_headless_feature_unavailable(lua_State* state) noexcept
    {
        const char* function_name = lua_tostring(state, lua_upvalueindex(1));
        const char* reason = lua_tostring(state, lua_upvalueindex(2));
        return luaL_error(
                state,
                "%s is unavailable in headless Linux mode because %s",
                function_name != nullptr ? function_name : "this UE4SS API",
                reason != nullptr ? reason : "the required feature is disabled");
    }

    void register_headless_unavailable_global(
            lua_State* state,
            const char* function_name,
            const char* reason) noexcept
    {
        lua_pushstring(state, function_name);
        lua_pushstring(state, reason);
        lua_pushcclosure(state, lua_headless_feature_unavailable, 2);
        lua_setglobal(state, function_name);
    }

    int lua_dump_all_objects(lua_State* state) noexcept
    {
        std::filesystem::path temporary_path;
        try
        {
            auto* runtime = get_runtime(state);
            const auto root = lua_ue4ss_root_directory(state);
            if (runtime == nullptr || root == std::nullopt)
            {
                return luaL_error(
                        state,
                        "DumpAllObjects requires the validated UObject runtime and UE4SS root directory");
            }

            const std::filesystem::path output_directory = *root / "Logs";
            std::error_code filesystem_error;
            if (!std::filesystem::exists(output_directory, filesystem_error))
            {
                filesystem_error.clear();
                static_cast<void>(std::filesystem::create_directories(
                        output_directory, filesystem_error));
            }
            const auto directory_status =
                    std::filesystem::symlink_status(output_directory, filesystem_error);
            if (filesystem_error || !std::filesystem::is_directory(directory_status) ||
                std::filesystem::is_symlink(directory_status))
            {
                return luaL_error(
                        state,
                        "DumpAllObjects requires Logs to be a real writable directory: %s",
                        filesystem_error ? filesystem_error.message().c_str()
                                         : output_directory.string().c_str());
            }

            const std::filesystem::path output_path =
                    output_directory / "UE4SS_ObjectDump.txt";
            temporary_path = output_directory /
                    (".UE4SS_ObjectDump.txt.tmp." + std::to_string(getpid()) + "." +
                     std::to_string(std::chrono::steady_clock::now()
                                            .time_since_epoch()
                                            .count()));
            std::ofstream output{temporary_path, std::ios::out | std::ios::trunc};
            if (!output)
            {
                return luaL_error(
                        state,
                        "DumpAllObjects could not create %s",
                        temporary_path.string().c_str());
            }
            output << "# UE4SS Linux UObject and reflected property dump\n";
            output << "# Format: [chunk:slot] address flags full-name\n\n";

            std::int32_t visited{};
            std::int32_t written{};
            std::string iteration_error;
            bool write_failed{};
            const bool iterated = runtime->for_each_uobject(
                    [&](const ue4ss::linux::core::ReadOnlyUObjectHandle& object,
                        std::int32_t chunk_index,
                        std::int32_t object_index) {
                        ue4ss::linux::core::ReadOnlyUObjectDescription description;
                        std::string description_error;
                        if (!runtime->describe_object(
                                    object, description, description_error))
                        {
                            output << '[' << chunk_index << ':' << object_index
                                   << "] <stale: " << description_error << ">\n";
                            write_failed = !output.good();
                            return !write_failed;
                        }
                        output << '[' << chunk_index << ':' << object_index << "] 0x"
                               << std::hex << object.address << " flags=0x"
                               << description.object_flags << std::dec << ' '
                               << description.full_name << '\n';
                        ++written;

                        std::string struct_error;
                        const bool is_struct =
                                runtime->is_a(object, "Struct", struct_error);
                        struct_error.clear();
                        const bool is_class =
                                runtime->is_a(object, "Class", struct_error);
                        if (is_struct || is_class)
                        {
                            std::vector<ue4ss::linux::core::ReflectedPropertyDescription>
                                    properties;
                            std::string property_error;
                            if (runtime->enumerate_properties(
                                        object,
                                        properties,
                                        property_error,
                                        false))
                            {
                                for (const auto& property : properties)
                                {
                                    output << "  +0x" << std::hex << property.offset
                                           << std::dec << ' ' << property.type_name << ' '
                                           << property.name << " size="
                                           << property.element_size << " property=0x"
                                           << std::hex << property.address << std::dec
                                           << '\n';
                                }
                            }
                            else
                            {
                                output << "  <properties unavailable: "
                                       << property_error << ">\n";
                            }
                        }
                        write_failed = !output.good();
                        return !write_failed;
                    },
                    visited,
                    iteration_error);
            output << "\n# visited=" << visited << " written=" << written << '\n';
            output.flush();
            if (!iterated || write_failed || !output.good())
            {
                output.close();
                std::filesystem::remove(temporary_path, filesystem_error);
                temporary_path.clear();
                return luaL_error(
                        state,
                        "DumpAllObjects failed: %s",
                        iteration_error.empty() ? "output stream write failed"
                                                : iteration_error.c_str());
            }
            output.close();
            filesystem_error.clear();
            std::filesystem::rename(
                    temporary_path, output_path, filesystem_error);
            if (filesystem_error)
            {
                std::filesystem::remove(temporary_path, filesystem_error);
                temporary_path.clear();
                return luaL_error(
                        state,
                        "DumpAllObjects could not publish %s",
                        output_path.string().c_str());
            }
            temporary_path.clear();
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] DumpAllObjects wrote %d live objects to %s\n",
                    written,
                    output_path.c_str());
            return 0;
        }
        catch (const std::exception& exception)
        {
            if (!temporary_path.empty())
            {
                std::error_code ignored;
                std::filesystem::remove(temporary_path, ignored);
            }
            return luaL_error(
                    state, "DumpAllObjects failed: %s", exception.what());
        }
        catch (...)
        {
            if (!temporary_path.empty())
            {
                std::error_code ignored;
                std::filesystem::remove(temporary_path, ignored);
            }
            return luaL_error(
                    state, "DumpAllObjects failed with an unknown exception");
        }
    }

    int lua_execute_in_game_thread(lua_State* state) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(state, "ExecuteInGameThread requires a Lua callback");
        }
        if (!lua_isnoneornil(state, 2) && (!lua_isinteger(state, 2) || lua_tointeger(state, 2) != 1))
        {
            return luaL_error(state, "ExecuteInGameThread: only EGameThreadMethod.EngineTick is available in headless Linux mode");
        }
        return ue4ss::linux::core::schedule_lua_callback(state, 1, std::chrono::milliseconds{}, std::chrono::milliseconds{});
    }

    int lua_execute_async(lua_State* state) noexcept
    {
        if (lua_type(state, 1) != LUA_TFUNCTION)
        {
            return luaL_error(state, "ExecuteAsync requires a Lua callback");
        }
        return ue4ss::linux::core::schedule_lua_async_callback(
                state, 1, std::chrono::milliseconds{}, false);
    }

    int lua_execute_with_delay(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 1) || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "ExecuteWithDelay requires a millisecond delay and a Lua callback");
        }
        return ue4ss::linux::core::schedule_lua_async_callback(
                state, 2, std::chrono::milliseconds{lua_tointeger(state, 1)}, false);
    }

    int lua_loop_async(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 1) || lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state, "LoopAsync requires a millisecond delay and a Lua callback");
        }
        return ue4ss::linux::core::schedule_lua_async_callback(
                state, 2, std::chrono::milliseconds{lua_tointeger(state, 1)}, true);
    }

    int lua_get_current_thread_id(lua_State* state) noexcept
    {
        return push_thread_id(state, std::this_thread::get_id());
    }

    int lua_get_main_mod_thread_id(lua_State* state) noexcept
    {
        auto* owner = get_state_owner(state);
        return owner == nullptr ? luaL_error(state, "GetMainModThreadId has no mod state")
                                : push_thread_id(state, owner->main_thread_id());
    }

    int lua_get_async_thread_id(lua_State* state) noexcept
    {
        auto* owner = get_state_owner(state);
        return owner == nullptr ? luaL_error(state, "GetAsyncThreadId has no mod state")
                                : push_thread_id(state, owner->async_thread_id());
    }

    int lua_get_game_thread_id(lua_State* state) noexcept
    {
        auto* scheduler = get_scheduler(state);
        const auto game_thread = scheduler != nullptr ? scheduler->game_thread_id() : std::nullopt;
        if (!game_thread.has_value())
        {
            return luaL_error(state, "GetGameThreadId called before UGameEngine::Tick initialized the game thread");
        }
        return push_thread_id(state, *game_thread);
    }

    int lua_is_in_main_mod_thread(lua_State* state) noexcept
    {
        auto* owner = get_state_owner(state);
        lua_pushboolean(state,
                        owner != nullptr && owner->main_thread_id() == std::this_thread::get_id());
        return 1;
    }

    int lua_is_in_async_thread(lua_State* state) noexcept
    {
        auto* owner = get_state_owner(state);
        lua_pushboolean(state, owner != nullptr && owner->is_async_thread());
        return 1;
    }

    int lua_is_in_game_thread(lua_State* state) noexcept
    {
        auto* scheduler = get_scheduler(state);
        if (scheduler == nullptr || !scheduler->game_thread_id().has_value())
        {
            return luaL_error(state, "IsInGameThread called before UGameEngine::Tick initialized the game thread");
        }
        lua_pushboolean(state, scheduler->is_game_thread());
        return 1;
    }

    [[nodiscard]] bool read_action_handle(lua_State* state,
                                          int index,
                                          std::uint64_t& handle) noexcept
    {
        if (!lua_isinteger(state, index))
        {
            return false;
        }
        const lua_Integer value = lua_tointeger(state, index);
        if (value <= 0)
        {
            return false;
        }
        handle = static_cast<std::uint64_t>(value);
        return true;
    }

    [[nodiscard]] ue4ss::linux::core::GameThreadScheduler::OwnerId action_owner(
            lua_State* state) noexcept
    {
        return reinterpret_cast<ue4ss::linux::core::GameThreadScheduler::OwnerId>(
                get_state_owner(state));
    }

    int lua_execute_in_game_thread_with_delay(lua_State* state) noexcept
    {
        const int argument_count = lua_gettop(state);
        const bool explicit_handle = argument_count >= 3 && lua_isinteger(state, 1) &&
                                     lua_isinteger(state, 2) && lua_type(state, 3) == LUA_TFUNCTION;
        const bool automatic_handle = argument_count >= 2 && lua_isinteger(state, 1) &&
                                      lua_type(state, 2) == LUA_TFUNCTION;
        if (!explicit_handle && !automatic_handle)
        {
            return luaL_error(state,
                              "ExecuteInGameThreadWithDelay expects (delayMs, callback) or "
                              "(handle, delayMs, callback)");
        }

        auto* scheduler = get_scheduler(state);
        if (scheduler == nullptr || !scheduler->is_ready())
        {
            return luaL_error(state, "ExecuteInGameThreadWithDelay: EngineTick is unavailable");
        }
        const int delay_index = explicit_handle ? 2 : 1;
        const lua_Integer delay = lua_tointeger(state, delay_index);
        if (delay < 0)
        {
            return luaL_error(state, "ExecuteInGameThreadWithDelay requires a non-negative delay");
        }

        std::uint64_t requested_handle{};
        if (explicit_handle)
        {
            if (!read_action_handle(state, 1, requested_handle))
            {
                return luaL_error(state, "ExecuteInGameThreadWithDelay requires a positive handle");
            }
            if (scheduler->is_valid_handle(requested_handle))
            {
                return 0;
            }
        }
        const std::uint64_t handle = ue4ss::linux::core::schedule_lua_action(
                state,
                explicit_handle ? 3 : 2,
                std::chrono::milliseconds{delay},
                std::nullopt,
                false,
                requested_handle);
        if (handle == 0u)
        {
            return luaL_error(state, "ExecuteInGameThreadWithDelay could not queue the callback");
        }
        if (explicit_handle)
        {
            return 0;
        }
        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    int lua_retriggerable_execute_in_game_thread_with_delay(lua_State* state) noexcept
    {
        std::uint64_t handle{};
        if (!read_action_handle(state, 1, handle) || !lua_isinteger(state, 2) ||
            lua_tointeger(state, 2) < 0 || lua_type(state, 3) != LUA_TFUNCTION)
        {
            return luaL_error(state,
                              "RetriggerableExecuteInGameThreadWithDelay expects "
                              "(positiveHandle, nonNegativeDelayMs, callback)");
        }
        auto* scheduler = get_scheduler(state);
        auto* owner = get_state_owner(state);
        if (scheduler == nullptr || owner == nullptr || !scheduler->is_ready())
        {
            return luaL_error(state,
                              "RetriggerableExecuteInGameThreadWithDelay: EngineTick is unavailable");
        }
        const auto snapshot = scheduler->action_snapshot(handle);
        if (snapshot.has_value())
        {
            if (snapshot->owner != action_owner(state))
            {
                return luaL_error(state,
                                  "RetriggerableExecuteInGameThreadWithDelay: handle is owned by another mod");
            }
            if (!scheduler->retrigger_owned(
                        action_owner(state), handle, static_cast<std::int64_t>(lua_tointeger(state, 2))))
            {
                return luaL_error(state,
                                  "RetriggerableExecuteInGameThreadWithDelay could not reset the callback");
            }
            lua_pushinteger(state, static_cast<lua_Integer>(handle));
            return 1;
        }
        if (ue4ss::linux::core::schedule_lua_action(
                    state,
                    3,
                    std::chrono::milliseconds{lua_tointeger(state, 2)},
                    std::nullopt,
                    false,
                    handle) == 0u)
        {
            return luaL_error(state,
                              "RetriggerableExecuteInGameThreadWithDelay could not queue the callback");
        }
        return 0;
    }

    int lua_execute_in_game_thread_after_frames(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 1) || lua_tointeger(state, 1) < 0 ||
            lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state,
                              "ExecuteInGameThreadAfterFrames expects (nonNegativeFrames, callback)");
        }
        const std::uint64_t handle = ue4ss::linux::core::schedule_lua_action(
                state, 2, {}, static_cast<std::int64_t>(lua_tointeger(state, 1)), false);
        if (handle == 0u)
        {
            return luaL_error(state, "ExecuteInGameThreadAfterFrames could not queue the callback");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    int lua_loop_in_game_thread_with_delay(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 1) || lua_tointeger(state, 1) < 0 ||
            lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state,
                              "LoopInGameThreadWithDelay expects (nonNegativeDelayMs, callback)");
        }
        const std::uint64_t handle = ue4ss::linux::core::schedule_lua_action(
                state,
                2,
                std::chrono::milliseconds{lua_tointeger(state, 1)},
                std::nullopt,
                true);
        if (handle == 0u)
        {
            return luaL_error(state, "LoopInGameThreadWithDelay could not queue the callback");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    int lua_loop_in_game_thread_after_frames(lua_State* state) noexcept
    {
        if (!lua_isinteger(state, 1) || lua_tointeger(state, 1) < 0 ||
            lua_type(state, 2) != LUA_TFUNCTION)
        {
            return luaL_error(state,
                              "LoopInGameThreadAfterFrames expects (nonNegativeFrames, callback)");
        }
        const std::uint64_t handle = ue4ss::linux::core::schedule_lua_action(
                state, 2, {}, static_cast<std::int64_t>(lua_tointeger(state, 1)), true);
        if (handle == 0u)
        {
            return luaL_error(state, "LoopInGameThreadAfterFrames could not queue the callback");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    template <bool (ue4ss::linux::core::GameThreadScheduler::*Operation)(
                      ue4ss::linux::core::GameThreadScheduler::OwnerId,
                      std::uint64_t) noexcept>
    int lua_owned_action_operation(lua_State* state, const char* function_name) noexcept
    {
        std::uint64_t handle{};
        auto* scheduler = get_scheduler(state);
        auto* owner = get_state_owner(state);
        if (!read_action_handle(state, 1, handle))
        {
            return luaL_error(state, "%s expects a positive action handle", function_name);
        }
        const bool success = scheduler != nullptr && owner != nullptr &&
                             (scheduler->*Operation)(action_owner(state), handle);
        lua_pushboolean(state, success ? 1 : 0);
        return 1;
    }

    int lua_reset_delayed_action_timer(lua_State* state) noexcept
    {
        return lua_owned_action_operation<&ue4ss::linux::core::GameThreadScheduler::reset_owned>(
                state, "ResetDelayedActionTimer");
    }

    int lua_pause_delayed_action(lua_State* state) noexcept
    {
        return lua_owned_action_operation<&ue4ss::linux::core::GameThreadScheduler::pause_owned>(
                state, "PauseDelayedAction");
    }

    int lua_unpause_delayed_action(lua_State* state) noexcept
    {
        return lua_owned_action_operation<&ue4ss::linux::core::GameThreadScheduler::unpause_owned>(
                state, "UnpauseDelayedAction");
    }

    int lua_cancel_delayed_action(lua_State* state) noexcept
    {
        return lua_owned_action_operation<&ue4ss::linux::core::GameThreadScheduler::cancel_owned>(
                state, "CancelDelayedAction");
    }

    int lua_set_delayed_action_timer(lua_State* state) noexcept
    {
        std::uint64_t handle{};
        if (!read_action_handle(state, 1, handle) || !lua_isinteger(state, 2) ||
            lua_tointeger(state, 2) < 0)
        {
            return luaL_error(state,
                              "SetDelayedActionTimer expects (positiveHandle, nonNegativeDelay)");
        }
        auto* scheduler = get_scheduler(state);
        const bool success = scheduler != nullptr &&
                             scheduler->set_rate_owned(
                                     action_owner(state),
                                     handle,
                                     static_cast<std::int64_t>(lua_tointeger(state, 2)));
        lua_pushboolean(state, success ? 1 : 0);
        return 1;
    }

    int lua_is_valid_delayed_action_handle(lua_State* state) noexcept
    {
        std::uint64_t handle{};
        if (!read_action_handle(state, 1, handle))
        {
            return luaL_error(state, "IsValidDelayedActionHandle expects a positive action handle");
        }
        auto* scheduler = get_scheduler(state);
        lua_pushboolean(state, scheduler != nullptr && scheduler->is_valid_handle(handle));
        return 1;
    }

    int lua_is_delayed_action_active(lua_State* state) noexcept
    {
        std::uint64_t handle{};
        if (!read_action_handle(state, 1, handle))
        {
            return luaL_error(state, "IsDelayedActionActive expects a positive action handle");
        }
        auto* scheduler = get_scheduler(state);
        const auto snapshot = scheduler != nullptr ? scheduler->action_snapshot(handle) : std::nullopt;
        lua_pushboolean(state,
                        snapshot.has_value() &&
                                snapshot->state ==
                                        ue4ss::linux::core::GameThreadScheduler::ActionState::active);
        return 1;
    }

    int lua_is_delayed_action_paused(lua_State* state) noexcept
    {
        std::uint64_t handle{};
        if (!read_action_handle(state, 1, handle))
        {
            return luaL_error(state, "IsDelayedActionPaused expects a positive action handle");
        }
        auto* scheduler = get_scheduler(state);
        const auto snapshot = scheduler != nullptr ? scheduler->action_snapshot(handle) : std::nullopt;
        lua_pushboolean(state,
                        snapshot.has_value() &&
                                snapshot->state ==
                                        ue4ss::linux::core::GameThreadScheduler::ActionState::paused);
        return 1;
    }

    enum class ActionQuery
    {
        remaining,
        elapsed,
        rate,
    };

    template <ActionQuery Query>
    int lua_delayed_action_query(lua_State* state, const char* function_name) noexcept
    {
        std::uint64_t handle{};
        if (!read_action_handle(state, 1, handle))
        {
            return luaL_error(state, "%s expects a positive action handle", function_name);
        }
        auto* scheduler = get_scheduler(state);
        const auto snapshot = scheduler != nullptr ? scheduler->action_snapshot(handle) : std::nullopt;
        std::int64_t value = -1;
        if (snapshot.has_value())
        {
            if constexpr (Query == ActionQuery::remaining)
            {
                value = snapshot->remaining;
            }
            else if constexpr (Query == ActionQuery::elapsed)
            {
                value = snapshot->elapsed;
            }
            else
            {
                value = snapshot->rate;
            }
        }
        lua_pushinteger(state, static_cast<lua_Integer>(value));
        return 1;
    }

    int lua_get_delayed_action_time_remaining(lua_State* state) noexcept
    {
        return lua_delayed_action_query<ActionQuery::remaining>(
                state, "GetDelayedActionTimeRemaining");
    }

    int lua_get_delayed_action_time_elapsed(lua_State* state) noexcept
    {
        return lua_delayed_action_query<ActionQuery::elapsed>(
                state, "GetDelayedActionTimeElapsed");
    }

    int lua_get_delayed_action_rate(lua_State* state) noexcept
    {
        return lua_delayed_action_query<ActionQuery::rate>(state, "GetDelayedActionRate");
    }

    int lua_clear_all_delayed_actions(lua_State* state) noexcept
    {
        auto* scheduler = get_scheduler(state);
        auto* owner = get_state_owner(state);
        const std::size_t count = scheduler != nullptr && owner != nullptr
                                          ? scheduler->clear_owner(action_owner(state))
                                          : 0u;
        lua_pushinteger(state, static_cast<lua_Integer>(count));
        return 1;
    }

    int lua_make_action_handle(lua_State* state) noexcept
    {
        auto* scheduler = get_scheduler(state);
        if (scheduler == nullptr)
        {
            return luaL_error(state, "MakeActionHandle: EngineTick is unavailable");
        }
        const std::uint64_t handle = scheduler->reserve_handle();
        if (handle == 0u)
        {
            return luaL_error(state, "MakeActionHandle could not allocate a handle");
        }
        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    void register_method(
            lua_State* state, const char* name, lua_CFunction function);

    int lua_ue4ss_get_version(lua_State* state) noexcept
    {
        lua_pushinteger(state, UE4SS_LINUX_VERSION_MAJOR);
        lua_pushinteger(state, UE4SS_LINUX_VERSION_MINOR);
        lua_pushinteger(state, UE4SS_LINUX_VERSION_HOTFIX);
        return 3;
    }

    int lua_unreal_version_get_major(lua_State* state) noexcept
    {
        lua_pushinteger(state, 5);
        return 1;
    }

    int lua_unreal_version_get_minor(lua_State* state) noexcept
    {
        lua_pushinteger(state, 1);
        return 1;
    }

    enum class UnrealVersionComparison : std::uint8_t
    {
        equal,
        at_least,
        at_most,
        below,
        above,
    };

    template <UnrealVersionComparison Comparison>
    int lua_unreal_version_compare(lua_State* state) noexcept
    {
        const int first = lua_type(state, 1) == LUA_TTABLE ? 2 : 1;
        if (!lua_isinteger(state, first) || !lua_isinteger(state, first + 1) ||
            lua_tointeger(state, first) < 0 ||
            lua_tointeger(state, first + 1) < 0 ||
            static_cast<std::uint64_t>(lua_tointeger(state, first)) >
                    std::numeric_limits<std::uint32_t>::max() ||
            static_cast<std::uint64_t>(lua_tointeger(state, first + 1)) >
                    std::numeric_limits<std::uint32_t>::max())
        {
            return luaL_error(
                    state,
                    "UnrealVersion comparison requires non-negative uint32 major/minor integers");
        }
        const auto requested = std::pair{
                static_cast<std::uint32_t>(lua_tointeger(state, first)),
                static_cast<std::uint32_t>(lua_tointeger(state, first + 1))};
        constexpr std::pair<std::uint32_t, std::uint32_t> current{5u, 1u};
        bool result{};
        if constexpr (Comparison == UnrealVersionComparison::equal)
        {
            result = current == requested;
        }
        else if constexpr (Comparison == UnrealVersionComparison::at_least)
        {
            result = current >= requested;
        }
        else if constexpr (Comparison == UnrealVersionComparison::at_most)
        {
            result = current <= requested;
        }
        else if constexpr (Comparison == UnrealVersionComparison::below)
        {
            result = current < requested;
        }
        else
        {
            result = current > requested;
        }
        lua_pushboolean(state, result ? 1 : 0);
        return 1;
    }

    int lua_package_name_is_short(lua_State* state) noexcept
    {
        const int value_index = lua_type(state, 1) == LUA_TTABLE ? 2 : 1;
        if (lua_type(state, value_index) != LUA_TSTRING)
        {
            return luaL_error(
                    state,
                    "FPackageName:IsShortPackageName requires a string");
        }
        std::size_t length{};
        const char* value = lua_tolstring(state, value_index, &length);
        if (value == nullptr)
        {
            return luaL_error(
                    state,
                    "FPackageName:IsShortPackageName requires a string");
        }
        lua_pushboolean(
                state,
                std::string_view{value, length}.find('/') == std::string_view::npos);
        return 1;
    }

    int lua_package_name_is_valid_long(lua_State* state) noexcept
    {
        const int value_index = lua_type(state, 1) == LUA_TTABLE ? 2 : 1;
        if (lua_type(state, value_index) != LUA_TSTRING)
        {
            return luaL_error(
                    state,
                    "FPackageName:IsValidLongPackageName requires a string");
        }
        std::size_t length{};
        const char* value = lua_tolstring(state, value_index, &length);
        if (value == nullptr)
        {
            return luaL_error(
                    state,
                    "FPackageName:IsValidLongPackageName requires a string");
        }
        bool valid{};
        try
        {
            const std::u16string unreal = ue4ss::linux::utf8_to_unreal(
                    std::string_view{value, length});
            constexpr std::u16string_view invalid =
                    u"\\:*?\"<>|' ,.&!~\n\r\t@#";
            valid = unreal.size() >= 4u && unreal.front() == u'/' &&
                    unreal.back() != u'/' &&
                    unreal.find_first_of(invalid) == std::u16string::npos;
        }
        catch (...)
        {
            valid = false;
        }
        lua_pushboolean(state, valid ? 1 : 0);
        return 1;
    }

    struct SharedLuaValue
    {
        enum class Kind : std::uint8_t
        {
            nil,
            boolean,
            integer,
            number,
            string,
            light_userdata,
            object,
        } kind{Kind::nil};
        std::variant<std::monostate,
                     bool,
                     std::int64_t,
                     double,
                     std::string,
                     void*,
                     ue4ss::linux::core::ReadOnlyUObjectHandle>
                value;
    };

    std::mutex g_shared_lua_values_mutex;
    std::unordered_map<std::string, SharedLuaValue> g_shared_lua_values;

    int lua_modref_set_shared_variable(lua_State* state) noexcept
    {
        const int name_index = lua_type(state, 1) == LUA_TTABLE ? 2 : 1;
        const int value_index = name_index + 1;
        if (lua_type(state, name_index) != LUA_TSTRING ||
            lua_gettop(state) < value_index)
        {
            return luaL_error(
                    state,
                    "ModRef:SetSharedVariable requires a name and value");
        }
        std::size_t name_length{};
        const char* name = lua_tolstring(state, name_index, &name_length);
        SharedLuaValue stored;
        switch (lua_type(state, value_index))
        {
        case LUA_TNIL:
            stored.kind = SharedLuaValue::Kind::nil;
            break;
        case LUA_TBOOLEAN:
            stored.kind = SharedLuaValue::Kind::boolean;
            stored.value = lua_toboolean(state, value_index) != 0;
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(state, value_index))
            {
                stored.kind = SharedLuaValue::Kind::integer;
                stored.value = static_cast<std::int64_t>(
                        lua_tointeger(state, value_index));
            }
            else
            {
                stored.kind = SharedLuaValue::Kind::number;
                stored.value = static_cast<double>(
                        lua_tonumber(state, value_index));
            }
            break;
        case LUA_TSTRING:
        {
            std::size_t value_length{};
            const char* value =
                    lua_tolstring(state, value_index, &value_length);
            stored.kind = SharedLuaValue::Kind::string;
            stored.value = std::string{value, value_length};
            break;
        }
        case LUA_TLIGHTUSERDATA:
            stored.kind = SharedLuaValue::Kind::light_userdata;
            stored.value = lua_touserdata(state, value_index);
            break;
        case LUA_TUSERDATA:
            if (auto* object = get_object(state, value_index); object != nullptr)
            {
                stored.kind = SharedLuaValue::Kind::object;
                stored.value = object->handle;
                break;
            }
            [[fallthrough]];
        default:
            return luaL_error(
                    state,
                    "ModRef shared variables support only nil, bool, number, string, lightuserdata, and UObject values");
        }
        try
        {
            std::lock_guard lock{g_shared_lua_values_mutex};
            g_shared_lua_values[std::string{name, name_length}] =
                    std::move(stored);
            return 0;
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "%s", exception.what());
        }
    }

    int lua_modref_get_shared_variable(lua_State* state) noexcept
    {
        const int name_index = lua_type(state, 1) == LUA_TTABLE ? 2 : 1;
        if (lua_type(state, name_index) != LUA_TSTRING)
        {
            return luaL_error(
                    state,
                    "ModRef:GetSharedVariable requires a name");
        }
        std::size_t length{};
        const char* name = lua_tolstring(state, name_index, &length);
        SharedLuaValue stored;
        bool found{};
        try
        {
            std::lock_guard lock{g_shared_lua_values_mutex};
            const auto value = g_shared_lua_values.find(
                    std::string{name, length});
            if (value != g_shared_lua_values.end())
            {
                stored = value->second;
                found = true;
            }
        }
        catch (const std::exception& exception)
        {
            return luaL_error(state, "%s", exception.what());
        }
        if (!found || stored.kind == SharedLuaValue::Kind::nil)
        {
            lua_pushnil(state);
            return 1;
        }
        switch (stored.kind)
        {
        case SharedLuaValue::Kind::boolean:
            lua_pushboolean(state, std::get<bool>(stored.value));
            break;
        case SharedLuaValue::Kind::integer:
            lua_pushinteger(
                    state,
                    static_cast<lua_Integer>(
                            std::get<std::int64_t>(stored.value)));
            break;
        case SharedLuaValue::Kind::number:
            lua_pushnumber(state, std::get<double>(stored.value));
            break;
        case SharedLuaValue::Kind::string:
        {
            const auto& value = std::get<std::string>(stored.value);
            lua_pushlstring(state, value.data(), value.size());
            break;
        }
        case SharedLuaValue::Kind::light_userdata:
            lua_pushlightuserdata(state, std::get<void*>(stored.value));
            break;
        case SharedLuaValue::Kind::object:
            return push_object(
                    state,
                    std::get<ue4ss::linux::core::ReadOnlyUObjectHandle>(
                            stored.value));
        case SharedLuaValue::Kind::nil:
            lua_pushnil(state);
            break;
        }
        return 1;
    }

    int lua_modref_type(lua_State* state) noexcept
    {
        lua_pushliteral(state, "ModRef");
        return 1;
    }

    void register_compatibility_classes(lua_State* state) noexcept
    {
        lua_createtable(state, 0, 1);
        register_method(state, "GetVersion", lua_ue4ss_get_version);
        lua_setglobal(state, "UE4SS");

        lua_createtable(state, 0, 7);
        register_method(state, "GetMajor", lua_unreal_version_get_major);
        register_method(state, "GetMinor", lua_unreal_version_get_minor);
        register_method(
                state,
                "IsEqual",
                lua_unreal_version_compare<UnrealVersionComparison::equal>);
        register_method(
                state,
                "IsAtLeast",
                lua_unreal_version_compare<UnrealVersionComparison::at_least>);
        register_method(
                state,
                "IsAtMost",
                lua_unreal_version_compare<UnrealVersionComparison::at_most>);
        register_method(
                state,
                "IsBelow",
                lua_unreal_version_compare<UnrealVersionComparison::below>);
        register_method(
                state,
                "IsAbove",
                lua_unreal_version_compare<UnrealVersionComparison::above>);
        lua_setglobal(state, "UnrealVersion");

        lua_createtable(state, 0, 2);
        register_method(
                state,
                "IsShortPackageName",
                lua_package_name_is_short);
        register_method(
                state,
                "IsValidLongPackageName",
                lua_package_name_is_valid_long);
        lua_setglobal(state, "FPackageName");

        lua_createtable(state, 0, 3);
        register_method(
                state,
                "SetSharedVariable",
                lua_modref_set_shared_variable);
        register_method(
                state,
                "GetSharedVariable",
                lua_modref_get_shared_variable);
        register_method(state, "type", lua_modref_type);
        lua_setglobal(state, "ModRef");
    }

    struct LuaPropertyTypeEntry
    {
        const char* name;
        std::int32_t size;
    };

    constexpr std::array<LuaPropertyTypeEntry, 22> k_property_types{{
            {"ObjectProperty", 8},
            {"ObjectPtrProperty", 8},
            {"Int8Property", 1},
            {"Int16Property", 2},
            {"IntProperty", 4},
            {"Int64Property", 8},
            {"ByteProperty", 1},
            {"UInt16Property", 2},
            {"UInt32Property", 4},
            {"UInt64Property", 8},
            {"NameProperty", 8},
            {"FloatProperty", 4},
            {"BoolProperty", 1},
            {"ArrayProperty", 16},
            {"SetProperty", 80},
            {"MapProperty", 80},
            {"StructProperty", 0},
            {"ClassProperty", 8},
            {"WeakObjectProperty", 8},
            {"EnumProperty", 0},
            {"TextProperty", 24},
            {"StrProperty", 16},
    }};

    void register_property_types(lua_State* state) noexcept
    {
        lua_createtable(state, 0, static_cast<int>(k_property_types.size()));
        for (std::size_t index = 0; index < k_property_types.size(); ++index)
        {
            const auto& entry = k_property_types[index];
            lua_createtable(state, 0, 4);
            lua_pushstring(state, entry.name);
            lua_setfield(state, -2, "Name");
            lua_pushinteger(state, static_cast<lua_Integer>(entry.size));
            lua_setfield(state, -2, "Size");
            // Linux never dereferences this compatibility token. Property:IsA
            // compares the public Name field, and RegisterCustomProperty
            // validates the complete descriptor before accepting it.
            lua_pushinteger(state, static_cast<lua_Integer>(index + 1u));
            lua_setfield(state, -2, "FFieldClassPointer");
            lua_pushinteger(state, 0);
            lua_setfield(state, -2, "StaticPointer");
            lua_setfield(state, -2, entry.name);
        }
        lua_setglobal(state, "PropertyTypes");
    }

    void register_input_constants(lua_State* state) noexcept
    {
        // Keep the public Windows virtual-key values available even though a
        // headless server has no input backend. Existing mods can therefore
        // load unchanged and receive the explicit headless error only if they
        // actually try to register an input callback.
        lua_createtable(state, 0, 166);
#define UE4SS_KEY(name)                                                                          \
        lua_pushinteger(state, static_cast<lua_Integer>(RC::Input::Key::name));                  \
        lua_setfield(state, -2, #name)
        UE4SS_KEY(LEFT_MOUSE_BUTTON);
        UE4SS_KEY(RIGHT_MOUSE_BUTTON);
        UE4SS_KEY(CANCEL);
        UE4SS_KEY(MIDDLE_MOUSE_BUTTON);
        UE4SS_KEY(XBUTTON_ONE);
        UE4SS_KEY(XBUTTON_TWO);
        UE4SS_KEY(BACKSPACE);
        UE4SS_KEY(TAB);
        UE4SS_KEY(CLEAR);
        UE4SS_KEY(RETURN);
        UE4SS_KEY(PAUSE);
        UE4SS_KEY(CAPS_LOCK);
        UE4SS_KEY(IME_KANA);
        UE4SS_KEY(IME_HANGUEL);
        UE4SS_KEY(IME_HANGUL);
        UE4SS_KEY(IME_ON);
        UE4SS_KEY(IME_JUNJA);
        UE4SS_KEY(IME_FINAL);
        UE4SS_KEY(IME_HANJA);
        UE4SS_KEY(IME_KANJI);
        UE4SS_KEY(IME_OFF);
        UE4SS_KEY(ESCAPE);
        UE4SS_KEY(IME_CONVERT);
        UE4SS_KEY(IME_NONCONVERT);
        UE4SS_KEY(IME_ACCEPT);
        UE4SS_KEY(IME_MODECHANGE);
        UE4SS_KEY(SPACE);
        UE4SS_KEY(PAGE_UP);
        UE4SS_KEY(PAGE_DOWN);
        UE4SS_KEY(END);
        UE4SS_KEY(HOME);
        UE4SS_KEY(LEFT_ARROW);
        UE4SS_KEY(UP_ARROW);
        UE4SS_KEY(RIGHT_ARROW);
        UE4SS_KEY(DOWN_ARROW);
        UE4SS_KEY(SELECT);
        UE4SS_KEY(PRINT);
        UE4SS_KEY(EXECUTE);
        UE4SS_KEY(PRINT_SCREEN);
        UE4SS_KEY(INS);
        UE4SS_KEY(DEL);
        UE4SS_KEY(HELP);
        UE4SS_KEY(ZERO);
        UE4SS_KEY(ONE);
        UE4SS_KEY(TWO);
        UE4SS_KEY(THREE);
        UE4SS_KEY(FOUR);
        UE4SS_KEY(FIVE);
        UE4SS_KEY(SIX);
        UE4SS_KEY(SEVEN);
        UE4SS_KEY(EIGHT);
        UE4SS_KEY(NINE);
        UE4SS_KEY(A);
        UE4SS_KEY(B);
        UE4SS_KEY(C);
        UE4SS_KEY(D);
        UE4SS_KEY(E);
        UE4SS_KEY(F);
        UE4SS_KEY(G);
        UE4SS_KEY(H);
        UE4SS_KEY(I);
        UE4SS_KEY(J);
        UE4SS_KEY(K);
        UE4SS_KEY(L);
        UE4SS_KEY(M);
        UE4SS_KEY(N);
        UE4SS_KEY(O);
        UE4SS_KEY(P);
        UE4SS_KEY(Q);
        UE4SS_KEY(R);
        UE4SS_KEY(S);
        UE4SS_KEY(T);
        UE4SS_KEY(U);
        UE4SS_KEY(V);
        UE4SS_KEY(W);
        UE4SS_KEY(X);
        UE4SS_KEY(Y);
        UE4SS_KEY(Z);
        UE4SS_KEY(LEFT_WIN);
        UE4SS_KEY(RIGHT_WIN);
        UE4SS_KEY(APPS);
        UE4SS_KEY(SLEEP);
        UE4SS_KEY(NUM_ZERO);
        UE4SS_KEY(NUM_ONE);
        UE4SS_KEY(NUM_TWO);
        UE4SS_KEY(NUM_THREE);
        UE4SS_KEY(NUM_FOUR);
        UE4SS_KEY(NUM_FIVE);
        UE4SS_KEY(NUM_SIX);
        UE4SS_KEY(NUM_SEVEN);
        UE4SS_KEY(NUM_EIGHT);
        UE4SS_KEY(NUM_NINE);
        UE4SS_KEY(MULTIPLY);
        UE4SS_KEY(ADD);
        UE4SS_KEY(SEPARATOR);
        UE4SS_KEY(SUBTRACT);
        UE4SS_KEY(DECIMAL);
        UE4SS_KEY(DIVIDE);
        UE4SS_KEY(F1);
        UE4SS_KEY(F2);
        UE4SS_KEY(F3);
        UE4SS_KEY(F4);
        UE4SS_KEY(F5);
        UE4SS_KEY(F6);
        UE4SS_KEY(F7);
        UE4SS_KEY(F8);
        UE4SS_KEY(F9);
        UE4SS_KEY(F10);
        UE4SS_KEY(F11);
        UE4SS_KEY(F12);
        UE4SS_KEY(F13);
        UE4SS_KEY(F14);
        UE4SS_KEY(F15);
        UE4SS_KEY(F16);
        UE4SS_KEY(F17);
        UE4SS_KEY(F18);
        UE4SS_KEY(F19);
        UE4SS_KEY(F20);
        UE4SS_KEY(F21);
        UE4SS_KEY(F22);
        UE4SS_KEY(F23);
        UE4SS_KEY(F24);
        UE4SS_KEY(NUM_LOCK);
        UE4SS_KEY(SCROLL_LOCK);
        UE4SS_KEY(BROWSER_BACK);
        UE4SS_KEY(BROWSER_FORWARD);
        UE4SS_KEY(BROWSER_REFRESH);
        UE4SS_KEY(BROWSER_STOP);
        UE4SS_KEY(BROWSER_SEARCH);
        UE4SS_KEY(BROWSER_FAVORITES);
        UE4SS_KEY(BROWSER_HOME);
        UE4SS_KEY(VOLUME_MUTE);
        UE4SS_KEY(VOLUME_DOWN);
        UE4SS_KEY(VOLUME_UP);
        UE4SS_KEY(MEDIA_NEXT_TRACK);
        UE4SS_KEY(MEDIA_PREV_TRACK);
        UE4SS_KEY(MEDIA_STOP);
        UE4SS_KEY(MEDIA_PLAY_PAUSE);
        UE4SS_KEY(LAUNCH_MAIL);
        UE4SS_KEY(LAUNCH_MEDIA_SELECT);
        UE4SS_KEY(LAUNCH_APP1);
        UE4SS_KEY(LAUNCH_APP2);
        UE4SS_KEY(OEM_ONE);
        UE4SS_KEY(OEM_PLUS);
        UE4SS_KEY(OEM_COMMA);
        UE4SS_KEY(OEM_MINUS);
        UE4SS_KEY(OEM_PERIOD);
        UE4SS_KEY(OEM_TWO);
        UE4SS_KEY(OEM_THREE);
        UE4SS_KEY(OEM_FOUR);
        UE4SS_KEY(OEM_FIVE);
        UE4SS_KEY(OEM_SIX);
        UE4SS_KEY(OEM_SEVEN);
        UE4SS_KEY(OEM_EIGHT);
        UE4SS_KEY(OEM_102);
        UE4SS_KEY(IME_PROCESS);
        UE4SS_KEY(PACKET);
        UE4SS_KEY(ATTN);
        UE4SS_KEY(CRSEL);
        UE4SS_KEY(EXSEL);
        UE4SS_KEY(EREOF);
        UE4SS_KEY(PLAY);
        UE4SS_KEY(ZOOM);
        UE4SS_KEY(PA1);
        UE4SS_KEY(OEM_CLEAR);
#undef UE4SS_KEY
        lua_setglobal(state, "Key");

        lua_createtable(state, 0, 3);
        lua_pushinteger(state, 0x10);
        lua_setfield(state, -2, "SHIFT");
        lua_pushinteger(state, 0x11);
        lua_setfield(state, -2, "CONTROL");
        lua_pushinteger(state, 0x12);
        lua_setfield(state, -2, "ALT");
        lua_setglobal(state, "ModifierKey");
    }

    [[nodiscard]] bool read_required_string_field(
            lua_State* state,
            int table_index,
            const char* field,
            std::string& output,
            std::string& error) noexcept
    {
        table_index = lua_absindex(state, table_index);
        lua_getfield(state, table_index, field);
        if (lua_type(state, -1) != LUA_TSTRING)
        {
            lua_pop(state, 1);
            error = std::string{"RegisterCustomProperty field '"} + field +
                    "' must be a string";
            return false;
        }
        std::size_t length{};
        const char* value = lua_tolstring(state, -1, &length);
        output.assign(value, length);
        lua_pop(state, 1);
        if (output.empty())
        {
            error = std::string{"RegisterCustomProperty field '"} + field +
                    "' must not be empty";
            return false;
        }
        return true;
    }

    [[nodiscard]] bool read_int32_value(
            lua_State* state,
            int index,
            std::int32_t& output,
            std::string_view field,
            std::string& error) noexcept
    {
        if (!lua_isinteger(state, index))
        {
            error = "RegisterCustomProperty field '" + std::string{field} +
                    "' must be an integer";
            return false;
        }
        const lua_Integer value = lua_tointeger(state, index);
        if (value < std::numeric_limits<std::int32_t>::min() ||
            value > std::numeric_limits<std::int32_t>::max())
        {
            error = "RegisterCustomProperty field '" + std::string{field} +
                    "' is outside the 32-bit integer range";
            return false;
        }
        output = static_cast<std::int32_t>(value);
        return true;
    }

    [[nodiscard]] bool read_custom_property_type(
            lua_State* state,
            int table_index,
            ue4ss::linux::core::CustomPropertyTypeDescription& output,
            std::string_view field_path,
            std::string& error) noexcept
    {
        table_index = lua_absindex(state, table_index);
        if (!read_required_string_field(
                    state, table_index, "Name", output.type_name, error))
        {
            error = std::string{field_path} + ".Name: " + error;
            return false;
        }
        lua_getfield(state, table_index, "Size");
        if (!read_int32_value(
                    state,
                    -1,
                    output.element_size,
                    std::string{field_path} + ".Size",
                    error))
        {
            lua_pop(state, 1);
            return false;
        }
        lua_pop(state, 1);
        lua_getfield(state, table_index, "FFieldClassPointer");
        if (!lua_isinteger(state, -1) || lua_tointeger(state, -1) == 0)
        {
            lua_pop(state, 1);
            error = std::string{field_path} +
                    ".FFieldClassPointer must be a non-zero integer";
            return false;
        }
        lua_pop(state, 1);
        lua_getfield(state, table_index, "StaticPointer");
        if (!lua_isnil(state, -1) && !lua_isinteger(state, -1))
        {
            lua_pop(state, 1);
            error = std::string{field_path} +
                    ".StaticPointer must be an integer when present";
            return false;
        }
        lua_pop(state, 1);
        return true;
    }

    int lua_register_custom_property(lua_State* state) noexcept
    {
        auto* runtime = get_runtime(state);
        if (runtime == nullptr || lua_type(state, 1) != LUA_TTABLE)
        {
            return luaL_error(
                    state,
                    "RegisterCustomProperty requires one property-info table and a validated Unreal runtime");
        }
        bool failed{};
        {
            std::string error;
            ue4ss::linux::core::CustomPropertyDescription property;
            std::string relative_property;
            std::int32_t relative_offset{};
            bool relative{};
            if (error.empty() &&
                !read_required_string_field(
                        state, 1, "Name", property.name, error))
            {
                // Preserve the field diagnostic.
            }
            lua_getfield(state, 1, "Type");
            if (error.empty() && lua_type(state, -1) != LUA_TTABLE)
            {
                error = "RegisterCustomProperty field 'Type' must be a PropertyTypes table";
            }
            if (error.empty() &&
                !read_custom_property_type(
                        state, -1, property.type, "Type", error))
            {
                // Preserve the type diagnostic.
            }
            lua_pop(state, 1);

            std::string owner_path;
            if (error.empty() &&
                !read_required_string_field(
                        state, 1, "BelongsToClass", owner_path, error))
            {
                // Preserve the field diagnostic.
            }

            lua_getfield(state, 1, "OffsetInternal");
            if (error.empty() && lua_isinteger(state, -1))
            {
                static_cast<void>(read_int32_value(
                        state,
                        -1,
                        property.offset,
                        "OffsetInternal",
                        error));
            }
            else if (error.empty() && lua_type(state, -1) == LUA_TTABLE)
            {
                relative = true;
                const int offset_table = lua_absindex(state, -1);
                static_cast<void>(read_required_string_field(
                        state,
                        offset_table,
                        "Property",
                        relative_property,
                        error));
                lua_getfield(state, offset_table, "RelativeOffset");
                if (error.empty())
                {
                    static_cast<void>(read_int32_value(
                            state,
                            -1,
                            relative_offset,
                            "OffsetInternal.RelativeOffset",
                            error));
                }
                lua_pop(state, 1);
            }
            else if (error.empty())
            {
                error = "RegisterCustomProperty field 'OffsetInternal' must be an integer or relative-offset table";
            }
            lua_pop(state, 1);

            if (error.empty() && property.type.type_name == "ArrayProperty")
            {
                lua_getfield(state, 1, "ArrayProperty");
                if (lua_type(state, -1) != LUA_TTABLE)
                {
                    error = "RegisterCustomProperty ArrayProperty metadata is missing";
                }
                else
                {
                    const int array_table = lua_absindex(state, -1);
                    lua_getfield(state, array_table, "Type");
                    if (lua_type(state, -1) != LUA_TTABLE)
                    {
                        error = "RegisterCustomProperty ArrayProperty.Type must be a PropertyTypes table";
                    }
                    else
                    {
                        ue4ss::linux::core::CustomPropertyTypeDescription inner;
                        if (read_custom_property_type(
                                    state,
                                    -1,
                                    inner,
                                    "ArrayProperty.Type",
                                    error))
                        {
                            property.array_inner = std::move(inner);
                        }
                    }
                    lua_pop(state, 1);
                }
                lua_pop(state, 1);
            }

            if (error.empty())
            {
                constexpr std::string_view class_prefix{"Class "};
                std::string_view requested_path{owner_path};
                if (requested_path.starts_with(class_prefix))
                {
                    requested_path.remove_prefix(class_prefix.size());
                }
                const auto found = runtime->find_by_path(requested_path, 1u);
                std::string class_error;
                if (!found.success || found.objects.empty() ||
                    !runtime->is_a(
                            found.objects.front(), "Class", class_error))
                {
                    error = class_error.empty()
                                    ? "RegisterCustomProperty could not find UClass '" +
                                              std::string{requested_path} + "'"
                                    : std::move(class_error);
                }
                else
                {
                    property.owner_class = found.objects.front();
                }
            }
            if (error.empty() && relative)
            {
                std::vector<ue4ss::linux::core::ReflectedPropertyDescription>
                        reflected;
                if (!runtime->enumerate_properties(
                            property.owner_class, reflected, error, true))
                {
                    // Preserve the enumeration diagnostic.
                }
                else
                {
                    const auto base = std::find_if(
                            reflected.begin(),
                            reflected.end(),
                            [&relative_property](const auto& candidate) {
                                return candidate.name == relative_property;
                            });
                    if (base == reflected.end())
                    {
                        error = "RegisterCustomProperty could not find relative base property '" +
                                relative_property + "'";
                    }
                    else
                    {
                        const std::int64_t combined =
                                static_cast<std::int64_t>(base->offset) +
                                static_cast<std::int64_t>(relative_offset);
                        if (combined < std::numeric_limits<std::int32_t>::min() ||
                            combined > std::numeric_limits<std::int32_t>::max())
                        {
                            error = "RegisterCustomProperty relative offset overflows a 32-bit offset";
                        }
                        else
                        {
                            property.offset = static_cast<std::int32_t>(combined);
                        }
                    }
                }
            }
            if (error.empty() &&
                !runtime->register_custom_property(std::move(property), error) &&
                error.empty())
            {
                error = "RegisterCustomProperty was rejected without a diagnostic";
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    void register_method(lua_State* state, const char* name, lua_CFunction function)
    {
        lua_pushcfunction(state, function);
        lua_setfield(state, -2, name);
    }

    [[nodiscard]] ue4ss::linux::core::LuaExecutionResult protected_call(lua_State* state, int load_status) noexcept
    {
        if (state == nullptr)
        {
            return {false, "Lua state allocation failed"};
        }
        if (load_status != LUA_OK)
        {
            const char* error = lua_tostring(state, -1);
            std::string detail = error != nullptr ? error : "Lua chunk could not be loaded";
            lua_settop(state, 0);
            return {false, std::move(detail)};
        }
        const int call_status = lua_pcall(state, 0, 0, 0);
        if (call_status != LUA_OK)
        {
            const char* error = lua_tostring(state, -1);
            std::string detail = error != nullptr ? error : "Lua chunk failed without an error message";
            lua_settop(state, 0);
            return {false, std::move(detail)};
        }
        lua_settop(state, 0);
        return {true, "Lua chunk completed in protected mode"};
    }
}

namespace ue4ss::linux::core
{
    int register_lua_begin_play_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_begin_play_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterBeginPlay hook is unavailable because the validated native AActor::BeginPlay manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post ? "RegisterBeginPlayPostHook requires a callback"
                             : "RegisterBeginPlayPreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                BeginPlayHookManager::Callback pre_callback =
                        !post ? BeginPlayHookManager::Callback{[owner, callback_reference](const ReadOnlyUObjectHandle& actor) {
                            owner->invoke_begin_play_hook_callback(callback_reference, actor);
                        }} : BeginPlayHookManager::Callback{};
                BeginPlayHookManager::Callback post_callback =
                        post ? BeginPlayHookManager::Callback{[owner, callback_reference](const ReadOnlyUObjectHandle& actor) {
                            owner->invoke_begin_play_hook_callback(callback_reference, actor);
                        }} : BeginPlayHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "AActor::BeginPlay hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_begin_play_references.push_back({ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_end_play_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_end_play_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterEndPlay hook is unavailable because the validated native AActor::EndPlay manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post ? "RegisterEndPlayPostHook requires a callback"
                             : "RegisterEndPlayPreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                EndPlayHookManager::Callback pre_callback =
                        !post ? EndPlayHookManager::Callback{
                                        [owner, callback_reference](const ReadOnlyUObjectHandle& actor,
                                                                    std::int32_t& reason) {
                                            owner->invoke_end_play_hook_callback(
                                                    callback_reference, actor, reason);
                                        }}
                              : EndPlayHookManager::Callback{};
                EndPlayHookManager::Callback post_callback =
                        post ? EndPlayHookManager::Callback{
                                       [owner, callback_reference](const ReadOnlyUObjectHandle& actor,
                                                                   std::int32_t& reason) {
                                           owner->invoke_end_play_hook_callback(
                                                   callback_reference, actor, reason);
                                       }}
                             : EndPlayHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "AActor::EndPlay hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_end_play_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_init_game_state_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_init_game_state_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterInitGameState hook is unavailable because the validated native AGameModeBase::InitGameState manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post ? "RegisterInitGameStatePostHook requires a callback"
                             : "RegisterInitGameStatePreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                InitGameStateHookManager::Callback pre_callback =
                        !post ? InitGameStateHookManager::Callback{
                                        [owner, callback_reference](
                                                const ReadOnlyUObjectHandle& game_mode) {
                                            owner->invoke_init_game_state_hook_callback(
                                                    callback_reference, game_mode);
                                        }}
                              : InitGameStateHookManager::Callback{};
                InitGameStateHookManager::Callback post_callback =
                        post ? InitGameStateHookManager::Callback{
                                       [owner, callback_reference](
                                               const ReadOnlyUObjectHandle& game_mode) {
                                           owner->invoke_init_game_state_hook_callback(
                                                   callback_reference, game_mode);
                                       }}
                             : InitGameStateHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "AGameModeBase::InitGameState hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_init_game_state_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_load_map_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_load_map_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterLoadMap hook is unavailable because the validated native UEngine::LoadMap manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post ? "RegisterLoadMapPostHook requires a callback"
                             : "RegisterLoadMapPreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                LoadMapHookManager::Callback pre_callback =
                        !post ? LoadMapHookManager::Callback{
                                        [owner, callback_reference](
                                                const LoadMapHookInvocation& invocation) {
                                            return owner->invoke_load_map_hook_callback(
                                                    callback_reference, invocation);
                                        }}
                              : LoadMapHookManager::Callback{};
                LoadMapHookManager::Callback post_callback =
                        post ? LoadMapHookManager::Callback{
                                       [owner, callback_reference](
                                               const LoadMapHookInvocation& invocation) {
                                           return owner->invoke_load_map_hook_callback(
                                                   callback_reference, invocation);
                                       }}
                             : LoadMapHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "UEngine::LoadMap hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_load_map_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_process_console_exec_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_process_console_exec_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterProcessConsoleExec hook is unavailable because the validated native UObject::ProcessConsoleExec manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post ? "RegisterProcessConsoleExecPostHook requires a callback"
                             : "RegisterProcessConsoleExecPreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                ProcessConsoleExecHookManager::Callback pre_callback =
                        !post ? ProcessConsoleExecHookManager::Callback{
                                        [owner, callback_reference](
                                                const ProcessConsoleExecInvocation& invocation) {
                                            return owner->invoke_process_console_exec_hook_callback(
                                                    callback_reference, invocation);
                                        }}
                              : ProcessConsoleExecHookManager::Callback{};
                ProcessConsoleExecHookManager::Callback post_callback =
                        post ? ProcessConsoleExecHookManager::Callback{
                                       [owner, callback_reference](
                                               const ProcessConsoleExecInvocation& invocation) {
                                           return owner->invoke_process_console_exec_hook_callback(
                                                   callback_reference, invocation);
                                       }}
                             : ProcessConsoleExecHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "ProcessConsoleExec hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_process_console_exec_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_call_function_by_name_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_call_function_by_name_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterCallFunctionByNameWithArguments hook is unavailable because the validated native manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post
                                ? "RegisterCallFunctionByNameWithArgumentsPostHook requires a callback"
                                : "RegisterCallFunctionByNameWithArgumentsPreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                CallFunctionByNameWithArgumentsHookManager::Callback pre_callback =
                        !post ? CallFunctionByNameWithArgumentsHookManager::Callback{
                                        [owner, callback_reference](
                                                const CallFunctionByNameWithArgumentsInvocation& invocation) {
                                            return owner->invoke_call_function_by_name_hook_callback(
                                                    callback_reference, invocation);
                                        }}
                              : CallFunctionByNameWithArgumentsHookManager::Callback{};
                CallFunctionByNameWithArgumentsHookManager::Callback post_callback =
                        post ? CallFunctionByNameWithArgumentsHookManager::Callback{
                                       [owner, callback_reference](
                                               const CallFunctionByNameWithArgumentsInvocation& invocation) {
                                           return owner->invoke_call_function_by_name_hook_callback(
                                                   callback_reference, invocation);
                                       }}
                             : CallFunctionByNameWithArgumentsHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "CallFunctionByNameWithArguments hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_call_function_by_name_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_local_player_exec_hook(lua_State* state, bool post) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_local_player_exec_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterULocalPlayerExec hook is unavailable because the validated native manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TFUNCTION)
            {
                error = post ? "RegisterULocalPlayerExecPostHook requires a callback"
                             : "RegisterULocalPlayerExecPreHook requires a callback";
            }
            else
            {
                lua_pushvalue(state, 1);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                LocalPlayerExecHookManager::Callback pre_callback =
                        !post ? LocalPlayerExecHookManager::Callback{
                                        [owner, callback_reference](
                                                const LocalPlayerExecInvocation& invocation) {
                                            return owner->invoke_local_player_exec_hook_callback(
                                                    callback_reference, invocation);
                                        }}
                              : LocalPlayerExecHookManager::Callback{};
                LocalPlayerExecHookManager::Callback post_callback =
                        post ? LocalPlayerExecHookManager::Callback{
                                       [owner, callback_reference](
                                               const LocalPlayerExecInvocation& invocation) {
                                           return owner->invoke_local_player_exec_hook_callback(
                                                   callback_reference, invocation);
                                       }}
                             : LocalPlayerExecHookManager::Callback{};
                const auto ids = hooks->register_hook(
                        std::move(pre_callback), std::move(post_callback), error);
                if (!error.empty() || (ids.first == 0u && ids.second == 0u))
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "ULocalPlayer::Exec hook registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_local_player_exec_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_console_command(lua_State* state, bool global) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_process_console_exec_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            std::string command_name;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterConsoleCommand is unavailable because the validated native UObject::ProcessConsoleExec manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TSTRING || lua_type(state, 2) != LUA_TFUNCTION)
            {
                error = global
                                ? "RegisterConsoleCommandGlobalHandler requires a command name and callback"
                                : "RegisterConsoleCommandHandler requires a command name and callback";
            }
            else
            {
                std::size_t command_length{};
                const char* command = lua_tolstring(state, 1, &command_length);
                command_name.assign(command, command_length);
                if (command_name.empty() ||
                    std::any_of(command_name.begin(), command_name.end(), [](const char byte) {
                        return std::isspace(static_cast<unsigned char>(byte)) != 0;
                    }))
                {
                    error = "console command name must be one non-empty token";
                }
            }
            if (error.empty())
            {
                lua_pushvalue(state, 2);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                ProcessConsoleExecHookManager::Callback post_callback{
                        [owner,
                         callback_reference,
                         command_name = std::move(command_name),
                         global](const ProcessConsoleExecInvocation& invocation)
                                -> std::optional<bool> {
                            if (invocation.command_parts.empty() ||
                                invocation.command_parts.front() != command_name)
                            {
                                return std::nullopt;
                            }
                            return owner->invoke_console_command_callback(
                                    callback_reference, invocation, !global);
                        }};
                const auto ids = hooks->register_hook({}, std::move(post_callback), error);
                if (!error.empty() || ids.second == 0u)
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    if (error.empty())
                    {
                        error = "console command handler registration returned no callback id";
                    }
                }
                else
                {
                    owner->m_process_console_exec_references.push_back(
                            {ids.first, ids.second, callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int register_lua_custom_event(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_blueprint_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            std::string event_name;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "RegisterCustomEvent is unavailable because the validated ProcessLocalScriptFunction manager is not active";
            }
            else if (lua_gettop(state) != 2 || lua_type(state, 1) != LUA_TSTRING ||
                     lua_type(state, 2) != LUA_TFUNCTION)
            {
                error = "RegisterCustomEvent requires an event name and Lua callback";
            }
            else
            {
                std::size_t event_name_length{};
                const char* event_name_data =
                        lua_tolstring(state, 1, &event_name_length);
                event_name.assign(event_name_data, event_name_length);
                if (event_name.empty() ||
                    std::memchr(event_name.data(), '\0', event_name.size()) != nullptr)
                {
                    error = "RegisterCustomEvent requires a non-empty event name without NUL bytes";
                }
            }
            if (error.empty())
            {
                lua_pushvalue(state, 2);
                callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                const std::uint64_t callback_id = hooks->register_named_hook(
                        event_name,
                        [owner, callback_reference](
                                const UFunctionHookInvocation& invocation) {
                            owner->invoke_custom_event_callback(
                                    callback_reference, invocation);
                        },
                        error);
                if (callback_id == 0u || !error.empty())
                {
                    luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                }
                else
                {
                    owner->m_custom_event_references.push_back(
                            {ascii_lower_copy(event_name),
                             callback_id,
                             callback_reference});
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int unregister_lua_custom_event(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* hooks = get_blueprint_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            std::string event_name;
            if (hooks == nullptr || owner == nullptr || !hooks->is_ready())
            {
                error = "UnregisterCustomEvent is unavailable because the validated ProcessLocalScriptFunction manager is not active";
            }
            else if (lua_gettop(state) != 1 || lua_type(state, 1) != LUA_TSTRING)
            {
                error = "UnregisterCustomEvent requires an event name";
            }
            else
            {
                std::size_t event_name_length{};
                const char* event_name_data =
                        lua_tolstring(state, 1, &event_name_length);
                event_name.assign(event_name_data, event_name_length);
                event_name = ascii_lower_copy(event_name);
                if (event_name.empty())
                {
                    error = "UnregisterCustomEvent requires a non-empty event name";
                }
            }
            if (error.empty())
            {
                bool removed{};
                for (auto iterator = owner->m_custom_event_references.begin();
                     iterator != owner->m_custom_event_references.end();)
                {
                    if (iterator->event_name != event_name)
                    {
                        ++iterator;
                        continue;
                    }
                    std::string unregister_error;
                    if (!hooks->unregister_named_hook(
                                iterator->callback_id, unregister_error))
                    {
                        error = unregister_error.empty()
                                        ? "custom event callback could not be unregistered"
                                        : std::move(unregister_error);
                        break;
                    }
                    luaL_unref(
                            state,
                            LUA_REGISTRYINDEX,
                            iterator->callback_reference);
                    iterator = owner->m_custom_event_references.erase(iterator);
                    removed = true;
                }
                if (error.empty() && !removed)
                {
                    // Upstream treats an unknown name as an idempotent no-op.
                    return 0;
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int request_lua_mod_lifecycle(
            lua_State* state,
            LuaModLifecycleAction action,
            bool current_mod) noexcept
    {
        auto* owner = get_state_owner(state);
        if (owner == nullptr)
        {
            return luaL_error(
                    state, "mod lifecycle controller is unavailable");
        }

        std::optional<std::string> target_storage;
        std::optional<std::string_view> target;
        if (!current_mod)
        {
            if (lua_type(state, 1) != LUA_TSTRING)
            {
                return luaL_error(
                        state,
                        "%s requires a mod name string",
                        action == LuaModLifecycleAction::restart ? "RestartMod"
                                                                  : "UninstallMod");
            }
            std::size_t target_length{};
            const char* target_data =
                    lua_tolstring(state, 1, &target_length);
            target_storage.emplace(target_data, target_length);
            if (target_storage->empty() ||
                std::memchr(
                        target_storage->data(), '\0', target_storage->size()) !=
                        nullptr)
            {
                return luaL_error(state, "mod name must be non-empty and contain no NUL bytes");
            }
            target = *target_storage;
        }
        std::string error;
        if (!owner->request_mod_lifecycle(action, target, error))
        {
            return luaL_error(
                    state,
                    "%s",
                    error.empty() ? "mod lifecycle request could not be queued"
                                  : error.c_str());
        }
        return 0;
    }

    int lua_restart_current_mod(lua_State* state) noexcept
    {
        return request_lua_mod_lifecycle(
                state, LuaModLifecycleAction::restart, true);
    }

    int lua_uninstall_current_mod(lua_State* state) noexcept
    {
        return request_lua_mod_lifecycle(
                state, LuaModLifecycleAction::uninstall, true);
    }

    int lua_restart_mod(lua_State* state) noexcept
    {
        return request_lua_mod_lifecycle(
                state, LuaModLifecycleAction::restart, false);
    }

    int lua_uninstall_mod(lua_State* state) noexcept
    {
        return request_lua_mod_lifecycle(
                state, LuaModLifecycleAction::uninstall, false);
    }

    int register_lua_ufunction_hook(lua_State* state) noexcept
    {
        bool failed{};
        std::uint64_t pre_id{};
        std::uint64_t post_id{};
        {
            auto* runtime = get_runtime(state);
            auto* hooks = get_hooks(state);
            auto* blueprint_hooks = get_blueprint_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int pre_reference = LUA_NOREF;
            int post_reference = LUA_NOREF;
            if (runtime == nullptr || owner == nullptr ||
                ((hooks == nullptr || !hooks->is_ready()) &&
                 (blueprint_hooks == nullptr || !blueprint_hooks->is_ready())))
            {
                error = "RegisterHook is unavailable because the validated UFunction hook manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TSTRING || lua_type(state, 2) != LUA_TFUNCTION ||
                     (!lua_isnoneornil(state, 3) && lua_type(state, 3) != LUA_TFUNCTION))
            {
                error = "RegisterHook requires a UFunction path, pre-callback, and optional post-callback";
            }
            else
            {
                std::size_t path_length{};
                const char* path_data = lua_tolstring(state, 1, &path_length);
                std::string_view path{path_data, path_length};
                constexpr std::string_view prefix{"Function "};
                if (path.starts_with(prefix))
                {
                    path.remove_prefix(prefix.size());
                }
                const auto found = runtime->find_by_path(path, 1u);
                ReadOnlyUObjectHandle function;
                if (!found.success || found.objects.empty())
                {
                    error = "RegisterHook could not find UFunction '" + std::string{path} + "'";
                }
                else
                {
                    function = found.objects.front();
                    std::string class_error;
                    if (!runtime->is_a(function, "Function", class_error))
                    {
                        error = class_error.empty() ? "RegisterHook target is not a UFunction" : class_error;
                    }
                }
                if (error.empty())
                {
                    lua_pushvalue(state, 2);
                    pre_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                    if (lua_type(state, 3) == LUA_TFUNCTION)
                    {
                        lua_pushvalue(state, 3);
                        post_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                    }
                    UFunctionHookManager::Callback pre_callback{
                            [owner, pre_reference](const UFunctionHookInvocation& invocation) {
                                owner->invoke_ufunction_hook_callback(pre_reference, invocation, false);
                            }};
                    UFunctionHookManager::Callback post_callback =
                            post_reference != LUA_NOREF
                                    ? UFunctionHookManager::Callback{[owner, post_reference](const UFunctionHookInvocation& invocation) {
                                          owner->invoke_ufunction_hook_callback(post_reference, invocation, true);
                                      }}
                                    : UFunctionHookManager::Callback{};
                    const bool blueprint = blueprint_hooks != nullptr && blueprint_hooks->is_script_function(function);
                    std::pair<std::uint64_t, std::uint64_t> ids;
                    std::pair<std::uint64_t, std::uint64_t> blueprint_ids;
                    if (blueprint)
                    {
                        blueprint_ids = blueprint_hooks->register_hook(
                                function, pre_callback, post_callback, error);
                    }
                    if (error.empty() && hooks != nullptr && hooks->is_ready())
                    {
                        ids = hooks->register_hook(
                                function, std::move(pre_callback), std::move(post_callback), error);
                    }
                    else if (error.empty())
                    {
                        error = "native UFunction hook manager is unavailable for the requested function";
                    }
                    if ((!error.empty() || ids.first == 0u) && blueprint_ids.first != 0u)
                    {
                        std::string ignored;
                        static_cast<void>(blueprint_hooks->unregister_hook(
                                function, blueprint_ids.first, blueprint_ids.second, ignored));
                        blueprint_ids = {};
                    }
                    if (!error.empty() || ids.first == 0u)
                    {
                        if (error.empty())
                        {
                            error = "UFunction hook registration returned no callback id";
                        }
                        ids = {};
                    }
                    pre_id = ids.first;
                    post_id = ids.second;
                    if (pre_id == 0u || !error.empty())
                    {
                        luaL_unref(state, LUA_REGISTRYINDEX, pre_reference);
                        if (post_reference != LUA_NOREF)
                        {
                            luaL_unref(state, LUA_REGISTRYINDEX, post_reference);
                        }
                    }
                    else
                    {
                        owner->m_hook_references.push_back(
                                {function,
                                 blueprint,
                                 pre_id,
                                 post_id,
                                 blueprint_ids.first,
                                 blueprint_ids.second,
                                 pre_reference,
                                 post_reference});
                    }
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        if (failed)
        {
            return lua_error(state);
        }
        lua_pushinteger(state, static_cast<lua_Integer>(pre_id));
        lua_pushinteger(state, static_cast<lua_Integer>(post_id));
        return 2;
    }

    int unregister_lua_ufunction_hook(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* runtime = get_runtime(state);
            auto* hooks = get_hooks(state);
            auto* blueprint_hooks = get_blueprint_hooks(state);
            auto* owner = get_state_owner(state);
            std::string error;
            if (runtime == nullptr || owner == nullptr || lua_type(state, 1) != LUA_TSTRING ||
                !lua_isinteger(state, 2) || !lua_isinteger(state, 3) || lua_tointeger(state, 2) < 0 ||
                lua_tointeger(state, 3) < 0)
            {
                error = "UnregisterHook requires a UFunction path and non-negative pre/post callback ids";
            }
            else
            {
                std::size_t path_length{};
                const char* path_data = lua_tolstring(state, 1, &path_length);
                std::string_view path{path_data, path_length};
                constexpr std::string_view prefix{"Function "};
                if (path.starts_with(prefix))
                {
                    path.remove_prefix(prefix.size());
                }
                const auto found = runtime->find_by_path(path, 1u);
                if (!found.success || found.objects.empty())
                {
                    error = "UnregisterHook could not find UFunction '" + std::string{path} + "'";
                }
                else
                {
                    const std::uint64_t pre_id = static_cast<std::uint64_t>(lua_tointeger(state, 2));
                    const std::uint64_t post_id = static_cast<std::uint64_t>(lua_tointeger(state, 3));
                    const auto references = std::find_if(
                            owner->m_hook_references.begin(),
                            owner->m_hook_references.end(),
                            [function_address = found.objects.front().address, pre_id, post_id](
                                    const HeadlessLuaState::HookReferences& candidate) {
                                return candidate.function.address == function_address &&
                                       candidate.pre_id == pre_id && candidate.post_id == post_id;
                            });
                    if (references == owner->m_hook_references.end())
                    {
                        error = "UnregisterHook callback ids were not found for the requested UFunction";
                    }
                    else
                    {
                        const bool native_unregistered =
                                hooks != nullptr && hooks->unregister_hook(
                                                            references->function, pre_id, post_id, error);
                        bool blueprint_unregistered = true;
                        if (native_unregistered && references->blueprint)
                        {
                            blueprint_unregistered =
                                    blueprint_hooks != nullptr && blueprint_hooks->unregister_hook(
                                                                          references->function,
                                                                          references->blueprint_pre_id,
                                                                          references->blueprint_post_id,
                                                                          error);
                        }
                        const bool unregistered = native_unregistered && blueprint_unregistered;
                        if (unregistered)
                        {
                            luaL_unref(state, LUA_REGISTRYINDEX, references->pre_reference);
                            if (references->post_reference != LUA_NOREF)
                            {
                                luaL_unref(state, LUA_REGISTRYINDEX, references->post_reference);
                            }
                            owner->m_hook_references.erase(references);
                        }
                        else if (error.empty())
                        {
                            error = "the selected UFunction hook manager is unavailable";
                        }
                    }
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    int notify_on_new_object(lua_State* state) noexcept
    {
        bool failed{};
        {
            auto* runtime = get_runtime(state);
            auto* notifications = get_notifications(state);
            auto* owner = get_state_owner(state);
            std::string error;
            int callback_reference = LUA_NOREF;
            if (runtime == nullptr || notifications == nullptr || owner == nullptr || !notifications->is_ready())
            {
                error = "NotifyOnNewObject is unavailable because the validated object notification manager is not active";
            }
            else if (lua_type(state, 1) != LUA_TSTRING || lua_type(state, 2) != LUA_TFUNCTION)
            {
                error = "NotifyOnNewObject requires a UClass path and callback";
            }
            else
            {
                std::size_t path_length{};
                const char* path_data = lua_tolstring(state, 1, &path_length);
                std::string_view path{path_data, path_length};
                constexpr std::string_view prefix{"Class "};
                if (path.starts_with(prefix))
                {
                    path.remove_prefix(prefix.size());
                }
                const auto found = runtime->find_by_path(path, 1u);
                ReadOnlyUObjectHandle requested_class;
                if (!found.success || found.objects.empty())
                {
                    error = "NotifyOnNewObject could not find UClass '" + std::string{path} + "'";
                }
                else
                {
                    requested_class = found.objects.front();
                    std::string class_error;
                    if (!runtime->is_a(requested_class, "Class", class_error))
                    {
                        error = class_error.empty() ? "NotifyOnNewObject target is not a UClass" : class_error;
                    }
                }
                if (error.empty())
                {
                    lua_pushvalue(state, 2);
                    callback_reference = luaL_ref(state, LUA_REGISTRYINDEX);
                    const std::uint64_t id = notifications->register_notification(
                            requested_class,
                            [owner, callback_reference](const ReadOnlyUObjectHandle& object) {
                                return owner->invoke_new_object_callback(callback_reference, object);
                            },
                            error);
                    if (id == 0u || !error.empty())
                    {
                        luaL_unref(state, LUA_REGISTRYINDEX, callback_reference);
                    }
                    else
                    {
                        owner->m_notification_references.push_back({id, callback_reference});
                    }
                }
            }
            if (!error.empty())
            {
                lua_pushlstring(state, error.data(), error.size());
                failed = true;
            }
        }
        return failed ? lua_error(state) : 0;
    }

    HeadlessLuaState::HeadlessLuaState() noexcept
        : m_state{luaL_newstate()}, m_main_thread_id{std::this_thread::get_id()}
    {
        if (m_state != nullptr)
        {
            luaL_openlibs(m_state);
        }
    }

    HeadlessLuaState::~HeadlessLuaState()
    {
        stop_async_worker();
        std::lock_guard lock{m_mutex};
        if (m_scheduler != nullptr)
        {
            static_cast<void>(m_scheduler->clear_owner(
                    reinterpret_cast<GameThreadScheduler::OwnerId>(this)));
            m_scheduler = nullptr;
        }
        if (m_blueprint_hook_manager != nullptr)
        {
            for (const auto& references : m_custom_event_references)
            {
                std::string ignored;
                static_cast<void>(m_blueprint_hook_manager->unregister_named_hook(
                        references.callback_id, ignored));
                if (m_state != nullptr &&
                    references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(
                            m_state,
                            LUA_REGISTRYINDEX,
                            references.callback_reference);
                }
            }
        }
        m_custom_event_references.clear();
        if (m_hook_manager != nullptr || m_blueprint_hook_manager != nullptr)
        {
            for (const auto& references : m_hook_references)
            {
                std::string ignored;
                if (m_hook_manager != nullptr)
                {
                    static_cast<void>(m_hook_manager->unregister_hook(
                            references.function, references.pre_id, references.post_id, ignored));
                }
                if (references.blueprint && m_blueprint_hook_manager != nullptr)
                {
                    static_cast<void>(m_blueprint_hook_manager->unregister_hook(
                            references.function,
                            references.blueprint_pre_id,
                            references.blueprint_post_id,
                            ignored));
                }
            }
        }
        m_hook_references.clear();
        if (m_begin_play_hook_manager != nullptr)
        {
            for (const auto& references : m_begin_play_references)
            {
                std::string ignored;
                static_cast<void>(m_begin_play_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_begin_play_references.clear();
        m_begin_play_hook_manager = nullptr;
        if (m_end_play_hook_manager != nullptr)
        {
            for (const auto& references : m_end_play_references)
            {
                std::string ignored;
                static_cast<void>(m_end_play_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_end_play_references.clear();
        m_end_play_hook_manager = nullptr;
        if (m_init_game_state_hook_manager != nullptr)
        {
            for (const auto& references : m_init_game_state_references)
            {
                std::string ignored;
                static_cast<void>(m_init_game_state_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_init_game_state_references.clear();
        m_init_game_state_hook_manager = nullptr;
        if (m_load_map_hook_manager != nullptr)
        {
            for (const auto& references : m_load_map_references)
            {
                std::string ignored;
                static_cast<void>(m_load_map_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_load_map_references.clear();
        m_load_map_hook_manager = nullptr;
        if (m_process_console_exec_hook_manager != nullptr)
        {
            for (const auto& references : m_process_console_exec_references)
            {
                std::string ignored;
                static_cast<void>(m_process_console_exec_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_process_console_exec_references.clear();
        m_process_console_exec_hook_manager = nullptr;
        if (m_call_function_by_name_hook_manager != nullptr)
        {
            for (const auto& references : m_call_function_by_name_references)
            {
                std::string ignored;
                static_cast<void>(m_call_function_by_name_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_call_function_by_name_references.clear();
        m_call_function_by_name_hook_manager = nullptr;
        if (m_local_player_exec_hook_manager != nullptr)
        {
            for (const auto& references : m_local_player_exec_references)
            {
                std::string ignored;
                static_cast<void>(m_local_player_exec_hook_manager->unregister_hook(
                        references.pre_id, references.post_id, ignored));
                if (m_state != nullptr && references.callback_reference != LUA_NOREF)
                {
                    luaL_unref(m_state, LUA_REGISTRYINDEX, references.callback_reference);
                }
            }
        }
        m_local_player_exec_references.clear();
        m_local_player_exec_hook_manager = nullptr;
        if (m_notification_manager != nullptr)
        {
            for (const auto& references : m_notification_references)
            {
                static_cast<void>(m_notification_manager->unregister_notification(references.id));
            }
        }
        m_notification_references.clear();
        if (m_state != nullptr)
        {
            if (m_async_state_reference >= 0)
            {
                luaL_unref(m_state, LUA_REGISTRYINDEX, m_async_state_reference);
                m_async_state_reference = LUA_NOREF;
                m_async_state = nullptr;
            }
            lua_close(m_state);
            m_state = nullptr;
        }
    }

    bool HeadlessLuaState::is_ready() const noexcept
    {
        return m_state != nullptr;
    }

    std::string HeadlessLuaState::version() const
    {
        return is_ready() ? LUA_VERSION : "unavailable";
    }

    void HeadlessLuaState::configure_mod_lifecycle(
            std::string mod_name,
            LuaModLifecycleHandler handler) noexcept
    {
        try
        {
            std::lock_guard lock{m_mutex};
            const bool available =
                    m_state != nullptr && !mod_name.empty() &&
                    static_cast<bool>(handler);
            if (available)
            {
                m_mod_name = std::move(mod_name);
                m_mod_lifecycle_handler = std::move(handler);
            }
            else
            {
                m_mod_name.clear();
                m_mod_lifecycle_handler = {};
            }
            if (m_state != nullptr)
            {
                lua_pushboolean(m_state, available ? 1 : 0);
                lua_setglobal(m_state, "ModLifecycleAvailable");
            }
        }
        catch (...)
        {
            m_mod_name.clear();
            m_mod_lifecycle_handler = {};
        }
    }

    bool HeadlessLuaState::request_mod_lifecycle(
            LuaModLifecycleAction action,
            std::optional<std::string_view> target_mod,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            LuaModLifecycleHandler handler;
            std::string target;
            {
                std::lock_guard lock{m_mutex};
                if (!m_mod_lifecycle_handler || m_mod_name.empty())
                {
                    error = "mod lifecycle controller is unavailable for this Lua state";
                    return false;
                }
                target = target_mod.has_value()
                        ? std::string{*target_mod}
                        : m_mod_name;
                handler = m_mod_lifecycle_handler;
            }
            if (target.empty())
            {
                error = "mod lifecycle target name is empty";
                return false;
            }
            if (!handler(action, target, error) && error.empty())
            {
                error = "mod lifecycle controller rejected the request";
                return false;
            }
            return error.empty();
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

    LuaExecutionResult HeadlessLuaState::install_readonly_unreal_bindings(ReadOnlyUnrealRuntime& runtime,
                                                                           GameThreadScheduler* scheduler,
                                                                           UFunctionHookManager* hooks,
                                                                           ObjectNotificationManager* notifications,
                                                                           BlueprintHookManager* blueprint_hooks,
                                                                           BeginPlayHookManager* begin_play_hooks,
                                                                           EndPlayHookManager* end_play_hooks,
                                                                           InitGameStateHookManager* init_game_state_hooks,
                                                                           LoadMapHookManager* load_map_hooks,
                                                                           ProcessConsoleExecHookManager* process_console_exec_hooks,
                                                                           CallFunctionByNameWithArgumentsHookManager* call_function_by_name_hooks,
                                                                           LocalPlayerExecHookManager* local_player_exec_hooks,
                                                                           std::filesystem::path game_executable_directory,
                                                                           std::filesystem::path ue4ss_root_directory) noexcept
    {
        try
        {
            std::lock_guard lock{m_mutex};
            if (!is_ready() || !runtime.is_ready())
            {
                return {false, "Lua VM and validated read-only Unreal runtime are required"};
            }

            lua_pushlightuserdata(m_state, &runtime);
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_runtime_registry_key);
            lua_pushlightuserdata(m_state, this);
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_state_owner_registry_key);
            if (!game_executable_directory.empty())
            {
                if (!game_executable_directory.is_absolute())
                {
                    return {false, "game executable directory must be absolute"};
                }
                const std::string path =
                        game_executable_directory.lexically_normal().string();
                lua_pushlstring(m_state, path.data(), path.size());
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(
                    m_state,
                    LUA_REGISTRYINDEX,
                    k_game_executable_directory_registry_key);
            if (!ue4ss_root_directory.empty())
            {
                if (!ue4ss_root_directory.is_absolute())
                {
                    return {false, "UE4SS root directory must be absolute"};
                }
                const std::string path =
                        ue4ss_root_directory.lexically_normal().string();
                lua_pushlstring(m_state, path.data(), path.size());
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(
                    m_state,
                    LUA_REGISTRYINDEX,
                    k_ue4ss_root_directory_registry_key);
            if (scheduler != nullptr && scheduler->is_ready())
            {
                lua_pushlightuserdata(m_state, scheduler);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_scheduler_registry_key);
            m_scheduler = scheduler != nullptr && scheduler->is_ready() ? scheduler : nullptr;
            if (hooks != nullptr && hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_hooks_registry_key);
            m_hook_manager = hooks != nullptr && hooks->is_ready() ? hooks : nullptr;
            if (blueprint_hooks != nullptr && blueprint_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, blueprint_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_blueprint_hooks_registry_key);
            m_blueprint_hook_manager =
                    blueprint_hooks != nullptr && blueprint_hooks->is_ready() ? blueprint_hooks : nullptr;
            if (begin_play_hooks != nullptr && begin_play_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, begin_play_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_begin_play_hooks_registry_key);
            m_begin_play_hook_manager =
                    begin_play_hooks != nullptr && begin_play_hooks->is_ready() ? begin_play_hooks : nullptr;
            if (end_play_hooks != nullptr && end_play_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, end_play_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_end_play_hooks_registry_key);
            m_end_play_hook_manager =
                    end_play_hooks != nullptr && end_play_hooks->is_ready() ? end_play_hooks : nullptr;
            if (init_game_state_hooks != nullptr && init_game_state_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, init_game_state_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_init_game_state_hooks_registry_key);
            m_init_game_state_hook_manager =
                    init_game_state_hooks != nullptr && init_game_state_hooks->is_ready()
                            ? init_game_state_hooks
                            : nullptr;
            if (load_map_hooks != nullptr && load_map_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, load_map_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_load_map_hooks_registry_key);
            m_load_map_hook_manager =
                    load_map_hooks != nullptr && load_map_hooks->is_ready() ? load_map_hooks : nullptr;
            if (process_console_exec_hooks != nullptr && process_console_exec_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, process_console_exec_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(
                    m_state,
                    LUA_REGISTRYINDEX,
                    k_process_console_exec_hooks_registry_key);
            m_process_console_exec_hook_manager =
                    process_console_exec_hooks != nullptr && process_console_exec_hooks->is_ready()
                            ? process_console_exec_hooks
                            : nullptr;
            if (call_function_by_name_hooks != nullptr &&
                call_function_by_name_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, call_function_by_name_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(
                    m_state,
                    LUA_REGISTRYINDEX,
                    k_call_function_by_name_hooks_registry_key);
            m_call_function_by_name_hook_manager =
                    call_function_by_name_hooks != nullptr &&
                                    call_function_by_name_hooks->is_ready()
                            ? call_function_by_name_hooks
                            : nullptr;
            if (local_player_exec_hooks != nullptr &&
                local_player_exec_hooks->is_ready())
            {
                lua_pushlightuserdata(m_state, local_player_exec_hooks);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(
                    m_state,
                    LUA_REGISTRYINDEX,
                    k_local_player_exec_hooks_registry_key);
            m_local_player_exec_hook_manager =
                    local_player_exec_hooks != nullptr &&
                                    local_player_exec_hooks->is_ready()
                            ? local_player_exec_hooks
                            : nullptr;
            if (notifications != nullptr && notifications->is_ready())
            {
                lua_pushlightuserdata(m_state, notifications);
            }
            else
            {
                lua_pushnil(m_state);
            }
            lua_setfield(m_state, LUA_REGISTRYINDEX, k_notifications_registry_key);
            m_notification_manager = notifications != nullptr && notifications->is_ready() ? notifications : nullptr;

            if (luaL_newmetatable(m_state, k_fname_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "ToString", lua_fname_to_string);
                register_method(m_state, "GetComparisonIndex", lua_fname_get_comparison_index);
                register_method(m_state, "Equals", lua_fname_equals);
                register_method(m_state, "type", lua_fname_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__eq", lua_fname_equals);
                register_method(m_state, "__tostring", lua_fname_to_string);
                register_method(m_state, "__gc", lua_fname_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_thread_id_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "ToString", lua_thread_id_to_string);
                register_method(m_state, "type", lua_thread_id_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__eq", lua_thread_id_equal);
                register_method(m_state, "__gc", lua_thread_id_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_fstring_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "ToString", lua_fstring_to_string);
                register_method(m_state, "Empty", lua_fstring_empty);
                register_method(m_state, "Clear", lua_fstring_empty);
                register_method(m_state, "Len", lua_fstring_len);
                register_method(m_state, "IsEmpty", lua_fstring_is_empty);
                register_method(m_state, "Append", lua_fstring_append);
                register_method(m_state, "Find", lua_fstring_find);
                register_method(m_state, "StartsWith", lua_fstring_starts_with);
                register_method(m_state, "EndsWith", lua_fstring_ends_with);
                register_method(m_state, "ToUpper", lua_fstring_to_upper);
                register_method(m_state, "ToLower", lua_fstring_to_lower);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__tostring", lua_fstring_to_string);
                register_method(m_state, "__gc", lua_fstring_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_furl_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "type", lua_furl_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__gc", lua_furl_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_output_device_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "Log", lua_output_device_log);
                register_method(m_state, "type", lua_output_device_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__gc", lua_output_device_gc);
            }
            lua_pop(m_state, 1);

            const auto install_narrow_string_metatable = [this](const char* metatable) {
                if (luaL_newmetatable(m_state, metatable) != 0)
                {
                    lua_newtable(m_state);
                    register_method(m_state, "ToString", lua_narrow_string_to_string);
                    register_method(m_state, "Empty", lua_narrow_string_empty);
                    register_method(m_state, "Clear", lua_narrow_string_empty);
                    register_method(m_state, "Len", lua_narrow_string_len);
                    register_method(m_state, "IsEmpty", lua_narrow_string_is_empty);
                    register_method(m_state, "Append", lua_narrow_string_append);
                    register_method(m_state, "Find", lua_narrow_string_find);
                    register_method(m_state, "StartsWith", lua_narrow_string_starts_with);
                    register_method(m_state, "EndsWith", lua_narrow_string_ends_with);
                    register_method(m_state, "ToUpper", lua_narrow_string_to_upper);
                    register_method(m_state, "ToLower", lua_narrow_string_to_lower);
                    register_method(m_state, "type", lua_narrow_string_type);
                    lua_setfield(m_state, -2, "__index");
                    register_method(m_state, "__gc", lua_narrow_string_gc);
                }
                lua_pop(m_state, 1);
            };
            install_narrow_string_metatable(k_futf8string_metatable);
            install_narrow_string_metatable(k_fansistring_metatable);

            if (luaL_newmetatable(m_state, k_ftext_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "ToString", lua_ftext_to_string);
                register_method(m_state, "type", lua_ftext_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__eq", lua_ftext_equal);
                register_method(m_state, "__tostring", lua_ftext_to_string);
                register_method(m_state, "__gc", lua_ftext_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_tarray_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetArrayAddress", lua_tarray_get_unavailable_address);
                register_method(m_state, "GetArrayNum", lua_tarray_get_num);
                register_method(m_state, "GetArrayMax", lua_tarray_get_max);
                register_method(m_state, "GetArrayDataAddress", lua_tarray_get_unavailable_address);
                register_method(m_state, "Empty", lua_tarray_empty);
                register_method(m_state, "ForEach", lua_tarray_for_each);
                register_method(m_state, "IsValid", lua_tarray_is_valid);
                register_method(m_state, "type", lua_tarray_type);
                lua_pushvalue(m_state, -1);
                lua_setfield(m_state, -3, "__methods");
                lua_pop(m_state, 1);
                register_method(m_state, "__index", lua_tarray_index);
                register_method(m_state, "__newindex", lua_tarray_newindex);
                register_method(m_state, "__len", lua_tarray_len);
                register_method(m_state, "__gc", lua_tarray_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_tset_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "IsValid", lua_tset_is_valid);
                register_method(m_state, "Contains", lua_tset_contains);
                register_method(m_state, "ForEach", lua_tset_for_each);
                register_method(m_state, "Add", lua_tset_add);
                register_method(m_state, "Remove", lua_tset_remove);
                register_method(m_state, "Empty", lua_tset_empty);
                register_method(m_state, "type", lua_tset_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__len", lua_tset_len);
                register_method(m_state, "__gc", lua_tset_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_tmap_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "IsValid", lua_tmap_is_valid);
                register_method(m_state, "Find", lua_tmap_find);
                register_method(m_state, "Contains", lua_tmap_contains);
                register_method(m_state, "ForEach", lua_tmap_for_each);
                register_method(m_state, "Add", lua_tmap_add);
                register_method(m_state, "Remove", lua_tmap_remove);
                register_method(m_state, "Empty", lua_tmap_empty);
                register_method(m_state, "type", lua_tmap_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__len", lua_tmap_len);
                register_method(m_state, "__gc", lua_tmap_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_local_value_param_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "get", lua_local_value_param_get);
                register_method(m_state, "set", lua_local_value_param_set);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__gc", lua_local_value_param_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_ffield_class_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetName", lua_ffield_class_get_name);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__tostring", lua_ffield_class_get_name);
                register_method(m_state, "__gc", lua_ffield_class_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_uscript_struct_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetBaseAddress", lua_uscript_struct_get_base_address_unavailable);
                register_method(m_state, "GetStructAddress", lua_uscript_struct_get_struct_address);
                register_method(m_state, "GetPropertyAddress", lua_uscript_struct_get_property_address);
                register_method(m_state, "GetProperty", lua_uscript_struct_get_property);
                register_method(m_state, "GetStruct", lua_uscript_struct_get_struct);
                register_method(m_state, "IsValid", lua_uscript_struct_is_valid);
                register_method(m_state, "IsMappedToObject", lua_uscript_struct_is_mapped_to_object);
                register_method(m_state, "IsMappedToProperty", lua_uscript_struct_is_mapped_to_property);
                register_method(m_state, "type", lua_uscript_struct_type);
                lua_pushvalue(m_state, -1);
                lua_setfield(m_state, -3, "__methods");
                lua_pop(m_state, 1);
                register_method(m_state, "__index", lua_uscript_struct_index);
                register_method(m_state, "__newindex", lua_uscript_struct_newindex);
                register_method(m_state, "__gc", lua_uscript_struct_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_fproperty_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetAddress", lua_fproperty_get_address);
                register_method(m_state, "IsValid", lua_fproperty_is_valid);
                register_method(m_state, "GetName", lua_fproperty_get_name);
                register_method(m_state, "GetFullName", lua_fproperty_get_full_name);
                register_method(m_state, "GetFName", lua_fproperty_get_fname);
                register_method(m_state, "GetClass", lua_fproperty_get_class);
                register_method(m_state, "GetOffset_Internal", lua_fproperty_get_offset);
                register_method(m_state, "GetSize", lua_fproperty_get_size);
                register_method(m_state, "GetByteMask", lua_fproperty_get_byte_mask);
                register_method(m_state, "GetByteOffset", lua_fproperty_get_byte_offset);
                register_method(m_state, "GetFieldMask", lua_fproperty_get_field_mask);
                register_method(m_state, "GetFieldSize", lua_fproperty_get_field_size);
                register_method(m_state, "GetInner", lua_fproperty_get_inner);
                register_method(m_state, "GetPropertyClass", lua_fproperty_get_property_class);
                register_method(m_state, "GetStruct", lua_fproperty_get_struct);
                register_method(m_state, "GetEnum", lua_fproperty_get_enum);
                register_method(m_state, "GetInterfaceClass", lua_fproperty_get_interface_class);
                register_method(m_state, "ContainerPtrToValuePtr", lua_fproperty_container_ptr_to_value_ptr);
                register_method(m_state, "ImportText", lua_fproperty_import_text);
                register_method(m_state, "IsA", lua_fproperty_is_a);
                register_method(m_state, "type", lua_fproperty_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__tostring", lua_fproperty_get_name);
                register_method(m_state, "__gc", lua_fproperty_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_weak_object_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "Get", lua_weak_object_get);
                register_method(m_state, "get", lua_weak_object_get);
                register_method(m_state, "type", lua_weak_object_type);
                lua_setfield(m_state, -2, "__index");
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_soft_object_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetWeakPtr", lua_soft_object_get_weak_ptr);
                register_method(m_state, "GetTagAtLastTest", lua_soft_object_get_tag);
                register_method(m_state, "GetObjectID", lua_soft_object_get_object_id);
                register_method(m_state, "type", lua_soft_object_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__gc", lua_soft_object_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_soft_object_path_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetAssetPathName", lua_soft_object_path_get_asset_name);
                register_method(m_state, "GetSubPathString", lua_soft_object_path_get_sub_path);
                register_method(m_state, "type", lua_soft_object_path_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__gc", lua_soft_object_path_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_multicast_delegate_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "Add", lua_multicast_delegate_add);
                register_method(m_state, "Remove", lua_multicast_delegate_remove);
                register_method(m_state, "Clear", lua_multicast_delegate_clear);
                register_method(m_state, "Broadcast", lua_multicast_delegate_broadcast);
                register_method(m_state, "GetBindings", lua_multicast_delegate_get_bindings);
                register_method(m_state, "type", lua_multicast_delegate_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__len", lua_multicast_delegate_len);
                register_method(m_state, "__gc", lua_multicast_delegate_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_sparse_multicast_delegate_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "Add", lua_multicast_delegate_add);
                register_method(m_state, "Remove", lua_multicast_delegate_remove);
                register_method(m_state, "Clear", lua_multicast_delegate_clear);
                register_method(m_state, "Broadcast", lua_multicast_delegate_broadcast);
                register_method(m_state, "GetBindings", lua_multicast_delegate_get_bindings);
                register_method(m_state, "type", lua_multicast_delegate_type);
                lua_setfield(m_state, -2, "__index");
                register_method(m_state, "__len", lua_multicast_delegate_len);
                register_method(m_state, "__gc", lua_multicast_delegate_gc);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_uobject_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "GetAddress", lua_uobject_get_address);
                register_method(m_state, "IsValid", lua_uobject_is_valid);
                register_method(m_state, "GetName", lua_uobject_get_name);
                register_method(m_state, "GetPathName", lua_uobject_get_path_name);
                register_method(m_state, "GetFullName", lua_uobject_get_full_name);
                register_method(m_state, "GetFName", lua_uobject_get_fname);
                register_method(m_state, "GetClass", lua_uobject_get_class);
                register_method(m_state, "GetOuter", lua_uobject_get_outer);
                register_method(m_state, "GetWorld", lua_uobject_get_world);
                register_method(m_state, "GetLevel", lua_aactor_get_level);
                register_method(m_state, "IsAnyClass", lua_uobject_is_class);
                register_method(m_state, "IsClass", lua_uobject_is_class);
                register_method(m_state, "HasAllFlags", lua_uobject_has_all_flags);
                register_method(m_state, "HasAnyFlags", lua_uobject_has_any_flags);
                register_method(m_state, "HasAnyInternalFlags", lua_uobject_has_any_internal_flags);
                register_method(m_state, "GetCDO", lua_uclass_get_cdo);
                register_method(m_state, "IsChildOf", lua_uclass_is_child_of);
                register_method(m_state, "IsA", lua_uobject_is_a);
                register_method(m_state, "ImplementsInterface", lua_uobject_implements_interface);
                register_method(m_state, "GetSuperStruct", lua_ustruct_get_super_struct);
                register_method(m_state, "ForEachFunction", lua_ustruct_for_each_function);
                register_method(m_state, "ForEachProperty", lua_ustruct_for_each_property);
                register_method(m_state, "Reflection", lua_uobject_reflection);
                register_method(m_state, "GetPropertyValue", lua_uobject_get_property_value);
                register_method(m_state, "SetPropertyValue", lua_uobject_newindex);
                register_method(m_state, "CallFunction", lua_uobject_call_function);
                register_method(m_state, "GetRowStruct", lua_udata_table_get_row_struct);
                register_method(m_state, "GetRowMap", lua_udata_table_get_row_map);
                register_method(m_state, "FindRow", lua_udata_table_find_row);
                register_method(m_state, "GetRowNames", lua_udata_table_get_row_names);
                register_method(m_state, "GetAllRows", lua_udata_table_get_all_rows);
                register_method(m_state, "ForEachRow", lua_udata_table_for_each_row);
                register_method(m_state, "AddRow", lua_udata_table_add_row);
                register_method(m_state, "RemoveRow", lua_udata_table_remove_row);
                register_method(m_state, "EmptyTable", lua_udata_table_empty);
                register_method(m_state, "GetFunctionFlags", lua_ufunction_get_flags);
                register_method(m_state, "SetFunctionFlags", lua_ufunction_set_flags);
                register_method(m_state, "GetNameByValue", lua_uenum_get_name_by_value);
                register_method(m_state, "GetEnumNameByIndex", lua_uenum_get_name_by_index);
                register_method(m_state, "ForEachName", lua_uenum_for_each_name);
                register_method(m_state, "NumEnums", lua_uenum_num_enums);
                register_method(m_state, "InsertIntoNames", lua_uenum_insert_into_names);
                register_method(m_state, "EditNameAt", lua_uenum_edit_name_at);
                register_method(m_state, "EditValueAt", lua_uenum_edit_value_at);
                register_method(m_state, "RemoveFromNamesAt", lua_uenum_remove_from_names_at);
                register_method(m_state, "type", lua_uobject_type);
                lua_pushvalue(m_state, -1);
                lua_setfield(m_state, -3, "__methods");
                lua_pop(m_state, 1);
                register_method(m_state, "__index", lua_uobject_index);
                register_method(m_state, "__newindex", lua_uobject_newindex);
                register_method(m_state, "__eq", lua_uobject_equal);
                register_method(m_state, "__tostring", lua_uobject_to_string);
                register_method(m_state, "__len", lua_udata_table_len);
            }
            lua_pop(m_state, 1);

            if (luaL_newmetatable(m_state, k_remote_param_metatable) != 0)
            {
                lua_newtable(m_state);
                register_method(m_state, "Get", lua_remote_param_get);
                register_method(m_state, "get", lua_remote_param_get);
                register_method(m_state, "Set", lua_remote_param_set);
                register_method(m_state, "set", lua_remote_param_set);
                register_method(m_state, "type", lua_remote_param_type);
                lua_setfield(m_state, -2, "__index");
            }
            lua_pop(m_state, 1);

            lua_pushcfunction(m_state, lua_find_first_of);
            lua_setglobal(m_state, "FindFirstOf");
            lua_pushcfunction(m_state, lua_find_all_of);
            lua_setglobal(m_state, "FindAllOf");
            lua_pushcfunction(m_state, lua_find_objects);
            lua_setglobal(m_state, "FindObjects");
            lua_pushcfunction(m_state, lua_find_object);
            lua_setglobal(m_state, "FindObject");
            lua_pushcfunction(m_state, lua_for_each_uobject);
            lua_setglobal(m_state, "ForEachUObject");
            lua_pushcfunction(m_state, lua_create_invalid_object);
            lua_setglobal(m_state, "CreateInvalidObject");
            lua_pushcfunction(m_state, lua_static_find_object);
            lua_setglobal(m_state, "StaticFindObject");
            lua_pushcfunction(m_state, lua_static_construct_object);
            lua_setglobal(m_state, "StaticConstructObject");
            lua_pushcfunction(m_state, lua_iterate_game_directories);
            lua_setglobal(m_state, "IterateGameDirectories");
            lua_pushcfunction(m_state, lua_create_logic_mods_directory);
            lua_setglobal(m_state, "CreateLogicModsDirectory");
            lua_pushcfunction(m_state, lua_fname_constructor);
            lua_setglobal(m_state, "FName");
            push_fname(m_state, {}, "None");
            lua_setglobal(m_state, "NAME_None");
            lua_newtable(m_state);
            lua_pushinteger(m_state, 0);
            lua_setfield(m_state, -2, "FNAME_Find");
            lua_pushinteger(m_state, 1);
            lua_setfield(m_state, -2, "FNAME_Add");
            lua_setglobal(m_state, "EFindName");
            lua_pushinteger(m_state, 0);
            lua_setglobal(m_state, "FNAME_Find");
            lua_pushinteger(m_state, 1);
            lua_setglobal(m_state, "FNAME_Add");
            register_compatibility_classes(m_state);
            register_property_types(m_state);
            register_input_constants(m_state);
            lua_pushthread(m_state);
            lua_setglobal(m_state, "MainThread");
            lua_pushnil(m_state);
            lua_setglobal(m_state, "__OriginalReturnValue");
            lua_newtable(m_state);
            const auto add_object_flag = [this](const char* name, std::uint32_t value) {
                lua_pushinteger(m_state, static_cast<lua_Integer>(value));
                lua_setfield(m_state, -2, name);
            };
            add_object_flag("RF_NoFlags", 0x00000000u);
            add_object_flag("RF_Public", 0x00000001u);
            add_object_flag("RF_Standalone", 0x00000002u);
            add_object_flag("RF_MarkAsNative", 0x00000004u);
            add_object_flag("RF_Transactional", 0x00000008u);
            add_object_flag("RF_ClassDefaultObject", 0x00000010u);
            add_object_flag("RF_ArchetypeObject", 0x00000020u);
            add_object_flag("RF_Transient", 0x00000040u);
            add_object_flag("RF_MarkAsRootSet", 0x00000080u);
            add_object_flag("RF_TagGarbageTemp", 0x00000100u);
            add_object_flag("RF_NeedInitialization", 0x00000200u);
            add_object_flag("RF_NeedLoad", 0x00000400u);
            add_object_flag("RF_KeepForCooker", 0x00000800u);
            add_object_flag("RF_NeedPostLoad", 0x00001000u);
            add_object_flag("RF_NeedPostLoadSubobjects", 0x00002000u);
            add_object_flag("RF_NewerVersionExists", 0x00004000u);
            add_object_flag("RF_BeginDestroyed", 0x00008000u);
            add_object_flag("RF_FinishDestroyed", 0x00010000u);
            add_object_flag("RF_BeingRegenerated", 0x00020000u);
            add_object_flag("RF_DefaultSubObject", 0x00040000u);
            add_object_flag("RF_WasLoaded", 0x00080000u);
            add_object_flag("RF_TextExportTransient", 0x00100000u);
            add_object_flag("RF_LoadCompleted", 0x00200000u);
            add_object_flag("RF_InheritableComponentTemplate", 0x00400000u);
            add_object_flag("RF_DuplicateTransient", 0x00800000u);
            add_object_flag("RF_StrongRefOnFrame", 0x01000000u);
            add_object_flag("RF_NonPIEDuplicateTransient", 0x01000000u);
            add_object_flag("RF_Dynamic", 0x02000000u);
            add_object_flag("RF_WillBeLoaded", 0x04000000u);
            add_object_flag("RF_HasExternalPackage", 0x08000000u);
            add_object_flag("RF_AllFlags", 0x0fffffffu);
            lua_setglobal(m_state, "EObjectFlags");
            lua_newtable(m_state);
            const auto add_internal_object_flag = [this](const char* name, std::uint32_t value) {
                lua_pushinteger(m_state, static_cast<lua_Integer>(value));
                lua_setfield(m_state, -2, name);
            };
            add_internal_object_flag("None", 0x00000000u);
            add_internal_object_flag("ReachableInCluster", 0x00800000u);
            add_internal_object_flag("ClusterRoot", 0x01000000u);
            add_internal_object_flag("Native", 0x02000000u);
            add_internal_object_flag("Async", 0x04000000u);
            add_internal_object_flag("AsyncLoading", 0x08000000u);
            add_internal_object_flag("Unreachable", 0x10000000u);
            add_internal_object_flag("PendingKill", 0x20000000u);
            add_internal_object_flag("RootSet", 0x40000000u);
            add_internal_object_flag("PendingConstruction", 0x80000000u);
            add_internal_object_flag("GarbageCollectionKeepFlags", 0x0e000000u);
            add_internal_object_flag("AllFlags", 0xff800000u);
            lua_setglobal(m_state, "EInternalObjectFlags");
            lua_pushcfunction(m_state, lua_fstring_constructor);
            lua_setglobal(m_state, "FString");
            lua_pushcfunction(m_state, lua_futf8string_constructor);
            lua_setglobal(m_state, "FUtf8String");
            lua_pushcfunction(m_state, lua_fansistring_constructor);
            lua_setglobal(m_state, "FAnsiString");
            lua_pushcfunction(m_state, lua_ftext_constructor);
            lua_setglobal(m_state, "FText");
            lua_pushcfunction(m_state, register_lua_ufunction_hook);
            lua_setglobal(m_state, "RegisterHook");
            lua_pushcfunction(m_state, unregister_lua_ufunction_hook);
            lua_setglobal(m_state, "UnregisterHook");
            lua_pushcfunction(m_state, lua_register_begin_play_pre_hook);
            lua_setglobal(m_state, "RegisterBeginPlayPreHook");
            lua_pushcfunction(m_state, lua_register_begin_play_post_hook);
            lua_setglobal(m_state, "RegisterBeginPlayPostHook");
            lua_pushcfunction(m_state, lua_register_end_play_pre_hook);
            lua_setglobal(m_state, "RegisterEndPlayPreHook");
            lua_pushcfunction(m_state, lua_register_end_play_post_hook);
            lua_setglobal(m_state, "RegisterEndPlayPostHook");
            lua_pushcfunction(m_state, lua_register_init_game_state_pre_hook);
            lua_setglobal(m_state, "RegisterInitGameStatePreHook");
            lua_pushcfunction(m_state, lua_register_init_game_state_post_hook);
            lua_setglobal(m_state, "RegisterInitGameStatePostHook");
            lua_pushcfunction(m_state, lua_register_load_map_pre_hook);
            lua_setglobal(m_state, "RegisterLoadMapPreHook");
            lua_pushcfunction(m_state, lua_register_load_map_post_hook);
            lua_setglobal(m_state, "RegisterLoadMapPostHook");
            lua_pushcfunction(m_state, lua_register_process_console_exec_pre_hook);
            lua_setglobal(m_state, "RegisterProcessConsoleExecPreHook");
            lua_pushcfunction(m_state, lua_register_process_console_exec_post_hook);
            lua_setglobal(m_state, "RegisterProcessConsoleExecPostHook");
            lua_pushcfunction(m_state, lua_register_call_function_by_name_pre_hook);
            lua_setglobal(
                    m_state,
                    "RegisterCallFunctionByNameWithArgumentsPreHook");
            lua_pushcfunction(m_state, lua_register_call_function_by_name_post_hook);
            lua_setglobal(
                    m_state,
                    "RegisterCallFunctionByNameWithArgumentsPostHook");
            lua_pushcfunction(m_state, lua_register_local_player_exec_pre_hook);
            lua_setglobal(m_state, "RegisterULocalPlayerExecPreHook");
            lua_pushcfunction(m_state, lua_register_local_player_exec_post_hook);
            lua_setglobal(m_state, "RegisterULocalPlayerExecPostHook");
            lua_pushcfunction(m_state, lua_register_console_command_handler);
            lua_setglobal(m_state, "RegisterConsoleCommandHandler");
            lua_pushcfunction(m_state, lua_register_console_command_global_handler);
            lua_setglobal(m_state, "RegisterConsoleCommandGlobalHandler");
            lua_pushcfunction(m_state, notify_on_new_object);
            lua_setglobal(m_state, "NotifyOnNewObject");
            lua_pushcfunction(m_state, register_lua_custom_event);
            lua_setglobal(m_state, "RegisterCustomEvent");
            lua_pushcfunction(m_state, unregister_lua_custom_event);
            lua_setglobal(m_state, "UnregisterCustomEvent");
            lua_pushcfunction(m_state, lua_restart_current_mod);
            lua_setglobal(m_state, "RestartCurrentMod");
            lua_pushcfunction(m_state, lua_uninstall_current_mod);
            lua_setglobal(m_state, "UninstallCurrentMod");
            lua_pushcfunction(m_state, lua_restart_mod);
            lua_setglobal(m_state, "RestartMod");
            lua_pushcfunction(m_state, lua_uninstall_mod);
            lua_setglobal(m_state, "UninstallMod");
            lua_pushcfunction(m_state, lua_load_asset);
            lua_setglobal(m_state, "LoadAsset");
            lua_pushcfunction(m_state, lua_load_export);
            lua_setglobal(m_state, "LoadExport");
            lua_pushcfunction(m_state, lua_load_export);
            lua_setglobal(m_state, "loadexport");
            lua_pushcfunction(m_state, lua_dump_all_objects);
            lua_setglobal(m_state, "DumpAllObjects");
            lua_pushcfunction(m_state, lua_register_custom_property);
            lua_setglobal(m_state, "RegisterCustomProperty");

            register_headless_unavailable_global(
                    m_state,
                    "IsKeyBindRegistered",
                    "UE4SS_WITH_INPUT=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "RegisterKeyBind",
                    "UE4SS_WITH_INPUT=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "RegisterKeyBindAsync",
                    "UE4SS_WITH_INPUT=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "GenerateSDK",
                    "UE4SS_WITH_SDK_GENERATOR=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "GenerateLuaTypes",
                    "UE4SS_WITH_SDK_GENERATOR=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "GenerateUHTCompatibleHeaders",
                    "UE4SS_WITH_SDK_GENERATOR=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "DumpUSMAP",
                    "UE4SS_WITH_UVTD=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "DumpStaticMeshes",
                    "UE4SS_WITH_GUI=OFF");
            register_headless_unavailable_global(
                    m_state,
                    "DumpAllActors",
                    "UE4SS_WITH_GUI=OFF");

            lua_pushcfunction(m_state, lua_execute_in_game_thread);
            lua_setglobal(m_state, "ExecuteInGameThread");
            lua_pushcfunction(m_state, lua_execute_in_game_thread);
            lua_setglobal(m_state, "RunOnGameThread");
            lua_pushcfunction(m_state, lua_execute_with_delay);
            lua_setglobal(m_state, "ExecuteWithDelay");
            lua_pushcfunction(m_state, lua_execute_async);
            lua_setglobal(m_state, "ExecuteAsync");
            lua_pushcfunction(m_state, lua_loop_async);
            lua_setglobal(m_state, "LoopAsync");
            lua_pushcfunction(m_state, lua_get_current_thread_id);
            lua_setglobal(m_state, "GetCurrentThreadId");
            lua_pushcfunction(m_state, lua_get_main_mod_thread_id);
            lua_setglobal(m_state, "GetMainModThreadId");
            lua_pushcfunction(m_state, lua_get_async_thread_id);
            lua_setglobal(m_state, "GetAsyncThreadId");
            lua_pushcfunction(m_state, lua_get_game_thread_id);
            lua_setglobal(m_state, "GetGameThreadId");
            lua_pushcfunction(m_state, lua_is_in_main_mod_thread);
            lua_setglobal(m_state, "IsInMainModThread");
            lua_pushcfunction(m_state, lua_is_in_async_thread);
            lua_setglobal(m_state, "IsInAsyncThread");
            lua_pushcfunction(m_state, lua_is_in_game_thread);
            lua_setglobal(m_state, "IsInGameThread");
            lua_pushcfunction(m_state, lua_execute_in_game_thread_with_delay);
            lua_setglobal(m_state, "ExecuteInGameThreadWithDelay");
            lua_pushcfunction(m_state, lua_retriggerable_execute_in_game_thread_with_delay);
            lua_setglobal(m_state, "RetriggerableExecuteInGameThreadWithDelay");
            lua_pushcfunction(m_state, lua_execute_in_game_thread_after_frames);
            lua_setglobal(m_state, "ExecuteInGameThreadAfterFrames");
            lua_pushcfunction(m_state, lua_loop_in_game_thread_with_delay);
            lua_setglobal(m_state, "LoopInGameThreadWithDelay");
            lua_pushcfunction(m_state, lua_loop_in_game_thread_after_frames);
            lua_setglobal(m_state, "LoopInGameThreadAfterFrames");
            lua_pushcfunction(m_state, lua_reset_delayed_action_timer);
            lua_setglobal(m_state, "ResetDelayedActionTimer");
            lua_pushcfunction(m_state, lua_set_delayed_action_timer);
            lua_setglobal(m_state, "SetDelayedActionTimer");
            lua_pushcfunction(m_state, lua_pause_delayed_action);
            lua_setglobal(m_state, "PauseDelayedAction");
            lua_pushcfunction(m_state, lua_unpause_delayed_action);
            lua_setglobal(m_state, "UnpauseDelayedAction");
            lua_pushcfunction(m_state, lua_cancel_delayed_action);
            lua_setglobal(m_state, "CancelDelayedAction");
            lua_pushcfunction(m_state, lua_is_valid_delayed_action_handle);
            lua_setglobal(m_state, "IsValidDelayedActionHandle");
            lua_pushcfunction(m_state, lua_is_delayed_action_active);
            lua_setglobal(m_state, "IsDelayedActionActive");
            lua_pushcfunction(m_state, lua_is_delayed_action_paused);
            lua_setglobal(m_state, "IsDelayedActionPaused");
            lua_pushcfunction(m_state, lua_get_delayed_action_time_remaining);
            lua_setglobal(m_state, "GetDelayedActionTimeRemaining");
            lua_pushcfunction(m_state, lua_get_delayed_action_time_elapsed);
            lua_setglobal(m_state, "GetDelayedActionTimeElapsed");
            lua_pushcfunction(m_state, lua_get_delayed_action_rate);
            lua_setglobal(m_state, "GetDelayedActionRate");
            lua_pushcfunction(m_state, lua_clear_all_delayed_actions);
            lua_setglobal(m_state, "ClearAllDelayedActions");
            lua_pushcfunction(m_state, lua_make_action_handle);
            lua_setglobal(m_state, "MakeActionHandle");

            lua_newtable(m_state);
            lua_pushinteger(m_state, 0);
            lua_setfield(m_state, -2, "ProcessEvent");
            lua_pushinteger(m_state, 1);
            lua_setfield(m_state, -2, "EngineTick");
            lua_setglobal(m_state, "EGameThreadMethod");
            lua_pushboolean(m_state, scheduler != nullptr && scheduler->is_ready());
            lua_setglobal(m_state, "EngineTickAvailable");
            lua_pushboolean(m_state, runtime.process_event_available());
            lua_setglobal(m_state, "ProcessEventAvailable");
            lua_pushboolean(m_state, runtime.static_construct_object_available());
            lua_setglobal(m_state, "StaticConstructObjectAvailable");
            lua_pushboolean(m_state, hooks != nullptr && hooks->is_ready());
            lua_setglobal(m_state, "NativeUFunctionHooksAvailable");
            lua_pushboolean(m_state, blueprint_hooks != nullptr && blueprint_hooks->is_ready());
            lua_setglobal(m_state, "BlueprintHooksAvailable");
            lua_pushboolean(m_state, blueprint_hooks != nullptr && blueprint_hooks->is_ready());
            lua_setglobal(m_state, "CustomEventsAvailable");
            lua_pushboolean(m_state, notifications != nullptr && notifications->is_ready());
            lua_setglobal(m_state, "ObjectNotificationsAvailable");
            lua_pushboolean(m_state, begin_play_hooks != nullptr && begin_play_hooks->is_ready());
            lua_setglobal(m_state, "BeginPlayHooksAvailable");
            lua_pushboolean(m_state, end_play_hooks != nullptr && end_play_hooks->is_ready());
            lua_setglobal(m_state, "EndPlayHooksAvailable");
            lua_pushboolean(
                    m_state,
                    init_game_state_hooks != nullptr && init_game_state_hooks->is_ready());
            lua_setglobal(m_state, "InitGameStateHooksAvailable");
            lua_pushboolean(m_state, load_map_hooks != nullptr && load_map_hooks->is_ready());
            lua_setglobal(m_state, "LoadMapHooksAvailable");
            lua_pushboolean(
                    m_state,
                    process_console_exec_hooks != nullptr &&
                            process_console_exec_hooks->is_ready());
            lua_setglobal(m_state, "ProcessConsoleExecHooksAvailable");
            lua_pushboolean(
                    m_state,
                    call_function_by_name_hooks != nullptr &&
                            call_function_by_name_hooks->is_ready());
            lua_setglobal(m_state, "CallFunctionByNameHooksAvailable");
            lua_pushboolean(
                    m_state,
                    local_player_exec_hooks != nullptr &&
                            local_player_exec_hooks->is_ready());
            lua_setglobal(m_state, "ULocalPlayerExecHooksAvailable");
            lua_pushboolean(m_state, 0);
            lua_setglobal(m_state, "InputAvailable");
            lua_pushboolean(m_state, 0);
            lua_setglobal(m_state, "GUIAvailable");
            lua_pushboolean(m_state, 0);
            lua_setglobal(m_state, "SDKGeneratorAvailable");
            lua_pushboolean(m_state, 0);
            lua_setglobal(m_state, "UVTDAvailable");
            lua_pushboolean(m_state, !ue4ss_root_directory.empty());
            lua_setglobal(m_state, "DumpAllObjectsAvailable");
            lua_pushboolean(m_state, 1);
            lua_setglobal(m_state, "CustomPropertiesAvailable");
            lua_pushboolean(m_state, 0);
            lua_setglobal(m_state, "ModLifecycleAvailable");
            std::string async_error;
            if (!start_async_worker(async_error))
            {
                return {false,
                        async_error.empty() ? "headless Lua async worker could not start"
                                            : std::move(async_error)};
            }
            return {true,
                    scheduler != nullptr && scheduler->is_ready()
                            ? "UObject discovery, reflected calls/properties, scheduling, and UFunction hook bindings installed"
                            : "read-only UObject discovery installed; game-thread scheduling explicitly unavailable"};
        }
        catch (const std::exception& exception)
        {
            return {false, exception.what()};
        }
        catch (...)
        {
            return {false, "unknown exception while installing read-only Lua bindings"};
        }
    }

    void HeadlessLuaState::invoke_ufunction_hook_callback(
            int reference,
            const UFunctionHookInvocation& invocation,
            bool post_callback) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return;
        }

        auto* runtime = get_runtime(m_state);
        std::vector<RemoteUnrealParameter> parameters;
        std::string parameter_error;
        if (runtime == nullptr || !runtime->describe_hook_parameters(
                                          invocation.function,
                                          invocation.stack_address,
                                          invocation.result_address,
                                          post_callback,
                                          parameters,
                                          parameter_error))
        {
            lua_pop(m_state, 1);
            std::fprintf(stderr,
                         "[UE4SS Linux] UFunction hook parameters unavailable: %s\n",
                         parameter_error.empty() ? "validated Unreal runtime is missing" : parameter_error.c_str());
            return;
        }

        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u)
        {
            epoch = m_next_hook_callback_epoch++;
        }
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, invocation.context);
        for (const auto& parameter : parameters)
        {
            push_remote_reflected_param(m_state, *this, epoch, parameter);
        }
        const int argument_count = static_cast<int>(parameters.size() + 1u);
        const int call_status = lua_pcall(m_state, argument_count, 1, 0);
        if (call_status != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] UFunction hook callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
        }
        else if (post_callback && !parameters.empty() && parameters.front().is_return && !lua_isnil(m_state, -1))
        {
            UnrealPropertyValue replacement;
            std::string replacement_error;
            if (!lua_to_remote_property_value(
                        m_state, -1, parameters.front().kind, replacement, replacement_error) ||
                !runtime->write_hook_parameter(parameters.front(), replacement, replacement_error))
            {
                std::fprintf(stderr,
                             "[UE4SS Linux] UFunction hook return override failed: %s\n",
                             replacement_error.c_str());
            }
        }
        lua_pop(m_state, 1);
        std::erase(m_active_hook_callback_epochs, epoch);
    }

    void HeadlessLuaState::invoke_custom_event_callback(
            int reference,
            const UFunctionHookInvocation& invocation) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return;
        }

        auto* runtime = get_runtime(m_state);
        std::vector<RemoteUnrealParameter> parameters;
        std::string parameter_error;
        if (runtime == nullptr || !runtime->describe_hook_parameters(
                                          invocation.function,
                                          invocation.stack_address,
                                          invocation.result_address,
                                          true,
                                          parameters,
                                          parameter_error))
        {
            lua_pop(m_state, 1);
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] custom event parameters unavailable: %s\n",
                    parameter_error.empty()
                            ? "validated Unreal runtime is missing"
                            : parameter_error.c_str());
            return;
        }

        const bool has_return =
                !parameters.empty() && parameters.front().is_return;
        const std::size_t first_argument = has_return ? 1u : 0u;
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u)
        {
            epoch = m_next_hook_callback_epoch++;
        }
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, invocation.context);
        for (std::size_t index = first_argument; index < parameters.size(); ++index)
        {
            push_remote_reflected_param(m_state, *this, epoch, parameters[index]);
        }
        const int argument_count =
                static_cast<int>(parameters.size() - first_argument + 1u);
        const int call_status = lua_pcall(m_state, argument_count, 1, 0);
        if (call_status != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] custom event callback failed: %s\n",
                    error != nullptr ? error : "unknown Lua error");
        }
        else if (has_return && !lua_isnil(m_state, -1))
        {
            UnrealPropertyValue replacement;
            std::string replacement_error;
            if (!lua_to_remote_property_value(
                        m_state,
                        -1,
                        parameters.front().kind,
                        replacement,
                        replacement_error) ||
                !runtime->write_hook_parameter(
                        parameters.front(), replacement, replacement_error))
            {
                std::fprintf(
                        stderr,
                        "[UE4SS Linux] custom event return override failed: %s\n",
                        replacement_error.c_str());
            }
        }
        lua_pop(m_state, 1);
        std::erase(m_active_hook_callback_epochs, epoch);
    }

    void HeadlessLuaState::invoke_begin_play_hook_callback(
            int reference, const ReadOnlyUObjectHandle& actor) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u)
        {
            epoch = m_next_hook_callback_epoch++;
        }
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, actor);
        if (lua_pcall(m_state, 1, 0, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] AActor::BeginPlay hook callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
        }
        std::erase(m_active_hook_callback_epochs, epoch);
    }

    void HeadlessLuaState::invoke_end_play_hook_callback(
            int reference,
            const ReadOnlyUObjectHandle& actor,
            std::int32_t& reason) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u)
        {
            epoch = m_next_hook_callback_epoch++;
        }
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, actor);
        const RemoteUnrealParameter reason_parameter{
                .kind = UnrealPropertyKind::signed_integer,
                .storage_address = std::bit_cast<std::uintptr_t>(&reason),
                .element_size = static_cast<std::int32_t>(sizeof(reason)),
        };
        push_remote_reflected_param(m_state, *this, epoch, reason_parameter);
        if (lua_pcall(m_state, 2, 0, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] AActor::EndPlay hook callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
        }
        std::erase(m_active_hook_callback_epochs, epoch);
    }

    void HeadlessLuaState::invoke_init_game_state_hook_callback(
            int reference,
            const ReadOnlyUObjectHandle& game_mode) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u)
        {
            epoch = m_next_hook_callback_epoch++;
        }
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, game_mode);
        if (lua_pcall(m_state, 1, 0, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] AGameModeBase::InitGameState hook callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
        }
        std::erase(m_active_hook_callback_epochs, epoch);
    }

    std::optional<bool> HeadlessLuaState::invoke_load_map_hook_callback(
            int reference,
            const LoadMapHookInvocation& invocation) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return std::nullopt;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return std::nullopt;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u)
        {
            epoch = m_next_hook_callback_epoch++;
        }
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, invocation.engine);
        push_remote_context_param(m_state, *this, epoch, invocation.world);
        push_furl(m_state, invocation.url);
        push_remote_context_param(m_state, *this, epoch, invocation.pending_game);
        lua_pushlstring(m_state, invocation.error.data(), invocation.error.size());
        if (lua_pcall(m_state, 5, 1, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] UEngine::LoadMap hook callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            std::erase(m_active_hook_callback_epochs, epoch);
            return std::nullopt;
        }
        std::optional<bool> result;
        if (lua_isboolean(m_state, -1))
        {
            result = lua_toboolean(m_state, -1) != 0;
        }
        else if (!lua_isnil(m_state, -1))
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] UEngine::LoadMap hook callback must return bool or nil\n");
        }
        lua_pop(m_state, 1);
        std::erase(m_active_hook_callback_epochs, epoch);
        return result;
    }

    std::optional<bool> HeadlessLuaState::invoke_process_console_exec_hook_callback(
            int reference, const ProcessConsoleExecInvocation& invocation) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return std::nullopt;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return std::nullopt;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u) epoch = m_next_hook_callback_epoch++;
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, invocation.context);
        lua_pushlstring(m_state, invocation.command.data(), invocation.command.size());
        lua_createtable(m_state, static_cast<int>(invocation.command_parts.size()), 0);
        for (std::size_t index = 0; index < invocation.command_parts.size(); ++index)
        {
            const auto& part = invocation.command_parts[index];
            lua_pushlstring(m_state, part.data(), part.size());
            lua_rawseti(m_state, -2, static_cast<lua_Integer>(index + 1u));
        }
        push_output_device(m_state, *this, epoch, invocation.output_device);
        push_remote_context_param(m_state, *this, epoch, invocation.executor);
        if (lua_pcall(m_state, 5, 1, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] UObject::ProcessConsoleExec hook callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            std::erase(m_active_hook_callback_epochs, epoch);
            return std::nullopt;
        }
        std::optional<bool> result;
        if (lua_isboolean(m_state, -1))
        {
            result = lua_toboolean(m_state, -1) != 0;
        }
        else if (!lua_isnil(m_state, -1))
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] ProcessConsoleExec hook callback must return bool or nil\n");
        }
        lua_pop(m_state, 1);
        std::erase(m_active_hook_callback_epochs, epoch);
        return result;
    }

    std::optional<bool> HeadlessLuaState::invoke_call_function_by_name_hook_callback(
            int reference,
            const CallFunctionByNameWithArgumentsInvocation& invocation) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return std::nullopt;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return std::nullopt;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u) epoch = m_next_hook_callback_epoch++;
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, invocation.context);
        lua_pushlstring(m_state, invocation.command.data(), invocation.command.size());
        push_output_device(m_state, *this, epoch, invocation.output_device);
        push_remote_context_param(m_state, *this, epoch, invocation.executor);
        lua_pushboolean(m_state, invocation.force_call_with_non_exec);
        if (lua_pcall(m_state, 5, 1, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] UObject::CallFunctionByNameWithArguments hook callback failed: %s\n",
                    error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            std::erase(m_active_hook_callback_epochs, epoch);
            return std::nullopt;
        }
        std::optional<bool> result;
        if (lua_isboolean(m_state, -1))
        {
            result = lua_toboolean(m_state, -1) != 0;
        }
        else if (!lua_isnil(m_state, -1))
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] CallFunctionByNameWithArguments hook callback must return bool or nil\n");
        }
        lua_pop(m_state, 1);
        std::erase(m_active_hook_callback_epochs, epoch);
        return result;
    }

    LocalPlayerExecCallbackResult
    HeadlessLuaState::invoke_local_player_exec_hook_callback(
            int reference, const LocalPlayerExecInvocation& invocation) noexcept
    {
        LocalPlayerExecCallbackResult result;
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return result;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return result;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u) epoch = m_next_hook_callback_epoch++;
        m_active_hook_callback_epochs.push_back(epoch);
        push_remote_context_param(m_state, *this, epoch, invocation.context);
        push_remote_context_param(m_state, *this, epoch, invocation.world);
        lua_pushlstring(m_state, invocation.command.data(), invocation.command.size());
        push_output_device(m_state, *this, epoch, invocation.output_device);
        if (lua_pcall(m_state, 4, 2, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] ULocalPlayer::Exec hook callback failed: %s\n",
                    error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            std::erase(m_active_hook_callback_epochs, epoch);
            return result;
        }
        if (lua_isboolean(m_state, -2))
        {
            result.return_override = lua_toboolean(m_state, -2) != 0;
        }
        else if (!lua_isnil(m_state, -2))
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] ULocalPlayer::Exec hook callback first return value must be bool or nil\n");
        }
        if (lua_isboolean(m_state, -1))
        {
            result.execute_original = lua_toboolean(m_state, -1) != 0;
        }
        else if (!lua_isnil(m_state, -1))
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] ULocalPlayer::Exec hook callback second return value must be bool or nil\n");
        }
        lua_pop(m_state, 2);
        std::erase(m_active_hook_callback_epochs, epoch);
        return result;
    }

    std::optional<bool> HeadlessLuaState::invoke_console_command_callback(
            int reference,
            const ProcessConsoleExecInvocation& invocation,
            bool require_game_viewport) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return std::nullopt;
        }
        if (require_game_viewport)
        {
            auto* runtime = get_runtime(m_state);
            std::string class_error;
            if (runtime == nullptr ||
                !runtime->is_a(invocation.context, "GameViewportClient", class_error))
            {
                return std::nullopt;
            }
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return std::nullopt;
        }
        std::uint64_t epoch = m_next_hook_callback_epoch++;
        if (epoch == 0u) epoch = m_next_hook_callback_epoch++;
        m_active_hook_callback_epochs.push_back(epoch);
        lua_pushlstring(m_state, invocation.command.data(), invocation.command.size());
        const std::size_t parameter_count =
                invocation.command_parts.empty() ? 0u : invocation.command_parts.size() - 1u;
        lua_createtable(m_state, static_cast<int>(parameter_count), 0);
        for (std::size_t index = 1; index < invocation.command_parts.size(); ++index)
        {
            const auto& part = invocation.command_parts[index];
            lua_pushlstring(m_state, part.data(), part.size());
            lua_rawseti(m_state, -2, static_cast<lua_Integer>(index));
        }
        push_output_device(m_state, *this, epoch, invocation.output_device);
        if (lua_pcall(m_state, 3, 1, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] registered console command callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            std::erase(m_active_hook_callback_epochs, epoch);
            return std::nullopt;
        }
        std::optional<bool> result;
        if (lua_isboolean(m_state, -1))
        {
            result = lua_toboolean(m_state, -1) != 0;
        }
        else
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] registered console command callback must return bool\n");
        }
        lua_pop(m_state, 1);
        std::erase(m_active_hook_callback_epochs, epoch);
        return result;
    }

    bool HeadlessLuaState::log_output_device(std::uint64_t epoch,
                                             std::uint64_t output_device,
                                             std::string_view message,
                                             std::string& error) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (epoch == 0u ||
            std::find(m_active_hook_callback_epochs.begin(),
                      m_active_hook_callback_epochs.end(),
                      epoch) == m_active_hook_callback_epochs.end())
        {
            error = "FOutputDevice is no longer valid outside its synchronous hook callback";
            return false;
        }
        if (m_process_console_exec_hook_manager == nullptr)
        {
            error = "validated ProcessConsoleExec manager is unavailable";
            return false;
        }
        return m_process_console_exec_hook_manager->log(output_device, message, error);
    }

    bool HeadlessLuaState::is_hook_callback_scope_active(std::uint64_t epoch) const noexcept
    {
        try
        {
            std::lock_guard lock{m_mutex};
            return epoch != 0u && std::find(
                                          m_active_hook_callback_epochs.begin(),
                                          m_active_hook_callback_epochs.end(),
                                          epoch) != m_active_hook_callback_epochs.end();
        }
        catch (...)
        {
            return false;
        }
    }

    bool HeadlessLuaState::invoke_new_object_callback(
            int reference, const ReadOnlyUObjectHandle& object) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr || reference == LUA_NOREF || reference == LUA_REFNIL)
        {
            return true;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return true;
        }
        push_object(m_state, object);
        if (lua_pcall(m_state, 1, 1, 0) != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] NotifyOnNewObject callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            return true;
        }
        const bool cancel = lua_type(m_state, -1) == LUA_TBOOLEAN && lua_toboolean(m_state, -1) != 0;
        lua_pop(m_state, 1);
        return cancel;
    }

    LuaExecutionResult HeadlessLuaState::execute_string(std::string_view source) noexcept
    {
        try
        {
            std::lock_guard lock{m_mutex};
            if (!is_ready())
            {
                return {false, "Lua state allocation failed"};
            }
            const std::string terminated_source{source};
            return protected_call(m_state, luaL_loadbufferx(m_state, terminated_source.data(), terminated_source.size(), "ue4ss-headless", "t"));
        }
        catch (const std::exception& error)
        {
            return {false, error.what()};
        }
        catch (...)
        {
            return {false, "unknown exception at the headless Lua boundary"};
        }
    }

    LuaExecutionResult HeadlessLuaState::execute_file(const std::filesystem::path& path) noexcept
    {
        try
        {
            std::lock_guard lock{m_mutex};
            if (!is_ready())
            {
                return {false, "Lua state allocation failed"};
            }
            prepend_script_module_paths(m_state, path);
            const std::string utf8_path = path.string();
            return protected_call(m_state, luaL_loadfilex(m_state, utf8_path.c_str(), "t"));
        }
        catch (const std::exception& error)
        {
            return {false, error.what()};
        }
        catch (...)
        {
            return {false, "unknown exception at the headless Lua file boundary"};
        }
    }

    std::thread::id HeadlessLuaState::main_thread_id() const noexcept
    {
        return m_main_thread_id;
    }

    std::thread::id HeadlessLuaState::async_thread_id() const noexcept
    {
        try
        {
            std::lock_guard lock{m_async_mutex};
            return m_async_thread_id;
        }
        catch (...)
        {
            return {};
        }
    }

    bool HeadlessLuaState::is_async_thread() const noexcept
    {
        const std::thread::id async_id = async_thread_id();
        return async_id != std::thread::id{} && async_id == std::this_thread::get_id();
    }

    bool HeadlessLuaState::start_async_worker(std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (m_state == nullptr)
            {
                error = "Lua state is unavailable while starting the async worker";
                return false;
            }
            if (m_async_thread.joinable())
            {
                return async_thread_id() != std::thread::id{};
            }
            if (m_async_state == nullptr)
            {
                m_async_state = lua_newthread(m_state);
                if (m_async_state == nullptr)
                {
                    error = "Lua could not allocate the per-mod async coroutine";
                    return false;
                }
                lua_pushvalue(m_state, -1);
                lua_setglobal(m_state, "AsyncThread");
                m_async_state_reference = luaL_ref(m_state, LUA_REGISTRYINDEX);
            }
            {
                std::lock_guard lock{m_async_mutex};
                m_async_stop_requested = false;
            }
            m_async_thread = std::thread{[this] { run_async_worker(); }};
            std::unique_lock lock{m_async_mutex};
            if (!m_async_wakeup.wait_for(lock, std::chrono::seconds{2}, [this] {
                    return m_async_thread_id != std::thread::id{};
                }))
            {
                lock.unlock();
                error = "per-mod async worker did not publish its thread id";
                stop_async_worker();
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            stop_async_worker();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while starting the per-mod async worker";
            stop_async_worker();
            return false;
        }
    }

    void HeadlessLuaState::stop_async_worker() noexcept
    {
        try
        {
            if (m_async_thread.joinable())
            {
                {
                    std::lock_guard lock{m_async_mutex};
                    m_async_stop_requested = true;
                }
                m_async_wakeup.notify_all();
                m_async_thread.join();
            }

            std::vector<int> references;
            {
                std::lock_guard lock{m_async_mutex};
                references.reserve(m_async_actions.size());
                for (const auto& action : m_async_actions)
                {
                    references.push_back(action.callback_reference);
                }
                m_async_actions.clear();
                m_async_thread_id = {};
                m_async_stop_requested = false;
            }
            for (const int reference : references)
            {
                release_lua_callback(reference);
            }
        }
        catch (...)
        {
        }
    }

    bool HeadlessLuaState::enqueue_async_callback(int reference,
                                                  std::chrono::milliseconds delay,
                                                  bool repeating) noexcept
    {
        try
        {
            if (!m_async_thread.joinable() || reference == LUA_NOREF ||
                reference == LUA_REFNIL)
            {
                return false;
            }
            std::chrono::milliseconds effective_delay =
                    delay.count() < 0 ? std::chrono::milliseconds{} : delay;
            if (repeating && effective_delay < std::chrono::milliseconds{5})
            {
                effective_delay = std::chrono::milliseconds{5};
            }
            {
                std::lock_guard lock{m_async_mutex};
                if (m_async_thread_id == std::thread::id{} ||
                    m_async_stop_requested)
                {
                    return false;
                }
                m_async_actions.push_back({reference,
                                           std::chrono::steady_clock::now() + effective_delay,
                                           effective_delay,
                                           repeating});
            }
            m_async_wakeup.notify_all();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool HeadlessLuaState::invoke_async_callback(int reference, bool repeating) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_async_state == nullptr)
        {
            return true;
        }
        lua_rawgeti(m_async_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_async_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_async_state, 1);
            return true;
        }
        const int call_status = lua_pcall(m_async_state, 0, repeating ? 1 : 0, 0);
        if (call_status != LUA_OK)
        {
            const char* error = lua_tostring(m_async_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] async Lua callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_async_state, 1);
            return true;
        }
        if (!repeating)
        {
            return true;
        }
        const bool stop = lua_isboolean(m_async_state, -1) &&
                          lua_toboolean(m_async_state, -1) != 0;
        lua_pop(m_async_state, 1);
        return stop;
    }

    void HeadlessLuaState::run_async_worker() noexcept
    {
        {
            std::lock_guard lock{m_async_mutex};
            m_async_thread_id = std::this_thread::get_id();
        }
        m_async_wakeup.notify_all();

        while (true)
        {
            AsyncAction action;
            bool have_action{};
            {
                std::unique_lock lock{m_async_mutex};
                if (m_async_actions.empty())
                {
                    m_async_wakeup.wait(lock, [this] {
                        return m_async_stop_requested || !m_async_actions.empty();
                    });
                }
                if (m_async_stop_requested)
                {
                    break;
                }
                const auto next = std::min_element(
                        m_async_actions.begin(),
                        m_async_actions.end(),
                        [](const AsyncAction& left, const AsyncAction& right) {
                            return left.due < right.due;
                        });
                if (next == m_async_actions.end())
                {
                    continue;
                }
                const auto now = std::chrono::steady_clock::now();
                if (next->due > now)
                {
                    m_async_wakeup.wait_until(lock, next->due);
                    continue;
                }
                action = *next;
                m_async_actions.erase(next);
                have_action = true;
            }
            if (!have_action)
            {
                continue;
            }

            const bool remove = invoke_async_callback(
                    action.callback_reference, action.repeating);
            bool repeat_action{};
            {
                std::lock_guard lock{m_async_mutex};
                repeat_action = action.repeating && !remove &&
                                !m_async_stop_requested;
                if (repeat_action)
                {
                    action.due = std::chrono::steady_clock::now() + action.delay;
                    m_async_actions.push_back(action);
                }
            }
            if (repeat_action)
            {
                m_async_wakeup.notify_all();
            }
            else
            {
                release_lua_callback(action.callback_reference);
            }
        }

        {
            std::lock_guard lock{m_async_mutex};
            m_async_thread_id = {};
        }
        m_async_wakeup.notify_all();
    }

    std::uint64_t HeadlessLuaState::enqueue_lua_callback(int reference,
                                                        GameThreadScheduler& scheduler,
                                                        std::chrono::milliseconds delay,
                                                        std::optional<std::int64_t> frames,
                                                        bool repeating,
                                                        std::uint64_t requested_handle) noexcept
    {
        try
        {
            const auto callback =
                    [this, reference, repeating] {
                        return invoke_lua_callback(reference, repeating);
                    };
            const auto cleanup = [this, reference] { release_lua_callback(reference); };
            if (frames.has_value())
            {
                return scheduler.enqueue_frames_owned(
                        reinterpret_cast<GameThreadScheduler::OwnerId>(this),
                        callback,
                        *frames,
                        repeating,
                        cleanup,
                        requested_handle);
            }
            return scheduler.enqueue_owned(
                    reinterpret_cast<GameThreadScheduler::OwnerId>(this),
                    callback,
                    delay,
                    repeating ? delay : std::chrono::milliseconds{},
                    cleanup,
                    requested_handle,
                    repeating);
        }
        catch (...)
        {
            return 0u;
        }
    }

    bool HeadlessLuaState::invoke_lua_callback(int reference, bool repeating) noexcept
    {
        std::lock_guard lock{m_mutex};
        if (m_state == nullptr)
        {
            return true;
        }
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
        if (lua_type(m_state, -1) != LUA_TFUNCTION)
        {
            lua_pop(m_state, 1);
            return true;
        }

        const int call_status = lua_pcall(m_state, 0, repeating ? 1 : 0, 0);
        if (call_status != LUA_OK)
        {
            const char* error = lua_tostring(m_state, -1);
            std::fprintf(stderr,
                         "[UE4SS Linux] scheduled Lua callback failed: %s\n",
                         error != nullptr ? error : "unknown Lua error");
            lua_pop(m_state, 1);
            return true;
        }

        bool stop = true;
        if (repeating)
        {
            stop = lua_isboolean(m_state, -1) && lua_toboolean(m_state, -1) != 0;
            lua_pop(m_state, 1);
        }
        return stop;
    }

    void HeadlessLuaState::release_lua_callback(int reference) noexcept
    {
        try
        {
            std::lock_guard lock{m_mutex};
            if (m_state != nullptr && reference != LUA_NOREF && reference != LUA_REFNIL)
            {
                luaL_unref(m_state, LUA_REGISTRYINDEX, reference);
            }
        }
        catch (...)
        {
        }
    }

    int schedule_lua_callback(lua_State* state,
                              int callback_index,
                              std::chrono::milliseconds delay,
                              std::chrono::milliseconds repeat) noexcept
    {
        auto* scheduler = get_scheduler(state);
        auto* owner = get_state_owner(state);
        if (scheduler == nullptr || owner == nullptr || !scheduler->is_ready())
        {
            return luaL_error(state, "game-thread scheduling is not available in this Linux runtime");
        }
        lua_pushvalue(state, callback_index);
        const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
        const bool repeating = repeat.count() > 0;
        if (owner->enqueue_lua_callback(
                    reference,
                    *scheduler,
                    delay,
                    std::nullopt,
                    repeating) == 0u)
        {
            luaL_unref(state, LUA_REGISTRYINDEX, reference);
            return luaL_error(state, "game-thread callback could not be queued");
        }
        return 0;
    }

    int schedule_lua_async_callback(lua_State* state,
                                    int callback_index,
                                    std::chrono::milliseconds delay,
                                    bool repeating) noexcept
    {
        auto* owner = get_state_owner(state);
        if (owner == nullptr)
        {
            return luaL_error(state, "per-mod async execution is not available in this Lua state");
        }
        lua_pushvalue(state, callback_index);
        const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
        if (!owner->enqueue_async_callback(reference, delay, repeating))
        {
            luaL_unref(state, LUA_REGISTRYINDEX, reference);
            return luaL_error(state, "per-mod async callback could not be queued");
        }
        return 0;
    }

    std::uint64_t schedule_lua_action(lua_State* state,
                                      int callback_index,
                                      std::chrono::milliseconds delay,
                                      std::optional<std::int64_t> frames,
                                      bool repeating,
                                      std::uint64_t requested_handle) noexcept
    {
        auto* scheduler = get_scheduler(state);
        auto* owner = get_state_owner(state);
        if (scheduler == nullptr || owner == nullptr || !scheduler->is_ready())
        {
            return 0u;
        }
        lua_pushvalue(state, callback_index);
        const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
        const std::uint64_t handle = owner->enqueue_lua_callback(
                reference, *scheduler, delay, frames, repeating, requested_handle);
        if (handle == 0u)
        {
            luaL_unref(state, LUA_REGISTRYINDEX, reference);
        }
        return handle;
    }

    LuaExecutionResult probe_lua_runtime() noexcept
    {
        try
        {
            HeadlessLuaState lua;
            if (!lua.is_ready())
            {
                return {false, "Lua state allocation failed"};
            }
            LuaExecutionResult result = lua.execute_string("assert(_VERSION == '" LUA_VERSION "')");
            if (result.success)
            {
                result.detail = lua.version() + " VM and standard libraries passed a protected headless probe";
            }
            return result;
        }
        catch (const std::exception& error)
        {
            return {false, error.what()};
        }
        catch (...)
        {
            return {false, "unknown exception while probing the headless Lua runtime"};
        }
    }

    LuaExecutionResult probe_readonly_unreal_lua(ReadOnlyUnrealRuntime& runtime,
                                                 GameThreadScheduler* scheduler,
                                                 UFunctionHookManager* hooks,
                                                 ObjectNotificationManager* notifications,
                                                 BlueprintHookManager* blueprint_hooks,
                                                 BeginPlayHookManager* begin_play_hooks,
                                                 EndPlayHookManager* end_play_hooks,
                                                 InitGameStateHookManager* init_game_state_hooks,
                                                 LoadMapHookManager* load_map_hooks,
                                                 ProcessConsoleExecHookManager* process_console_exec_hooks,
                                                 CallFunctionByNameWithArgumentsHookManager* call_function_by_name_hooks,
                                                 LocalPlayerExecHookManager* local_player_exec_hooks) noexcept
    {
        try
        {
            if (!runtime.is_ready())
            {
                return {false, "validated read-only Unreal runtime is required"};
            }
            HeadlessLuaState lua;
            LuaExecutionResult result =
                    lua.install_readonly_unreal_bindings(
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
            if (!result.success)
            {
                return result;
            }
            result = lua.execute_string(R"lua(
                assert(type(FindFirstOf) == "function")
                assert(type(FindAllOf) == "function")
                assert(type(FindObject) == "function")
                assert(type(FindObjects) == "function")
                assert(type(ForEachUObject) == "function")
                assert(type(CreateInvalidObject) == "function")
                assert(type(StaticFindObject) == "function")
                assert(type(StaticConstructObject) == "function")
                assert(type(IterateGameDirectories) == "function")
                assert(type(CreateLogicModsDirectory) == "function")
                assert(type(FUtf8String) == "function")
                assert(type(FAnsiString) == "function")
                assert(FUtf8String("Linux UTF-8 \u{2713}"):type() == "FUtf8String")
                assert(FAnsiString("Linux ANSI"):type() == "FAnsiString")
                assert(type(LoadAsset) == "function")
                assert(type(LoadExport) == "function")
                assert(type(loadexport) == "function")
                assert(type(LoadExport("malloc")) == "number")
                assert(LoadExport("malloc") ~= 0)
                assert(LoadExport("__ue4ss_linux_missing_export__") == 0)
                assert(LoadExport() == nil)
                assert(type(RegisterCustomProperty) == "function")
                assert(CustomPropertiesAvailable == true)
                assert(type(PropertyTypes) == "table")
                assert(PropertyTypes.IntProperty.Name == "IntProperty")
                assert(PropertyTypes.IntProperty.Size == 4)
                assert(PropertyTypes.ArrayProperty.Size == 16)
                assert(PropertyTypes.ObjectProperty.FFieldClassPointer ~= 0)
                local major, minor, hotfix = UE4SS.GetVersion()
                assert(type(major) == "number" and type(minor) == "number" and type(hotfix) == "number")
                assert(UnrealVersion.GetMajor() == 5 and UnrealVersion.GetMinor() == 1)
                assert(UnrealVersion.IsEqual(5, 1) and UnrealVersion.IsAtLeast(5, 0))
                assert(FPackageName.IsShortPackageName("Test"))
                assert(FPackageName.IsValidLongPackageName("/Game/Test"))
                assert(type(ModRef) == "table" and ModRef:type() == "ModRef")
                assert(type(MainThread) == "thread" and type(AsyncThread) == "thread")
                assert(type(DumpAllObjects) == "function")
                assert(type(IsKeyBindRegistered) == "function")
                assert(type(RegisterKeyBind) == "function")
                assert(type(RegisterKeyBindAsync) == "function")
                assert(type(GenerateSDK) == "function")
                assert(type(GenerateLuaTypes) == "function")
                assert(type(GenerateUHTCompatibleHeaders) == "function")
                assert(type(DumpUSMAP) == "function")
                assert(type(DumpStaticMeshes) == "function")
                assert(type(DumpAllActors) == "function")
                local input_ok, input_error = pcall(RegisterKeyBind, 0, function() end)
                assert(not input_ok)
                assert(string.find(input_error, "UE4SS_WITH_INPUT=OFF", 1, true))
                local sdk_ok, sdk_error = pcall(GenerateSDK)
                assert(not sdk_ok)
                assert(string.find(sdk_error, "UE4SS_WITH_SDK_GENERATOR=OFF", 1, true))
                local uvtd_ok, uvtd_error = pcall(DumpUSMAP)
                assert(not uvtd_ok)
                assert(string.find(uvtd_error, "UE4SS_WITH_UVTD=OFF", 1, true))
                local gui_ok, gui_error = pcall(DumpStaticMeshes)
                assert(not gui_ok)
                assert(string.find(gui_error, "UE4SS_WITH_GUI=OFF", 1, true))
                assert(type(ExecuteInGameThread) == "function")
                assert(type(RunOnGameThread) == "function")
                assert(type(ExecuteWithDelay) == "function")
                assert(type(ExecuteAsync) == "function")
                assert(type(LoopAsync) == "function")
                assert(type(GetCurrentThreadId) == "function")
                assert(type(GetMainModThreadId) == "function")
                assert(type(GetAsyncThreadId) == "function")
                assert(type(GetGameThreadId) == "function")
                assert(type(IsInMainModThread) == "function")
                assert(type(IsInAsyncThread) == "function")
                assert(type(IsInGameThread) == "function")
                assert(GetCurrentThreadId() == GetMainModThreadId())
                assert(GetCurrentThreadId():type() == "ThreadId")
                assert(not CreateInvalidObject():IsValid())
                assert(type(ExecuteInGameThreadWithDelay) == "function")
                assert(type(RetriggerableExecuteInGameThreadWithDelay) == "function")
                assert(type(ExecuteInGameThreadAfterFrames) == "function")
                assert(type(LoopInGameThreadWithDelay) == "function")
                assert(type(LoopInGameThreadAfterFrames) == "function")
                assert(type(ResetDelayedActionTimer) == "function")
                assert(type(SetDelayedActionTimer) == "function")
                assert(type(PauseDelayedAction) == "function")
                assert(type(UnpauseDelayedAction) == "function")
                assert(type(CancelDelayedAction) == "function")
                assert(type(IsValidDelayedActionHandle) == "function")
                assert(type(IsDelayedActionActive) == "function")
                assert(type(IsDelayedActionPaused) == "function")
                assert(type(GetDelayedActionTimeRemaining) == "function")
                assert(type(GetDelayedActionTimeElapsed) == "function")
                assert(type(GetDelayedActionRate) == "function")
                assert(type(ClearAllDelayedActions) == "function")
                assert(type(MakeActionHandle) == "function")
                assert(type(RegisterHook) == "function")
                assert(type(UnregisterHook) == "function")
                assert(type(RegisterBeginPlayPreHook) == "function")
                assert(type(RegisterBeginPlayPostHook) == "function")
                assert(type(RegisterEndPlayPreHook) == "function")
                assert(type(RegisterEndPlayPostHook) == "function")
                assert(type(RegisterInitGameStatePreHook) == "function")
                assert(type(RegisterInitGameStatePostHook) == "function")
                assert(type(RegisterLoadMapPreHook) == "function")
                assert(type(RegisterLoadMapPostHook) == "function")
                assert(type(RegisterProcessConsoleExecPreHook) == "function")
                assert(type(RegisterProcessConsoleExecPostHook) == "function")
                assert(type(RegisterCallFunctionByNameWithArgumentsPreHook) == "function")
                assert(type(RegisterCallFunctionByNameWithArgumentsPostHook) == "function")
                assert(type(RegisterULocalPlayerExecPreHook) == "function")
                assert(type(RegisterULocalPlayerExecPostHook) == "function")
                assert(type(RegisterConsoleCommandHandler) == "function")
                assert(type(RegisterConsoleCommandGlobalHandler) == "function")
                assert(type(NotifyOnNewObject) == "function")
                assert(type(RegisterCustomEvent) == "function")
                assert(type(UnregisterCustomEvent) == "function")
                assert(type(RestartCurrentMod) == "function")
                assert(type(UninstallCurrentMod) == "function")
                assert(type(RestartMod) == "function")
                assert(type(UninstallMod) == "function")
                assert(type(EngineTickAvailable) == "boolean")
                assert(type(StaticConstructObjectAvailable) == "boolean")
                assert(type(NativeUFunctionHooksAvailable) == "boolean")
                assert(type(BlueprintHooksAvailable) == "boolean")
                assert(type(CustomEventsAvailable) == "boolean")
                assert(type(ObjectNotificationsAvailable) == "boolean")
                assert(type(BeginPlayHooksAvailable) == "boolean")
                assert(type(EndPlayHooksAvailable) == "boolean")
                assert(type(InitGameStateHooksAvailable) == "boolean")
                assert(type(LoadMapHooksAvailable) == "boolean")
                assert(type(ProcessConsoleExecHooksAvailable) == "boolean")
                assert(type(CallFunctionByNameHooksAvailable) == "boolean")
                assert(type(ULocalPlayerExecHooksAvailable) == "boolean")
                assert(InputAvailable == false)
                assert(GUIAvailable == false)
                assert(SDKGeneratorAvailable == false)
                assert(UVTDAvailable == false)
                assert(type(DumpAllObjectsAvailable) == "boolean")
                assert(ModLifecycleAvailable == false)
                local lifecycle_ok, lifecycle_error = pcall(RestartCurrentMod)
                assert(not lifecycle_ok)
                assert(string.find(lifecycle_error, "lifecycle controller is unavailable", 1, true))
            )lua");
            if (result.success)
            {
                result.detail = "UObject discovery and reflected property/function bindings installed";
                if (scheduler != nullptr && scheduler->is_ready())
                {
                    result.detail += "; game-thread scheduling is available";
                }
                else
                {
                    result.detail += "; writes and scheduling are unavailable";
                }
                result.detail += hooks != nullptr && hooks->is_ready()
                                         ? "; native UFunction hooks are available"
                                         : "; native UFunction hooks are unavailable";
                result.detail += notifications != nullptr && notifications->is_ready()
                                         ? "; object notifications are available"
                                         : "; object notifications are unavailable";
                result.detail += blueprint_hooks != nullptr && blueprint_hooks->is_ready()
                                         ? "; Blueprint hooks are available"
                                         : "; Blueprint hooks are unavailable";
                result.detail += begin_play_hooks != nullptr && begin_play_hooks->is_ready()
                                         ? "; native AActor::BeginPlay hooks are available"
                                         : "; native AActor::BeginPlay hooks are unavailable";
                result.detail += end_play_hooks != nullptr && end_play_hooks->is_ready()
                                         ? "; native AActor::EndPlay hooks are available"
                                         : "; native AActor::EndPlay hooks are unavailable";
                result.detail += init_game_state_hooks != nullptr && init_game_state_hooks->is_ready()
                                         ? "; native AGameModeBase::InitGameState hooks are available"
                                         : "; native AGameModeBase::InitGameState hooks are unavailable";
                result.detail += load_map_hooks != nullptr && load_map_hooks->is_ready()
                                         ? "; native UEngine::LoadMap hooks are available"
                                         : "; native UEngine::LoadMap hooks are unavailable";
                result.detail += process_console_exec_hooks != nullptr &&
                                                 process_console_exec_hooks->is_ready()
                                         ? "; native UObject::ProcessConsoleExec hooks are available"
                                         : "; native UObject::ProcessConsoleExec hooks are unavailable";
                result.detail += call_function_by_name_hooks != nullptr &&
                                                 call_function_by_name_hooks->is_ready()
                                         ? "; native UObject::CallFunctionByNameWithArguments hooks are available"
                                         : "; native UObject::CallFunctionByNameWithArguments hooks are unavailable";
                result.detail += local_player_exec_hooks != nullptr &&
                                                 local_player_exec_hooks->is_ready()
                                         ? "; native ULocalPlayer::Exec hooks are available"
                                         : "; native ULocalPlayer::Exec hooks are unavailable";
            }
            return result;
        }
        catch (const std::exception& exception)
        {
            return {false, exception.what()};
        }
        catch (...)
        {
            return {false, "unknown exception while probing read-only Unreal Lua bindings"};
        }
    }
}
