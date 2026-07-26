#pragma once

#include <cstdint>
#include <memory>

#ifdef _WIN32
namespace PLH
{
    class IatHook;
}
#endif

namespace RC
{
    class CrashDumper
    {
      private:
        bool enabled = false;
#ifdef _WIN32
        void* m_previous_exception_filter = nullptr;
        std::unique_ptr<PLH::IatHook> m_set_unhandled_exception_filter_hook;
        uint64_t m_hook_trampoline_set_unhandled_exception_filter_hook;
#endif

      public:
        CrashDumper();
        ~CrashDumper();

      public:
        void enable();
        void set_full_memory_dump(bool enabled);
    };

}; // namespace RC
