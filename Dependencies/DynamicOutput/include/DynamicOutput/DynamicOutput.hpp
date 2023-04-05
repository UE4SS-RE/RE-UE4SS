#ifndef RC_DYNAMIC_OUTPUT_HPP
#define RC_DYNAMIC_OUTPUT_HPP

// Include this file when using the default output devices

#include <source_location>                                  // Line numbers etc...
#include <tuple>

#include <DynamicOutput/Macros.hpp>                         // Internal & external utility macros
#include <DynamicOutput/Output.hpp>                         // Core
#include <DynamicOutput/DebugConsoleDevice.hpp>             // stdout
#include <DynamicOutput/FileDevice.hpp>                     // File on drive
#include <DynamicOutput/NewFileDevice.hpp>                  // File on drive that gets deleted and re-created before receiving output
#include <DynamicOutput/TestDevice.hpp>                     // Debugging & developing only, remove later

namespace RC::Output
{
    // Change this to whatever you want the default to be for your entire system
    // You can explicitly specify a different one when using non-default output devices
    using DefaultFileDevice = FileDevice;
}

#endif //RC_DYNAMIC_OUTPUT_HPP
