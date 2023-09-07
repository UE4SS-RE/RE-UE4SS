#pragma once

#include <stdexcept>

#include <Helpers/String.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

namespace RC
{
    // Will try some code and properly propagate any exceptions
    // This is a simple helper function to avoid having 15 extra lines of code everywhere
    template<typename CodeToTry>
    auto constexpr TRY(CodeToTry code_to_try)
    {
        try
        {
            return code_to_try();
        }
        catch (std::exception& e)
        {
            if (!Output::has_internal_error())
            {
                Output::send<LogLevel::Error>(STR("Error: {}\n"), to_wstring(e.what()));
            }
            else
            {
                printf_s("Internal Error: %s\n", e.what());
            }

            using LambdaReturnType = decltype(code_to_try());
            if constexpr (!std::is_same_v<LambdaReturnType, void>)
            {
                return LambdaReturnType{};
            }
        }
    }
}


