#include <DynamicOutput/Output.hpp>

namespace RC::Output
{
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

    auto DefaultTargets::get_default_devices_ref() -> OutputDevicesContainerType&
    {
        return default_devices;
    }

    auto DefaultTargets::close_all_default_devices() -> void
    {
        // clear() will empty the container and will also call all the destructors
        default_devices.clear();
    }

    auto send(File::StringViewType content) -> void
    {
        for (const auto& device : DefaultTargets::get_default_devices_ref())
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
    }

    auto close_all_default_devices() -> void
    {
        DefaultTargets::close_all_default_devices();
    }
}
