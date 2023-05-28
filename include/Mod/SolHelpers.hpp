#ifndef UE4SS_REWRITTEN_SOL_HELPERS_HPP
#define UE4SS_REWRITTEN_SOL_HELPERS_HPP

#include <type_traits>

#define SOL_ALL_SAFETIES_ON 1
//#define SOL_SAFE_GETTER 0
#define SOL_PRINT_ERRORS 0
#define SOL_EXCEPTIONS 1
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1
#define SOL_USING_CXX_LUAJIT 1
#include <sol/sol.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

#include <Unreal/UFunctionStructs.hpp>
#include <map.h>

#define SOL_REMOVE_CVREF(x) std::declval<x>()

#define SOL_ARGS(...) __VA_ARGS__

#define SOL_MEMBER_FUNC_NEEDS_PTR_POLICY(Type, Func, Yes, No, ...) \
if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(std::declval<Type>().Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))>>) \
{ \
    return Yes(); \
} \
else \
{ \
    return No(); \
}

#define SOL_FREE_FUNC_NEEDS_PTR_POLICY(Func, Yes, No, ...) \
if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))>>) \
{ \
    return Yes(); \
} \
else \
{ \
    return No(); \
}

// Choose a member-function overload by specifying a class/struct name, function name, and arg types.
#define CHOOSE_MEMBER_OVERLOAD(Type, Func, ...) \
[] { \
    SOL_MEMBER_FUNC_NEEDS_PTR_POLICY(Type, Func, \
        [] { return sol::policies(static_cast<decltype(std::declval<Type>().Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))(Type::*)(__VA_ARGS__)>(&Type::Func), &pointer_policy); }, \
        [] { return static_cast<decltype(std::declval<Type>().Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))(Type::*)(__VA_ARGS__)>(&Type::Func); }, \
    __VA_ARGS__) \
}()

// Choose a const member-function overload by specifying a class/struct name, function name, and arg types.
#define CHOOSE_CONST_MEMBER_OVERLOAD(Type, Func, ...) \
[] { \
    SOL_MEMBER_FUNC_NEEDS_PTR_POLICY(Type, Func, \
        [] { return sol::policies(static_cast<decltype(std::declval<Type>().Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))(Type::*)(__VA_ARGS__) const>(&Type::Func), &pointer_policy); }, \
        [] { return static_cast<decltype(std::declval<Type>().Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))(Type::*)(__VA_ARGS__) const>(&Type::Func); }, \
    __VA_ARGS__) \
}()

// Choose a free-function overload by specifying a function name, and arg types.
#define CHOOSE_FREE_OVERLOAD(Func, ...) \
[] { \
    SOL_FREE_FUNC_NEEDS_PTR_POLICY(Func, \
        [] { return sol::policies(static_cast<decltype(Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))(*)(__VA_ARGS__)>(&Func), &pointer_policy); }, \
        [] { return static_cast<decltype(Func(__VA_OPT__(MAP_LIST(SOL_REMOVE_CVREF, __VA_ARGS__))))(*)(__VA_ARGS__)>(&Func); }, \
    __VA_ARGS__) \
}()

#define REGISTER_SOL_SERIALIZER(TypeName, Type) \
static auto push_##TypeName##property(const PropertyPusherFunctionParams& params) -> const char* \
{ \
    using Value = Type; \
    using OptionalValue = std::optional<Value>; \
    if (params.push_type == PushType::FromLua) \
    { \
        auto maybe_value = params.result ? params.result->get<OptionalValue>() : sol::stack::get<OptionalValue>(params.lua_state); \
        if (!maybe_value.has_value()) \
        { \
            return "[push_" #TypeName "property] Tried to push non-" #Type " as " #Type; \
        } \
        *std::bit_cast<Value*>(params.data) = maybe_value.value(); \
    } \
    else if (params.push_type == PushType::ToLua) \
    { \
        if (params.data) \
        { \
            sol::stack::push(params.lua_state, *std::bit_cast<Value*>(params.data)); \
        } \
        else \
        { \
            sol::stack::push(params.lua_state, InvalidObject{}); \
        } \
    } \
    else if (params.push_type == PushType::ToLuaParam) \
    { \
        if (!params.param_wrappers) { return "[push_" #TypeName "property] Tried setting Lua param but param wrapper pointer was nullptr"; } \
        params.param_wrappers->emplace_back(ParamPtrWrapper{params.data, &push_##TypeName##property}); \
    } \
    return nullptr; \
}

#define EXIT_NATIVE_HOOK_WITH_ERROR(Action, LuaData, ErrorMessage) \
Output::send<LogLevel::Error>(ErrorMessage); \
LuaData.function_processing_failed = true; \
is_mid_script_execution = false; \
Action;

// dumpstack function from: https://stackoverflow.com/questions/59091462/from-c-how-can-i-print-the-contents-of-the-lua-stack/59097940#59097940
// it's been modified slightly
inline auto dump_stack(lua_State* lua_state, const char* message) -> void
{
    printf_s("\n\nLUA Stack dump -> START------------------------------\n%s\n", message);
    int top = lua_gettop(lua_state);
    for (int i = 1; i <= top; i++)
    {
        printf_s("%d\t%s\t", i, luaL_typename(lua_state, i));
        switch (lua_type(lua_state, i))
        {
        case LUA_TNUMBER:
            printf_s("%g", lua_tonumber(lua_state, i));
            break;
        case LUA_TSTRING:
            printf_s("%s", lua_tostring(lua_state, i));
            break;
        case LUA_TBOOLEAN:
            printf_s("%s", (lua_toboolean(lua_state, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            printf_s("nil");
            break;
        case LUA_TFUNCTION:
            printf_s("function");
            break;
        default:
            printf_s("%p", lua_topointer(lua_state, i));
            break;
        }
        printf_s("\n");
    }
    printf_s("\nLUA Stack dump -> END----------------------------\n\n");
}

namespace RC
{
    using namespace Unreal;

    class SolMod;
    inline auto get_mod_ref(sol::state_view sol) -> SolMod*
    {
        auto maybe_mod = sol.get<std::optional<SolMod*>>("ModRef");
        if (!maybe_mod.has_value())
        {
            Output::send<LogLevel::Warning>(STR("[get_mod_ref] called without 'ModRef' existing in the Lua state\n"));
            return nullptr;
        }
        else
        {
            return maybe_mod.value();
        }
    }

    struct InvalidObject
    {
        static auto Index(sol::stack_object, sol::this_state) -> std::function<InvalidObject()>
        {
            return [] { return InvalidObject{}; };
        }
    };

    inline auto pointer_policy(lua_State* lua_state, int current_stack_return_count) -> int
    {
        // Multiple return values are not handled!
        if (current_stack_return_count == 1 && lua_isnil(lua_state, -1))
        {
            sol::stack::push(lua_state, InvalidObject{});   
        }
        return current_stack_return_count;
    }

    struct LuaUnrealScriptFunctionData
    {
        CallbackId pre_callback_id;
        CallbackId post_callback_id;
        UFunction* unreal_function;
        SolMod* mod;
        lua_State* lua;
        //const int lua_callback_ref;
        sol::function lua_callback;
        sol::protected_function_result lua_callback_result;
        bool function_processing_failed{};

        bool has_return_value{};
        // Will be non-nullptr if the UFunction has a return value
        FProperty* return_property{};
    };

    struct PropertyPusherFunctionParams;
    using PropertyPusherFunction = const char* (*)(const PropertyPusherFunctionParams&);
    class ParamPtrWrapper
    {
    private:
        void* value_ptr{};
        const PropertyPusherFunction property_pusher{};

    public:
        ParamPtrWrapper(void* data, PropertyPusherFunction pusher) : value_ptr(data), property_pusher(pusher) {};

        auto unwrap(lua_State* lua_state) -> void;
        auto rewrap(lua_State* lua_state) -> void;
    };

    template<typename Callable>
    auto static call_function_safe(sol::state_view state, const Callable& function, const std::vector<ParamPtrWrapper>& params = {}) -> sol::protected_function_result
    {
        sol::protected_function_result result{};
        if constexpr (std::is_same_v<Callable, sol::function>)
        {
            result = function(sol::as_args(params));
        }
        else
        {
            result = function(params);
        }
        if (!result.valid() && sol::stack::check<StringType>(state))
        {
            Output::send<LogLevel::Error>(STR("Error: {}\n"), sol::stack::get<StringType>(state));
            state.stack_clear();
        }
        return result;
    }

    template<typename Callable, typename... Params>
    auto static call_function_safe(sol::state_view state, const Callable& function, Params... params)->sol::protected_function_result
    {
        sol::protected_function_result result = function(params...);
        if (!result.valid() && sol::stack::check<StringType>(state))
        {
            Output::send<LogLevel::Error>(STR("Error: {}\n"), sol::stack::get<StringType>(state));
            state.stack_clear();
        }
        return result;
    }

    auto return_from_function_with_values(sol::state_view state, auto&... values) -> sol::variadic_results
    {
        sol::variadic_results return_values{};
        (void(return_values.emplace_back(state, sol::in_place, values)), ...);
        return return_values;
    }

    template<int NumberOfReturnValues = 0>
    auto exit_script_with_error(sol::state_view state, StringType error_message) -> std::conditional_t<NumberOfReturnValues == 0, void, sol::variadic_results>
    {
        Output::send<LogLevel::Error>(STR("{}\n"), error_message);
        // TODO: Figure out how to call 'exit_callback_with_values' with the a number of sol::nil's equal to the value of 'NumberOfReturnValues'.
        if constexpr (NumberOfReturnValues == 1)
        {
            return return_from_function_with_values(state, sol::nil);
        }
        if constexpr (NumberOfReturnValues == 2)
        {
            return return_from_function_with_values(state, sol::nil, sol::nil);
        }
        else
        {
            return;
        }
    }

    auto lua_unreal_script_function_hook_pre(UnrealScriptFunctionCallableContext context, void* custom_data) -> void;
    auto lua_unreal_script_function_hook_post(UnrealScriptFunctionCallableContext context, void* custom_data) -> void;
}

#endif // UE4SS_REWRITTEN_SOL_HELPERS_HPP
