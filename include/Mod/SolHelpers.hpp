#ifndef UE4SS_REWRITTEN_SOL_HELPERS_HPP
#define UE4SS_REWRITTEN_SOL_HELPERS_HPP

#include <type_traits>
#include <string>
#include <string_view>

#define SOL_ALL_SAFETIES_ON 1
//#define SOL_SAFE_GETTER 0
#define SOL_PRINT_ERRORS 0
#define SOL_EXCEPTIONS 1
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1
#define SOL_USING_CXX_LUAJIT 1
#define SOL_USE_INTEROP 1
#include <sol/sol.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/FScriptArray.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/World.hpp>
#include <Unreal/AActor.hpp>
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
static auto push_##TypeName##property(const PropertyPusherFunctionParams& params) -> std::string \
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
        sol::stack::pop_n(params.lua_state, 1); \
    } \
    else if (params.push_type == PushType::ToLua || params.push_type == PushType::ToLuaNonTrivialLocal) \
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
        params.param_wrappers->emplace_back(ParamPtrWrapper{params.property, params.data, &push_##TypeName##property}); \
    } \
    else \
    { \
        return "[push_" #TypeName "property] Unhandled Operation";\
    } \
    return {}; \
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

// This was a failed attempt to get sol_lua_check/sol_lua_get to convert numbers to booleans.
//template<typename Handler>
//auto sol_lua_check(sol::types<bool>, lua_State* lua_state, int index, Handler&& handler, sol::stack::record& tracking) -> bool
//{
//    dump_stack(lua_state, "sol_lua_check, bool");
//	int absolute_index = lua_absindex(lua_state, index);
//    bool success = sol::stack::check<int>(lua_state, absolute_index, &sol::no_panic) || sol::stack::check<bool>(lua_state, absolute_index, &sol::no_panic);
//	tracking.use(1);
//	return success;
//}
//
//inline auto sol_lua_get(sol::types<bool>, lua_State* lua_state, int index, sol::stack::record& tracking) -> bool
//{
//    // For some reason, sol isn't calling this function.
//    // meta::meta_detail::is_adl_sol_lua_get_v<Tu> seems to fail.
//    dump_stack(lua_state, "sol_lua_get, bool");
//    int absolute_index = lua_absindex(lua_state, index);
//    if (auto maybe_bool = sol::stack::get<std::optional<bool>>(lua_state, absolute_index); maybe_bool.has_value())
//    {
//        tracking.use(1);
//        return maybe_bool.value();
//    }
//    else if (auto maybe_int = sol::stack::get<std::optional<int>>(lua_state, absolute_index); maybe_int.has_value())
//    {
//        tracking.use(1);
//        return maybe_int.value() >= 1;
//    }
//    else
//    {
//        std::abort();
//    }
//}

template<typename Handler>
auto sol_lua_check(sol::types<RC::Unreal::FName>, lua_State* lua_state, int index, Handler&& handler, sol::stack::record& tracking) -> bool
{
    int absolute_index = lua_absindex(lua_state, index);
    bool success = sol::stack::check<sol::nil_t>(lua_state, absolute_index, &sol::no_panic) ||
                   sol::stack::check<int>(lua_state, absolute_index, &sol::no_panic) ||
                   (sol::stack::check<bool>(lua_state, absolute_index, &sol::no_panic) && !sol::stack::get<bool>(lua_state, absolute_index));
    tracking.use(1);
    return success;
}

inline auto sol_lua_get(sol::types<RC::Unreal::FName>, lua_State* lua_state, int index, sol::stack::record& tracking) -> RC::Unreal::FName
{
    using namespace ::RC;
    using namespace ::RC::Unreal;
    int absolute_index = lua_absindex(lua_state, index);
    if (auto maybe_nil = sol::stack::get<std::optional<sol::nil_t>>(lua_state, absolute_index); maybe_nil.has_value())
    {
        tracking.use(1);
        return NAME_None;
    }
    else if (auto maybe_int = sol::stack::get<std::optional<int>>(lua_state, absolute_index); maybe_int.has_value())
    {
        tracking.use(1);
        return FName(maybe_int.value());
    }
    else if (auto maybe_bool = sol::stack::get<std::optional<bool>>(lua_state, absolute_index); maybe_bool.has_value())
    {
        tracking.use(1);
        return NAME_None;
    }
    else
    {
        std::abort();
    }
}

template<typename T, typename Handler>
auto sol_lua_interop_check(sol::types<T>, lua_State* lua_state, int relindex, sol::type index_type, Handler&& handler, sol::stack::record& tracking) -> bool
{
    using namespace RC::Unreal;

    if constexpr (std::is_same_v<T, bool>)
    {
        if (auto maybe_bool = sol::stack::check<bool>(lua_state, relindex))
        {
            return true;
        }
        else if (auto maybe_int = sol::stack::check<int>(lua_state, relindex))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if constexpr (std::is_same_v<T, FName>)
    {
        if (auto maybe_int = sol::stack::check<int>(lua_state, relindex))
        {
            return true;
        }
        else if (auto maybe_nil = sol::stack::check<sol::nil_t>(lua_state, relindex))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if constexpr (std::is_convertible_v<std::remove_cvref_t<T*>, UObject*>)
    {
        if (auto maybe_int = sol::stack::get<std::optional<int>>(lua_state, relindex); maybe_int.has_value())
        {
            return maybe_int.value() == 0;
        }
        else if (auto maybe_bool = sol::stack::get<std::optional<bool>>(lua_state, relindex); maybe_bool.has_value())
        {
            return !maybe_bool.value();
        }
        else
        {
            return false;
        }
    }
    else if constexpr (std::is_convertible_v<std::remove_cvref_t<T*>, FObjectInstancingGraph*>)
    {
        if (auto maybe_int = sol::stack::get<std::optional<int>>(lua_state, relindex); maybe_int.has_value())
        {
            return maybe_int.value() == 0;
        }
        else if (auto maybe_nil = sol::stack::get<std::optional<sol::nil_t>>(lua_state, relindex); maybe_nil.has_value())
        {
            return true;
        }
        if (auto maybe_bool = sol::stack::get<std::optional<bool>>(lua_state, relindex); maybe_bool.has_value())
        {
            return !maybe_bool.value();
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

template<typename InT, typename ...Args>
auto create_sol_userdata(lua_State* lua_state, Args&&... args) -> std::conditional_t<std::is_pointer_v<std::remove_cvref_t<InT>>, InT, InT*>
{
    using T = std::conditional_t<std::is_pointer_v<std::remove_cvref_t<InT>>, std::remove_pointer_t<InT>, InT>;
    auto k = sol::usertype_traits<T>::metatable();
    sol::stack::stack_detail::undefined_metatable fx(lua_state, &k[0], &sol::stack::stack_detail::set_undefined_methods_on<T>);
    T* obj = sol::detail::usertype_allocate<T>(lua_state);
    fx();
    if constexpr (std::is_pointer_v<std::remove_cvref_t<InT>>)
    {
        // The pointer is default constructed, which means it'll be a nullptr.
        return obj;
    }
    else
    {
        std::allocator<T> alloc{};
        std::allocator_traits<std::allocator<T>>::construct(alloc, obj, std::forward<Args>(args)...);
        return obj;
    }
}

template <typename T>
auto sol_lua_interop_get(sol::types<T> t, lua_State* lua_state, int relindex, void* unadjusted_pointer, sol::stack::record& tracking) -> std::pair<bool, T*>
{
    using namespace RC::Unreal;

    if constexpr (std::is_same_v<T, FName>)
    {
        if (auto maybe_int = sol::stack::get<std::optional<int>>(lua_state, relindex); maybe_int.has_value())
        {
            return {true, create_sol_userdata<FName>(lua_state, maybe_int.value())};
        }
        else if (auto maybe_nil = sol::stack::get<std::optional<sol::nil_t>>(lua_state, relindex); maybe_nil.has_value())
        {
            return {true, create_sol_userdata<FName>(lua_state, NAME_None)};
        }
        else
        {
            return {false, nullptr};
        }
    }
    else if constexpr (std::is_convertible_v<std::remove_cvref_t<T*>, UObject*>)
    {
        if (auto maybe_int = sol::stack::get<std::optional<int>>(lua_state, relindex); maybe_int.has_value())
        {
            // We're converting any number to nullptr.
            // This number realistically should only be zero because of the interop_check function.
            return {true, nullptr};
        }
        else if (auto maybe_bool = sol::stack::get<std::optional<bool>>(lua_state, relindex); maybe_bool.has_value())
        {
            // We're converting any bool to nullptr.
            // This bool realistically should only be false because of the interop_check function.
            return {true, nullptr};
        }
        else
        {
            return {false, nullptr};
        }
    }
    else if constexpr (std::is_convertible_v<std::remove_cvref_t<T*>, FObjectInstancingGraph*>)
    {
        return {true, static_cast<T*>(unadjusted_pointer)};
    }
    else
    {
        return {false, nullptr};
    }
}

namespace RC
{
    namespace Unreal
    {
        class UObject;
        struct FWeakObjectPtr;
        struct FSoftObjectPtr;
        class UClass;
        class UInterface;
    }
    
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

    struct PropertyPusherFunctionParams;
    using PropertyPusherFunction = std::string (*)(const PropertyPusherFunctionParams&);
    class ParamPtrWrapper
    {
    private:
        FProperty* property{};
        void* value_ptr{};
        const PropertyPusherFunction property_pusher{};

    public:
        ParamPtrWrapper(FProperty* in_property, void* data, PropertyPusherFunction pusher) : property(in_property), value_ptr(data), property_pusher(pusher) {};

        auto unwrap(lua_State* lua_state) -> void;
        auto rewrap(lua_State* lua_state) -> void;
    };

    enum class PushType { None, FromLua, ToLua, ToLuaParam, ToLuaNonTrivialLocal };
    struct PropertyPusherFunctionParams
    {
        lua_State* lua_state{};
        sol::protected_function_result* result{};
        FProperty* property{};
        void* data{};
        PushType push_type{};
        std::vector<ParamPtrWrapper>* param_wrappers{};
        void* base{};
        bool create_new_if_to_lua_non_trivial_local{true};
    };
    
    REGISTER_SOL_SERIALIZER(int8, int8_t)
    REGISTER_SOL_SERIALIZER(int16, int16_t)
    REGISTER_SOL_SERIALIZER(int, int32_t)
    REGISTER_SOL_SERIALIZER(int64, int64_t)
    REGISTER_SOL_SERIALIZER(byte, uint8_t)
    REGISTER_SOL_SERIALIZER(uint16, uint16_t)
    REGISTER_SOL_SERIALIZER(uint32, uint32_t)
    REGISTER_SOL_SERIALIZER(uint64, uint64_t)
    REGISTER_SOL_SERIALIZER(float, float)
    REGISTER_SOL_SERIALIZER(double, double)
    REGISTER_SOL_SERIALIZER(bool, bool)
    REGISTER_SOL_SERIALIZER(object, UObject*)
    REGISTER_SOL_SERIALIZER(weakobject, FWeakObjectPtr*)
    REGISTER_SOL_SERIALIZER(softclass, FSoftObjectPtr*)
    REGISTER_SOL_SERIALIZER(class, UClass*)
    REGISTER_SOL_SERIALIZER(name, FName)
    REGISTER_SOL_SERIALIZER(interface, UInterface*)
    auto push_structproperty(const PropertyPusherFunctionParams& params) -> std::string;
    auto push_arrayproperty(const PropertyPusherFunctionParams& params) -> std::string;
    auto push_functionproperty(const PropertyPusherFunctionParams& params) -> std::string;

    inline auto auto_construct_uobject(sol::state_view state, UObject* object) -> void
    {
        if (object->IsA<UFunction>())
        {
            //UFunction::construct(lua, nullptr, static_cast<UFunction*>(object));
        }
        else if (object->IsA<UClass>())
        {
            push_classproperty({state, nullptr, nullptr, &object, PushType::ToLua, nullptr});
        }
        else if (object->IsA<UScriptStruct>())
        {
            //ScriptStructWrapper script_struct_wrapper{static_cast<UScriptStruct*>(object), nullptr, nullptr};
            //UScriptStruct::construct(lua, script_struct_wrapper);
        }
        else if (object->IsA<UStruct>())
        {
            //UStruct::construct(lua, static_cast<UStruct*>(object));
        }
        else if (object->IsA<UEnum>())
        {
            //UEnum::construct(lua, static_cast<UEnum*>(object));
        }
        else if (object->IsA<UWorld>())
        {
            //UWorld::construct(lua, static_cast<UWorld*>(object));
        }
        else if (object->IsA<AActor>())
        {
            //AActor::construct(lua, static_cast<AActor*>(object));
        }
    }
    
    inline auto pointer_policy(lua_State* lua_state, int current_stack_return_count) -> int
    {
        // Multiple return values are not handled!
        if (current_stack_return_count == 1 && lua_isnil(lua_state, -1))
        {
            sol::stack::push(lua_state, InvalidObject{});   
        }

        auto maybe_uobject = sol::stack::get<std::optional<UObject*>>(lua_state);
        if (!maybe_uobject) { return current_stack_return_count; }
        auto_construct_uobject(lua_state, maybe_uobject.value());
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

    template<typename T>
    struct NoWrap
    {
        T value;
        NoWrap(T new_value) { value = new_value; }
    };

    template<typename>
    struct is_wrapped : std::true_type {};
    template<typename T>
    struct is_wrapped<NoWrap<T>> : std::false_type {};
    template<typename T>
    static constexpr bool is_wrapped_t = is_wrapped<T>::value;
    
    template<typename ...>
    constexpr std::false_type generate_false{};
    
    template<typename Type>
    auto static get_pusher_from_cpp_type()
    {
        if constexpr (std::is_same_v<std::remove_cvref_t<Type>, int8_t>) { return &push_int8property; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, int16_t>) { return &push_int16property; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, int32_t>) { return &push_intproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, int64_t>) { return &push_int64property; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, uint8_t>) { return &push_byteproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, uint16_t>) { return &push_uint16property; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, uint32_t>) { return &push_uint32property; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, uint64_t>) { return &push_uint64property; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, float>) { return &push_floatproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, double>) { return &push_doubleproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, bool>) { return &push_boolproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, FWeakObjectPtr*>) { return &push_weakobjectproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, FSoftObjectPtr*>) { return &push_softclassproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, FName>) { return &push_nameproperty; }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, UInterface>) { return &push_interfaceproperty; }
        // This branch must be the last branch in this constexpr-if-statement.
        // Otherwise we can't override types that inherit from UObject but that needs their pusher.
        else if constexpr (std::is_convertible_v<std::remove_cvref_t<Type>, UObject*>)
        {
            return &push_objectproperty;
        }
        else
        {
            static_assert(generate_false<Type>, "Type was unhandled and couldn't be converted to a pusher.");
        }
    }
    
    // Internal function, use 'call_function_safe' instead.
    enum class ParamsAreWrapped { Yes, No };
    template<ParamsAreWrapped params_are_wrapped, typename Callable, typename ...Params>
    static auto call_function_safe_internal(sol::state_view state, const Callable& function, Params&&... params) -> sol::protected_function_result
    {
        sol::protected_function_result result{};
        if constexpr (params_are_wrapped == ParamsAreWrapped::No)
        {
            result = function(params...);
        }
        else
        {
            if constexpr (std::is_same_v<Callable, sol::function>)
            {
                result = function(sol::as_args(params...));
            }
            else
            {
                result = function(params...);
            }
        }
        if (!result.valid() && sol::stack::check<StringType>(state))
        {
            Output::send<LogLevel::Error>(STR("Error: {}\n"), sol::stack::get<StringType>(state));
            state.stack_clear();
        }
        return result;
    }

    template<typename ...Params>
    auto call_function_wrap_params_safe_internal(sol::state_view state, auto&& function, Params&&... params)
    {
        std::vector<ParamPtrWrapper> param_wrappers{};
        // TODO: If the param is wrapped in 'NoWrap', like 'NoWrap(object)', then we need to supply this param unwrapped.
        //       Problem #1: How do we check using constexpr if it has NoWrap ?
        //                   We can use 'is_wrapped_t<T>' but since we're doing an unfold expression thing, how can we apply this ?
        //       Problem #2: The ordering of params might be a problem.
        //                   If param #1 is NoWrap, params #2 and #3 are not NoWrap, and param #4 is NoWrap, is it even possible ?
        //                   We would most likely need multiple vectors of ParamPtrWrapper. 
        //                   It appears to be possible to have non-as_args params sprinkled in with as_args params. 
        //                   But somehow we need the order correct. 
        //                   Maybe we could use a special pusher ?
        //                   A special pusher isn't going to work; We have to give the param to the function call, we can't push onto the stack.
        //                   For now, we'll just use a different function for this.
        (void(get_pusher_from_cpp_type<Params>()({state, nullptr, nullptr, &params, PushType::ToLuaParam, &param_wrappers})), ...);
        return call_function_safe_internal<ParamsAreWrapped::Yes>(state, function, param_wrappers);
    }
    
    // Call a Lua function and wrap the params if they aren't already wrapped.
    // A call to ':get()' has to be used in order to retrieve the param value.
    // A call to ':set(new_value)' can be utilized to set the value of the param.
    template<typename Callable, typename ...Params>
    auto static call_function_wrap_params_safe(sol::state_view state, const Callable& function, Params&&... params) -> sol::protected_function_result
    {
        if constexpr (sizeof...(Params) == 1)
        {
            if constexpr (std::is_same_v<std::remove_cvref_t<Params...>, std::vector<ParamPtrWrapper>>)
            {
                return call_function_safe_internal<ParamsAreWrapped::Yes>(state, function, params...);
            }
            else
            {
                return call_function_wrap_params_safe_internal(state, function, params...);
            }
        }
        else
        {
            return call_function_wrap_params_safe_internal(state, function, params...);
        }
    }

    // Call a Lua function and don't wrap the params.
    // If there's only one param and it's of type 'vector<ParamPtrWrapper>', treat as already wrapped.
    // If there's multiple params or the only param isn't 'vector<ParamPtrWrapper', no wrapping will occur, meaning no setting of the params in Lua and no call to :get() is needed.
    template<typename Callable, typename ...Params>
    auto static call_function_dont_wrap_params_safe(sol::state_view state, const Callable& function, Params&&... params) -> sol::protected_function_result
    {
        if constexpr (sizeof...(Params) == 1)
        {
            if constexpr (std::is_same_v<std::remove_cvref_t<Params...>, std::vector<ParamPtrWrapper>>)
            {
                return call_function_safe_internal<ParamsAreWrapped::Yes>(state, function, params...);
            }
            else
            {
                return call_function_safe_internal<ParamsAreWrapped::No>(state, function, params...);
            }
        }
        else
        {
            return call_function_safe_internal<ParamsAreWrapped::No>(state, function, params...);
        }
    }

    template<typename Callable, typename... Params>
    auto static call_function_with_manual_handler_safe(sol::state_view state, const Callable& function, Params&&... params)->sol::protected_function_result
    {
        std::vector<ParamPtrWrapper> param_wrappers{};
        (void(param_wrappers.emplace_back(ParamPtrWrapper{&params, nullptr})), ...);
        sol::protected_function_result result = function(param_wrappers);
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
