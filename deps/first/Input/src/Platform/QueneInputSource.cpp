#include <Input/PlatformInputSource.hpp>
#include <Input/Platform/QueueInputSource.hpp>

#include <DynamicOutput/Output.hpp>

namespace RC::Input
{
    auto QueueInputSource::push_input_event(const InputEvent& event) -> void
    {
        if (m_activated) {
            m_input_queue.push(event);
            Output::send<LogLevel::Default>(SYSSTR("QueueInputSource::push_input_event: {}"), (int) event.key);
        }
    }

    // even if not activated, we still consume the remaining events in the queue
    std::vector<InputEvent>& QueueInputSource::process_event(Handler* handler)
    {
        m_input_events.clear();
        
        auto event = m_input_queue.pop();
        auto& key_set = handler->get_subscribed_keys();
        while (event)
        {
            Output::send<LogLevel::Default>(SYSSTR("QueueInputSource::reveive key event: {}"), (int) event->key);
            if (key_set[event->key])
            {
                m_input_events.emplace_back(*event);
            }
            event = m_input_queue.pop();
        }

        if (!m_activated) {
            m_input_events.clear();
        }

        if (!handler->get_allow_input()) {
            m_input_events.clear();
        }

        return m_input_events;
    }
}; // RC::Input
