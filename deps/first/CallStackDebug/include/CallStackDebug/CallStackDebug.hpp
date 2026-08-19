#pragma once

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
    auto RC_CALL_STACK_DEBUG_API generate_call_stack() -> StringType;
} // namespace RC::PDB
