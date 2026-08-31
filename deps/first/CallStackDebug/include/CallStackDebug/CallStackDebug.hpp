#pragma once

#include <vector>

#include <CallStackDebug/Common.hpp>
#include <String/StringType.hpp>

#ifndef RC_REQ_STACKTRACE_MIN_VER
#define RC_REQ_STACKTRACE_MIN_VER 202011L
#endif

#ifndef RC_STACKTRACE_ENABLED
#define RC_STACKTRACE_ENABLED __cpp_lib_stacktrace >= RC_REQ_STACKTRACE_MIN_VER
#endif

namespace RC::CallStackDebug
{
    using ModuleNameAndOffsetPair = std::pair<StringType, size_t>;
    using CallStackWithoutSymbols = std::vector<ModuleNameAndOffsetPair>;

    // Generates a new call stack.
    // Leave the CallStackWithoutSymbols param empty to use the current stack context.
    auto RC_CALL_STACK_DEBUG_API generate_new_call_stack(const CallStackWithoutSymbols& = {}) -> void;

    // Get the latest call stack trace.
    // Remember to generate a trace before calling this function.
    auto RC_CALL_STACK_DEBUG_API get_call_stack() -> StringType;

    // Returns a call stack without symbols attached, useful when debugging symbols are unavailable.
    // A use-case would be to serialize module + RIP pairs for symbolication later when debugging symbols are available.
    auto RC_CALL_STACK_DEBUG_API get_unsymbolized_call_stack() -> CallStackWithoutSymbols;

    // Turns module + RIP pairs into a symbolized call stack.
    auto RC_CALL_STACK_DEBUG_API symbolicate_call_stack(const CallStackWithoutSymbols&) -> StringType;
} // namespace RC::CallStackDebug
