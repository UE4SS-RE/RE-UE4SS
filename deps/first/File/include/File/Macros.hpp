#pragma once

#include <String/StringType.hpp>

#define THROW_INTERNAL_FILE_ERROR(msg)                                                                                                                         \
    RC::File::Internal::StaticStorage::internal_error = true;                                                                                                  \
    throw std::runtime_error{msg};

// This macro needs to be moved into the DynamicOutput system
/*
#define ASSERT_DEFAULT_OUTPUT_DEVICE_IS_VALID(param_device) \
                if (DefaultTargets::get_default_devices_ref().empty() || !param_device){                       \
                THROW_INTERNAL_OUTPUT_ERROR("[Output::send] Attempted to send but there were no default devices. Please specify at least one default device or
construct a Targets object and supply your own devices.") \
                }
//*/

namespace RC::File
{
    using StringType = RC::StringType;
    using StringViewType = RC::StringViewType;
    using CharType = RC::CharType;
    using StreamIType = RC::StreamIType;
    using StreamOType = RC::StreamOType;
    using StreamType = StreamIType;
} // namespace RC::File
