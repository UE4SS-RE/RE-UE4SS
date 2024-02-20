#ifndef UE4SS_REWRITTEN_DEBUGCONSOLEDEVICE_HPP
#define UE4SS_REWRITTEN_DEBUGCONSOLEDEVICE_HPP

#include <DynamicOutput/Common.hpp>
#include <DynamicOutput/Macros.hpp>
#include <DynamicOutput/OutputDevice.hpp>

namespace RC::Output
{
    // Very simple class that outputs to stdout
    class RC_DYNOUT_API DebugConsoleDevice : public OutputDevice
    {
      private:
        mutable bool m_windows_console_mode_set{};

      private:
#ifdef WIN32
        auto set_windows_console_out_mode_if_needed() const -> void;
#endif
      public:
      public:
#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        DebugConsoleDevice()
        {
            fmt::print("DebugConsoleDevice opening...\n");
        }

        ~DebugConsoleDevice() override
        {
            fmt::print("DebugConsoleDevice closing...\n");
        }
#else
        ~DebugConsoleDevice() override = default;
#endif

      public:
        auto has_optional_arg() const -> bool override;
        auto receive(SystemStringViewType fmt) const -> void override;
        auto receive_with_optional_arg(SystemStringViewType fmt, int32_t optional_arg = 0) const -> void override;
    };
} // namespace RC::Output

#endif // UE4SS_REWRITTEN_DEBUGCONSOLEDEVICE_HPP
