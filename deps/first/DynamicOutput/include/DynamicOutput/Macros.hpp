#ifndef DYNAMIC_OUTPUT_MACROS_HPP
#define DYNAMIC_OUTPUT_MACROS_HPP

#include <Helpers/String.hpp>

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

#ifdef _WIN32
#define RC_DEVICE_PRINT_FUNC(fmt_var, optional_arg, optional_prefix) \
wprintf_s(STR("%hs%hs%ls\033[0m"), optional_prefix, log_level_to_color(static_cast<Color::Color>(optional_arg)).c_str(), m_formatter(fmt_var).c_str());
#elif __linux__
#define RC_DEVICE_PRINT_FUNC(fmt_var, optional_arg, optional_prefix) \
const auto as_utf8 = to_utf8_string(m_formatter(fmt_var)); \
const auto color_as_utf8 = to_utf8_string(log_level_to_color(static_cast<Color::Color>(optional_arg))); \
printf("%s%s%s\033[0m", optional_prefix, color_as_utf8.c_str(), as_utf8.c_str());
#endif

#endif // DYNAMIC_OUTPUT_MACROS_HPP
