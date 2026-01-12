#include <DynamicOutput/Output.hpp>
#include <DynamicOutput/AsyncLogger.hpp>

namespace RC::Output
{
    std::shared_ptr<AsyncLogger> g_async_logger;
    AsyncConfig g_async_config;

    auto has_internal_error() -> bool
    {
        return File::Internal::StaticStorage::internal_error;
    }

    auto DefaultTargets::set_default_log_level(int32_t log_level) -> void
    {
        default_log_level = log_level;
    }

    auto DefaultTargets::get_default_log_level() -> int32_t
    {
        return default_log_level;
    }

    auto DefaultTargets::close_all_default_devices() -> void
    {
        with_devices_write([](auto& devices) {
            devices.clear();
        });
    }

    auto send(File::StringViewType content) -> void
    {
        DefaultTargets::with_devices_read([&](const auto& devices) {
            for (const auto& device : devices)
            {
                ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(device)

                if (device->has_optional_arg())
                {
                    device->receive_with_optional_arg(content, 0);
                }
                else
                {
                    device->receive(content);
                }
            }
        });
    }

    auto close_all_default_devices() -> void
    {
        DefaultTargets::close_all_default_devices();
    }
} // namespace RC::Output
