#pragma once
#include <thread>
#include <String/StringType.hpp>

namespace RC::EventViewerMod
{
    struct CallStackEntry
    {
        const StringType context_name;
        const StringType function_name;
        const uint32_t depth;
        const std::thread::id thread_id;
        bool is_disabled = false;
    };
}