#pragma once

#include <Function/Function.hpp>

#include <safetyhook.hpp>

namespace RC::Helper::Hook
{
    template <typename>
    class call_hook;

    template <typename ReturnType, typename... Params>
    class call_hook<Function<ReturnType(Params...)>>
    {
      public:
        ReturnType operator()(SafetyHookInline& hook, Params... args)
        {
            return hook.call<ReturnType, Params...>(std::forward<Params>(args)...);
        }
    };

    template <typename>
    class call_hook_unsafe;
    
    template <typename ReturnType, typename... Params>
    class call_hook_unsafe<Function<ReturnType(Params...)>>
    {
    public:
        ReturnType operator()(SafetyHookInline& hook, Params... args)
        {
            return hook.unsafe_call<ReturnType, Params...>(std::forward<Params>(args)...);
        }
    };
} // namespace RC::Helper::Hook
