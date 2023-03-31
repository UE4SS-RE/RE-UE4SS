#ifndef UE4SS_REWRITTEN_DEBUGCONSOLEDEVICE_HPP
#define UE4SS_REWRITTEN_DEBUGCONSOLEDEVICE_HPP

#ifdef UE4SS_CONSOLE_COLORS_ENABLED
#include <mutex>
#endif

#include <DynamicOutput/Common.hpp>
#include <DynamicOutput/OutputDevice.hpp>
#include <DynamicOutput/Macros.hpp>

namespace RC::Output
{
    // Very simple class that outputs to stdout
    class RC_DYNOUT_API DebugConsoleDevice : public OutputDevice
    {
    private:
#ifdef UE4SS_CONSOLE_COLORS_ENABLED
        mutable std::recursive_mutex m_receive_mutex;
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
        auto receive(File::StringViewType fmt) const -> void override;
        auto receive_with_optional_arg(File::StringViewType fmt, int32_t optional_arg = 0) const -> void override;
        auto lock() const -> void override;
        auto unlock() const -> void override;
    };
}


#endif //UE4SS_REWRITTEN_DEBUGCONSOLEDEVICE_HPP
