#pragma once

#include <memory>

#include <safetyhook.hpp>

namespace RC
{
    class CrashDumper
    {
      private:
        bool enabled = false;
        void* m_previous_exception_filter = nullptr;
        SafetyHookInline m_set_unhandled_exception_filter_hook;

      public:
        CrashDumper();
        ~CrashDumper();

      public:
        void enable();
        void set_full_memory_dump(bool enabled);
    };

}; // namespace RC
