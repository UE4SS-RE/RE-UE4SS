#ifndef UE4SS_REWRITTEN_OUTPUT_HPP
#define UE4SS_REWRITTEN_OUTPUT_HPP

#include <array>
#include <format>
#include <memory>
#include <shared_mutex>
#include <source_location>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <fmt/chrono.h>
#include <DynamicOutput/Common.hpp>
#include <DynamicOutput/Macros.hpp>
#include <DynamicOutput/OutputDevice.hpp>
#include <DynamicOutput/AsyncLogger.hpp>
#include <File/InternalFile.hpp>

#if RC_IS_ANSI == 1
#define RC_STD_MAKE_FORMAT_ARGS fmt::make_format_args
#else
#define RC_STD_MAKE_FORMAT_ARGS fmt::make_format_args<fmt::buffered_context<CharType>>
#endif

namespace RC::Output
{
    template <typename SupposedEnum>
    concept EnumType = std::is_enum_v<SupposedEnum>;

    using OutputDevicesContainerType = std::vector<std::unique_ptr<OutputDevice>>;

    class AsyncLogger;

    extern RC_DYNOUT_API std::shared_ptr<AsyncLogger> g_async_logger;

    struct AsyncConfig {
        bool enabled{false};
        bool immediate_error{true};
        bool immediate_warning{false};
        bool immediate_normal{false};
        bool immediate_verbose{false};
        bool bypass_for_system_logs{false};
    };

    extern RC_DYNOUT_API AsyncConfig g_async_config;

    inline auto should_flush_immediately(int32_t log_level) -> bool
    {
        switch (log_level) {
            case LogLevel::Error:   return g_async_config.immediate_error;
            case LogLevel::Warning: return g_async_config.immediate_warning;
            case LogLevel::Verbose: return g_async_config.immediate_verbose;
            default:                return g_async_config.immediate_normal;
        }
    }

    auto RC_DYNOUT_API has_internal_error() -> bool;

    template <typename DeviceType>
    auto get_device_internal(OutputDevicesContainerType& device_container) -> DeviceType&
    {
        if (device_container.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[Output::get_device_internal] tried to get_device but there were no devices.")
        }

        DeviceType* ret{};
        for (auto& device : device_container)
        {
            ret = dynamic_cast<DeviceType*>(device.get());
            if (ret)
            {
                break;
            }
        }

        if (!ret)
        {
            THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[Output::get_device_internal] tried to get_device but was unable to find device of type: {}", typeid(DeviceType).name()))
        }
        return *ret;
    }

    // Static container to hold default values
    class DefaultTargets
    {
      private:
        // Is empty unless set_default_device or set_default_devices is called
        // If empty, will cause an exception to be thrown if the static send() function is called
        // Otherwise it contains all of the devices that will receive output when the static send() function is called
        // Keep in mind that this is static so these will stay alive until main() ends or until you manually call the close_devices() function
        static inline OutputDevicesContainerType default_devices{};
        static inline int32_t default_log_level{LogLevel::Normal};
        static inline std::shared_mutex default_devices_mutex{};

      public:
        RC_DYNOUT_API auto static set_default_log_level(int32_t log_level) -> void;
        RC_DYNOUT_API auto static get_default_log_level() -> int32_t;
        RC_DYNOUT_API auto static close_all_default_devices() -> void;

        template<typename Func>
        auto static with_devices_read(Func&& func) -> decltype(auto)
        {
            std::shared_lock lock{default_devices_mutex};
            return func(default_devices);
        }

        template<typename Func>
        auto static with_devices_write(Func&& func) -> decltype(auto)
        {
            std::unique_lock lock{default_devices_mutex};
            return func(default_devices);
        }
    };

    // RAII class for making output devices not immediately close after calling send()
    // Cannot be used with default devices as those are already fully persistent, simply use the static Output::send() function instead
    template <typename OutputDeviceType, typename... OutputDeviceTypes>
    class Targets
    {
      private:
        // Is empty unless send_to was called
        // It then contains all of the devices that will receive output until either
        // A. The object leaves scope
        // or
        // B. close_devices() is manually called on the object
        OutputDevicesContainerType m_opened_devices{};

      private:
        template <typename DeviceType>
        auto open_device() -> void
        {
            m_opened_devices.emplace_back(std::make_unique<DeviceType>());
        }

        template <typename DeviceType, typename DeviceTypeWorkaround, typename... DeviceTypes>
        auto open_device() -> void
        {
            m_opened_devices.emplace_back(std::make_unique<DeviceType>());
            open_device<DeviceTypeWorkaround, DeviceTypes...>();
        }

      public:
        Targets()
        {
            open_device<OutputDeviceType, OutputDeviceTypes...>();
        };

        template <EnumType OptionalArg>
        auto send(File::StringViewType content, OptionalArg optional_arg) -> void
        {
            if (m_opened_devices.empty())
            {
                THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no opened devices.");
            }

            for (const auto& device : m_opened_devices)
            {
                ASSERT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(content, static_cast<int32_t>(optional_arg));
                }
                else
                {
                    device->receive(content);
                }
            }
        }

        template <typename... FmtArgs>
        auto send(File::StringViewType content, FmtArgs... fmt_args) -> void
        {
            if (m_opened_devices.empty())
            {
                THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no opened devices.");
            }

            for (const auto& device : m_opened_devices)
            {
                ASSERT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...)), 0);
                }
                else
                {
                    device->receive(fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...)));
                }
            }
        }

        template <EnumType OptionalArg, typename... FmtArgs>
        auto send(File::StringViewType content, OptionalArg optional_arg, FmtArgs... fmt_args) -> void
        {
            if (m_opened_devices.empty())
            {
                THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no opened devices.");
            }

            for (const auto& device : m_opened_devices)
            {
                ASSERT_OUTPUT_DEVICE_IS_VALID(device)
                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(fmt::vformat(content, fmt_args...), RC_STD_MAKE_FORMAT_ARGS(static_cast<int32_t>(optional_arg)));
                }
                else
                {
                    device->receive(fmt::vformat(content, RC_STD_MAKE_FORMAT_ARGS(fmt_args...)));
                }
            }
        }

        auto send(const File::StringType& content) -> void
        {
            if (m_opened_devices.empty())
            {
                THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no opened devices.");
            }

            for (const auto& device : m_opened_devices)
            {
                ASSERT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(content, 0);
                }
                else
                {
                    device->receive(content);
                }
            }
        }

        template <int32_t optional_arg, typename FmtArg, typename... FmtArgs>
        auto send(File::StringViewType content, FmtArg fmt_arg, FmtArgs... fmt_args) -> void
        {
            if (m_opened_devices.empty())
            {
                THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no opened devices.");
            }

            for (const auto& device : m_opened_devices)
            {
                ASSERT_OUTPUT_DEVICE_IS_VALID(device)
                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(fmt::vformat(content, RC_STD_MAKE_FORMAT_ARGS(fmt_arg, fmt_args...)), optional_arg);
                }
                else
                {
                    device->receive(fmt::vformat(content, RC_STD_MAKE_FORMAT_ARGS(fmt_args...)));
                }
            }
        }

        template <int32_t optional_arg>
        auto send(const File::StringType& content) -> void
        {
            if (m_opened_devices.empty())
            {
                THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no opened devices.");
            }

            for (const auto& device : m_opened_devices)
            {
                ASSERT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(content, optional_arg);
                }
                else
                {
                    device->receive(content);
                }
            }
        }

        template <typename DeviceType>
        auto get_device() -> DeviceType&
        {
            return get_device_internal<DeviceType>(m_opened_devices);
        }
    };

    // Call this once at the start of your program in order to set the default devices
    // Without this you cannot use the static send function (use the non-static send_to function instead)
    template <typename DeviceType>
    auto set_default_devices() -> DeviceType&
    {
        DeviceType* result = nullptr;
        DefaultTargets::with_devices_write([&](auto& devices) {
            result = static_cast<DeviceType*>(devices.emplace_back(std::make_unique<DeviceType>()).get());
        });
        return *result;
    }

    // Version of set_default_devices() that can take multiple devices
    template <typename DeviceType, typename DeviceTypeWorkaround, typename... DeviceTypes>
    auto set_default_devices() -> void
    {
        DefaultTargets::with_devices_write([](auto& devices) {
            devices.emplace_back(std::make_unique<DeviceType>());
        });
        set_default_devices<DeviceTypeWorkaround, DeviceTypes...>();
    }

    auto inline clear_all_default_devices() -> void
    {
        DefaultTargets::with_devices_write([](auto& devices) {
            devices.clear();
        });
    }

    // Sets the log level that will be used if one isn't explicitly provided with the 'send' function
    template <int32_t log_level>
    auto set_default_log_level() -> void
    {
        DefaultTargets::set_default_log_level(log_level);
    }

    template <typename... FmtArgs>
    auto send(File::StringViewType content, FmtArgs... fmt_args) -> void
    {
        DefaultTargets::with_devices_read([&](const auto& devices) {
            for (const auto& device : devices)
            {
                ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...)), 0);
                }
                else
                {
                    device->receive(fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...)));
                }
            }
        });
    }

    template <EnumType OptionalArg, typename... FmtArgs>
    auto send(File::StringViewType content, OptionalArg optional_arg, FmtArgs... fmt_args) -> void
    {
        DefaultTargets::with_devices_read([&](const auto& devices) {
            for (const auto& device : devices)
            {
                ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(fmt::vformat(content, RC_STD_MAKE_FORMAT_ARGS(fmt_args...)), static_cast<int32_t>(optional_arg));
                }
                else
                {
                    device->receive(fmt::vformat(content, RC_STD_MAKE_FORMAT_ARGS(fmt_args...)));
                }
            }
        });
    }

    auto RC_DYNOUT_API send(File::StringViewType content) -> void;

    template <EnumType OptionalArg>
    auto send(File::StringViewType content, OptionalArg optional_arg) -> void
    {
        DefaultTargets::with_devices_read([&](const auto& devices) {
            for (const auto& device : devices)
            {
                ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(content, optional_arg);
                }
                else
                {
                    device->receive(content);
                }
            }
        });
    }

    template <int32_t optional_arg, typename... FmtArgs>
    auto send(File::StringViewType content, FmtArgs... fmt_args) -> void
    {
        const bool use_async = g_async_config.enabled &&
                               !g_async_config.bypass_for_system_logs &&
                               !should_flush_immediately(optional_arg) &&
                               g_async_logger;

        if (use_async) {
            LogEntry entry;
            entry.timestamp = std::chrono::system_clock::now();
            entry.log_level = optional_arg;
            entry.thread_id = std::this_thread::get_id();

            auto formatted = fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...));
            entry.set_message(formatted);

            if (g_async_logger->try_enqueue(std::move(entry))) {
                return;
            }
        }
        else if (g_async_logger && g_async_config.enabled && !g_async_config.bypass_for_system_logs)
        {
            g_async_logger->flush();
        }

        DefaultTargets::with_devices_read([&](const auto& devices) {
            for (const auto& device : devices)
            {
                ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...)), optional_arg);
                }
                else
                {
                    device->receive(fmt::vformat(fmt::detail::to_string_view(content), RC_STD_MAKE_FORMAT_ARGS(fmt_args...)));
                }
            }
        });
    }

    template <int32_t optional_arg>
    auto send(File::StringViewType content) -> void
    {
        const bool use_async = g_async_config.enabled &&
                               !g_async_config.bypass_for_system_logs &&
                               !should_flush_immediately(optional_arg) &&
                               g_async_logger;

        if (use_async) {
            LogEntry entry;
            entry.timestamp = std::chrono::system_clock::now();
            entry.log_level = optional_arg;
            entry.thread_id = std::this_thread::get_id();
            entry.set_message(content);

            if (g_async_logger->try_enqueue(std::move(entry))) {
                return;
            }
        }
        else if (g_async_logger && g_async_config.enabled && !g_async_config.bypass_for_system_logs)
        {
            g_async_logger->flush();
        }

        DefaultTargets::with_devices_read([&](const auto& devices) {
            for (const auto& device : devices)
            {
                ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(content, optional_arg);
                }
                else
                {
                    device->receive(content);
                }
            }
        });
    }

    template <typename DeviceType>
    auto get_device() -> DeviceType&
    {
        DeviceType* result = nullptr;
        DefaultTargets::with_devices_read([&](const auto& devices) {
            result = &get_device_internal<DeviceType>(const_cast<OutputDevicesContainerType&>(devices));
        });
        return *result;
    }

    auto RC_DYNOUT_API close_all_default_devices() -> void;

    // Locks an output device so that nothing else can interact with it until the lock goes out of scope.
    // Used when you want to output multiple things with multiple calls to 'send' without interruptions.
    class Lock
    {
      private:
        const OutputDevice* m_output_device{};

      public:
        // Locks/Unlocks all default devices.
        Lock()
        {
            DefaultTargets::with_devices_read([](const auto& devices) {
                for (const auto& device : devices)
                {
                    device->lock();
                }
            });
        }

        Lock(OutputDevice* output_device) : m_output_device(output_device)
        {
            m_output_device->lock();
        }

        Lock(const OutputDevice* output_device) : m_output_device(output_device)
        {
            m_output_device->lock();
        }

        ~Lock()
        {
            if (m_output_device)
            {
                m_output_device->unlock();
            }
            else
            {
                DefaultTargets::with_devices_read([](const auto& devices) {
                    for (const auto& device : devices)
                    {
                        device->unlock();
                    }
                });
            }
        }
    };
} // namespace RC::Output

#endif // UE4SS_REWRITTEN_OUTPUT_HPP
