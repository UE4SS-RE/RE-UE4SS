#pragma once

#include <Common.hpp>
#include <cstdint>

#include <String/StringType.hpp>

#define DefaultStructMemberData(member_name)                                                                                                                   \
    union {                                                                                                                                                    \
        const char* as_string{};                                                                                                                               \
        uint32_t as_int;                                                                                                                                       \
        float as_float;                                                                                                                                        \
    } member_name;

struct lua_State;

namespace RC::Unreal
{
    class FOutputDevice;
}

namespace RC::LuaMadeSimple
{
    class Lua;
}

namespace RC::LuaLibrary
{
    auto get_outputdevice_ref(const LuaMadeSimple::Lua&) -> const Unreal::FOutputDevice*;
    auto set_outputdevice_ref(const LuaMadeSimple::Lua&, Unreal::FOutputDevice*) -> void;

    // Global Lua functions are meant to be called with intact Lua stack
    auto global_print(const LuaMadeSimple::Lua&) -> int;

    // Used exclusively for the scripts inside the 'UE4SS_Signatures' folder
    auto deref_to_int32(const LuaMadeSimple::Lua&) -> int;

    // Shared functions, to be used externally (via exporting or some other way of sharing functions)

    enum class ExportedFunctionStatus
    {
        NO_ERROR_TO_EXPORT = 0,
        SUCCESS = 1,
        VARIABLE_NOT_FOUND = 2,
        MOD_IS_NULLPTR = 3,
        SCRIPT_FUNCTION_RETURNED_FALSE = 4,
        UNABLE_TO_CALL_SCRIPT_FUNCTION = 5,
        SCRIPT_FUNCTION_NOT_FOUND = 6,
        UNKNOWN_ERROR = 7,
        UE4SS_NOT_INITIALIZED = 8,
    };

    struct ReturnValue
    {
        ExportedFunctionStatus status{ExportedFunctionStatus::NO_ERROR_TO_EXPORT};
    };

    struct ScriptFuncReturnValue
    {
        bool return_value{false};
    };

    enum class DefaultDataType : uint8_t
    {
        ConstCharPtr,
        Float,
    };

    struct DefaultDataStruct
    {
        DefaultDataType data1_type;
        DefaultDataType data2_type;
        DefaultDataType data3_type;
        DefaultDataType data4_type;

        DefaultStructMemberData(data1) DefaultStructMemberData(data2) DefaultStructMemberData(data3) DefaultStructMemberData(data4)
    };

    extern "C"
    {
        // TODO: Make helper functions inside LuaMadeSimple so that the Lua C api doesn't need to be used in these functions

        __declspec(dllexport) auto get_lua_state_by_mod_name(const char* mod_name) -> lua_State*;
        __declspec(dllexport) auto execute_lua_in_mod(const char* mod_name, const char* script, char* output_buffer) -> const char*;

        using SetScriptVariableInt32Signature = void (*)(const char*, const char*, int32_t, ReturnValue&);
        __declspec(dllexport) auto set_script_variable_int32(const char* mod_name, const char* variable_name, int32_t new_value, ReturnValue&) -> void;

        using SetScriptVariableDefaultDataSignature = void (*)(const char*, const char*, DefaultDataStruct&, ReturnValue&);
        __declspec(dllexport) auto set_script_variable_default_data(const char* mod_name, const char* variable_name, DefaultDataStruct& external_data, ReturnValue&)
                -> void;

        using CallScriptFunctionSignature = void (*)(const char*, const char*, ReturnValue&, ScriptFuncReturnValue&);
        __declspec(dllexport) auto call_script_function(const char* mod_name, const char* function_name, ReturnValue&, ScriptFuncReturnValue&) -> void;

        using IsUE4SSInitializedSignature = bool (*)();
        __declspec(dllexport) auto is_ue4ss_initialized() -> bool;
    }
}; // namespace RC::LuaLibrary
