#pragma once

#include <Input/Handler.hpp>

#include <optional>
#include <vector>

namespace RC::Input
{

    class PlatformInputSource
    {
      public:
        /// Check if the input source is available
        virtual bool is_available()
        {
            return false;
        };

        /// Initialize the input source
        virtual bool activate()
        {
            return false;
        };

        /// Initialize the input source
        virtual bool deactivate()
        {
            return false;
        };

        /// Get the priority of the input source, smaller number means higher priority
        virtual int source_priority()
        {
            return 999;
        };

        /// Process the event and return the input events in the frame
        virtual std::vector<InputEvent>& process_event(Handler* handler) = 0;

        virtual ~PlatformInputSource() = default;

        virtual const char* get_name()
        {
            return "Unknown";
        }
    };

}; // namespace RC::Input
