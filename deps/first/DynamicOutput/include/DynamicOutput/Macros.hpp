#ifndef DYNAMIC_OUTPUT_MACROS_HPP
#define DYNAMIC_OUTPUT_MACROS_HPP

#define ENABLE_OUTPUT_DEVICE_DEBUG_MODE 0

#define ASSERT_OUTPUT_DEVICE_IS_VALID(param_device)                                                                                                            \
    if (!param_device)                                                                                                                                         \
    {                                                                                                                                                          \
        THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send to a non-valid device")                                                                    \
    }

#define ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(param_device)                                                                                                    \
    if (DefaultTargets::get_default_devices_ref().empty() || !param_device)                                                                                    \
    {                                                                                                                                                          \
        THROW_INTERNAL_FILE_ERROR("[Output::send] Attempted to send but there were no default devices. Please specify at least one default device or "         \
                                  "construct a Targets object and supply your own devices.")                                                                   \
    }

#endif // DYNAMIC_OUTPUT_MACROS_HPP
