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
} // namespace RC::Helper::Hook
