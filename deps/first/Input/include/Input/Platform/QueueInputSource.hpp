#pragma once

#include <Input/PlatformInputSource.hpp>
#include <Input/KeyDef.hpp>

#include <Input/RingBuffer.hpp>

namespace RC::Input
{
    class QueueInputSource : public PlatformInputSource
    {
        private:
            static constexpr int max_inputs = 256;
            RingBufferSPSC<InputEvent, max_inputs> m_input_queue;
        protected:
            bool m_activated {false};
        private:
            /// SAFETY: Only update and return m_input_events 
            /// in process_event and flush_events functions 
            std::vector<InputEvent> m_input_events;
        public:
            ~QueueInputSource() = default;
            
            // QueeueInputSource is not a implemented input source
            // and should not be used directly
            bool is_available() override { return false; };

            bool activate() override { return m_activated = true; };

            bool deactivate() override { m_activated = false; return true; };

            std::vector<InputEvent>& process_event(Handler* handler) override;

            int source_priority() override { return 0; }

            const char* get_name() override { return "Queue"; }

        public:
            auto push_input_event(const InputEvent& event) -> void;
    };

}; // RC::Input