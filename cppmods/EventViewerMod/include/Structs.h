#pragma once
#include <thread>
#include <String/StringType.hpp>

namespace RC::EventViewerMod
{
    struct CallStackEntry
    {
        CallStackEntry(StringType ContextName, StringType FunctionName, uint32_t Depth, std::thread::id ThreadId)
            : context_name(std::move(ContextName)),
              function_name(std::move(FunctionName)),
              depth(Depth),
              thread_id(ThreadId)
        {
        }

        const StringType context_name;
        const StringType function_name;
        const uint32_t depth;
        const std::thread::id thread_id;
        bool is_disabled = false;

        auto context_name_as_string() -> const char*;
        auto function_name_as_string() -> const char*;

        ~CallStackEntry();
    private:
        char* m_context_name = nullptr;
        char* m_function_name = nullptr;
    };
}