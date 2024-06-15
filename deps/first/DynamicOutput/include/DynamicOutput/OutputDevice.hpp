#ifndef UE4SS_REWRITTEN_OUTPUTDEVICE_HPP
#define UE4SS_REWRITTEN_OUTPUTDEVICE_HPP

#include <DynamicOutput/Common.hpp>
#include <DynamicOutput/Macros.hpp>
#include <File/Macros.hpp>

namespace RC
{
    // Namespaced enums are used here to make them scoped
    // enum class is not used here because it gets rid of an implicit conversion that I want
    namespace Color
    {
        enum Color
        {
            Default,
            NoColor,
            Cyan,
            Yellow,
            Red,
            Green,
            Blue,
            Purple,
        };
    } // namespace Color
    namespace LogLevel
    {
        enum LogLevel
        {
            Default = Color::Default,
            Normal = Color::NoColor,
            Verbose = Color::Cyan,
            Warning = Color::Yellow,
            Error = Color::Red,
        };
    } // namespace LogLevel
} // namespace RC

namespace RC::Output
{
    class RC_DYNOUT_API OutputDevice
    {
      protected:
        // Whether the device is ready to process output
        // It's up to each derived device to decide when they're ready and whether they use this member at all
        mutable bool m_is_device_ready{};

        // Formatter function
        using Formatter = SystemStringType (*)(SystemStringViewType);
        Formatter m_formatter{&default_format_string};

      public:
        virtual ~OutputDevice() = default;

      public:
        virtual auto has_optional_arg() const -> bool;

        virtual auto receive(SystemStringViewType fmt) const -> void = 0;

        // The 'optional_arg' type should be cast to the proper enum by the derived class
        virtual auto receive_with_optional_arg(SystemStringViewType fmt, int32_t optional_arg = 0) const -> void;

        virtual auto lock() const -> void{};

        virtual auto unlock() const -> void{};

      public:
        auto set_formatter(Formatter new_formatter) -> void;

      protected:
        auto static get_now_as_string() -> const SystemStringType;
        auto static default_format_string(SystemStringViewType) -> SystemStringType;
    };
} // namespace RC::Output

#endif // UE4SS_REWRITTEN_OUTPUTDEVICE_HPP
