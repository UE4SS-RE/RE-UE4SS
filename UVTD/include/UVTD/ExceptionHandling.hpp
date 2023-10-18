#ifndef UNREALVTABLEDUMPER_EXCEPTIONHANDLING_HPP
#define UNREALVTABLEDUMPER_EXCEPTIONHANDLING_HPP

#include <stdexcept>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

namespace RC
{
    // Will try some code and properly propagate any exceptions
    // This is a simple helper function to avoid having 15 extra lines of code everywhere
    template <typename CodeToTry>
    auto constexpr TRY(CodeToTry code_to_try) -> void
    {
        try
        {
            code_to_try();
        }
        catch (std::exception& e)
        {
            if (!Output::has_internal_error())
            {
                Output::send(STR("Error: {}\n"), to_wstring(e.what()));
            }
            else
            {
                printf_s("Internal Error: %s\n", e.what());
            }
        }
    }
} // namespace RC

#endif // UNREALVTABLEDUMPER_EXCEPTIONHANDLING_HPP
