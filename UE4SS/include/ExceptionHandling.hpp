#pragma once

#include <stdexcept>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

#define UE4SS_ERROR_OUTPUTTER()                                                                                                                                \
    if (!Output::has_internal_error())                                                                                                                         \
    {                                                                                                                                                          \
        Output::send<LogLevel::Error>(STR("Error: {}\n"), ensure_str(e.what()));                                                                               \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        printf_s("Internal Error: %s\n", e.what());                                                                                                            \
    }

// These macros should never be used in header files because you are required to include Windows.h.
#ifdef _WIN32
#ifndef SEH_TRY
#define SEH_TRY(Code)                                                                                                                                          \
    __try                                                                                                                                                      \
    Code
#endif
#ifndef SEH_EXCEPT
#define SEH_EXCEPT(Code)                                                                                                                                       \
    __except (SEH_exception_filter(GetExceptionCode(), GetExceptionInformation()))                                                                             \
    {                                                                                                                                                          \
        Code std::exit(EXIT_FAILURE);                                                                                                                          \
    }
#endif
#else
#ifndef SEH_TRY
#define SEH_TRY(Code) Code
#endif
#ifndef SEH_EXCEPT
#define SEH_EXCEPT(...)
#endif
#endif

namespace RC
{
#ifdef _WIN32
    enum SEH_FILTER_RESULT
    {
        EXECUTE_HANDLER = 1,
        CONTINUE_SEARCH = 0,
        CONTINUE_EXECUTION = -1,
    };

    inline int SEH_exception_filter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
    {
        return EXECUTE_HANDLER;
    }
#endif

    // Will try some code and properly propagate any exceptions
    // This is a simple helper function to avoid having 15 extra lines of code everywhere
    template <typename CodeToTry>
    auto constexpr TRY(CodeToTry code_to_try)
    {
        try
        {
            return code_to_try();
        }
        catch (std::exception& e)
        {
            UE4SS_ERROR_OUTPUTTER()

            using LambdaReturnType = decltype(code_to_try());
            if constexpr (!std::is_same_v<LambdaReturnType, void>)
            {
                return LambdaReturnType{};
            }
        }
    }
} // namespace RC
