#include <array>
#include <climits>
#include <cstring>

#include <Input/Handler.hpp>
#include <Input/PlatformInputSource.hpp>
namespace RC::Input
{
    std::unordered_map<std::string, std::shared_ptr<PlatformInputSource>> Handler::m_input_sources_store;

    auto Handler::process_event() -> void
    {
        if (m_platform_handler == nullptr)
        {
            return;
        }

        std::vector<EventCallbackCallable> callbacks_to_call{};

        auto events = m_platform_handler->process_event(this);

        {
            // Lock the event mutex to access the key_set
            auto event_update_lock = std::lock_guard(m_event_mutex);
            for (auto& event : events)
            {
                auto key_set_array = m_key_set.key_data[event.key];
                for (auto& key_data : key_set_array)
                {
                    if (key_data.required_modifier_keys == event.modifier_keys)
                    {
                        callbacks_to_call.emplace_back(key_data.callback);
                    }
                }
            }
        }

        // No need to lock the event mutex to call the callbacks
        // this avoids key registration inside the callback
        for (const auto& callback : callbacks_to_call)
        {
            callback();
        }
    }

    auto Handler::register_keydown_event(Input::Key key, EventCallbackCallable callback, uint8_t custom_data, void* custom_data2) -> void
    {
        auto event_update_lock = std::lock_guard(m_event_mutex);
        KeyData& key_data = m_key_set.key_data[key].emplace_back();
        key_data.callback = callback;
        key_data.custom_data = custom_data;
        key_data.custom_data2 = custom_data2;
        m_subscribed_keys[key] = true;
    }

    auto Handler::register_keydown_event(
            Input::Key key, const ModifierKeyArray& modifier_keys, const EventCallbackCallable& callback, uint8_t custom_data, void* custom_data2) -> void
    {
        auto event_update_lock = std::lock_guard(m_event_mutex);
        KeyData& key_data = m_key_set.key_data[key].emplace_back();
        key_data.callback = callback;
        key_data.custom_data = custom_data;
        key_data.custom_data2 = custom_data2;
        key_data.requires_modifier_keys = true;
        key_data.required_modifier_keys = modifier_keys;
        m_subscribed_keys[key] = true;
    }

    auto Handler::is_keydown_event_registered(Input::Key key) -> bool
    {
        auto event_update_lock = std::lock_guard(m_event_mutex);
        auto key_data = m_key_set.key_data.find(key);
        if (key_data == m_key_set.key_data.end())
        {
            return false;
        }
        for (const auto& key_data_container : key_data->second)
        {
            if (key_data_container.required_modifier_keys.empty())
            {
                return true;
            }
        }
        return false;
    }

    auto Handler::is_keydown_event_registered(Input::Key key, const ModifierKeyArray& modifier_keys_array) -> bool
    {
        auto event_update_lock = std::lock_guard(m_event_mutex);
        auto key_data = m_key_set.key_data.find(key);
        auto modifier_keys = ModifierKeys(modifier_keys_array);
        if (key_data == m_key_set.key_data.end())
        {
            return false;
        }
        for (const auto& key_data_container : key_data->second)
        {
            if (key_data_container.required_modifier_keys == modifier_keys)
            {
                return true;
            }
        }
        return false;
    }

    auto Handler::has_event_on_key(Input::Key key) -> bool
    {
        return m_subscribed_keys[key];
    }

    auto Handler::get_events_safe(std::function<void(KeySet&)> callback) -> void
    {
        auto event_update_lock = std::lock_guard(m_event_mutex);
        callback(m_key_set);
    }

    auto Handler::clear_subscribed_keys() -> void
    {
        m_subscribed_keys.fill(false);
    }

    auto Handler::clear_subscribed_key(Key k) -> void
    {
        m_subscribed_keys[k] = false;
    }

    auto Handler::get_allow_input() -> bool
    {
        return m_allow_input;
    }

    auto Handler::set_allow_input(bool new_value) -> void
    {
        m_allow_input = new_value;
    }

    /// Set the input source to the given source
    /// SAFETY: Only call this function from the main thread
    auto Handler::set_input_source(std::string source) -> bool
    {
        auto event_update_lock = std::lock_guard(m_event_mutex);
        std::shared_ptr<PlatformInputSource> next_input_source = nullptr;
        if (source == "Default")
        {
            // find the highest priority input source
            int highest_priority = INT_MAX;
            for (auto& input_source : m_input_sources_store)
            {
                if (!input_source.second->is_available())
                {
                    continue;
                }
                auto priority = input_source.second->source_priority();
                if (priority < highest_priority)
                {
                    next_input_source = input_source.second;
                    highest_priority = priority;
                }
            }
            if (highest_priority == INT_MAX)
            {
                return false;
            }
        }
        else
        {
            auto input_source = m_input_sources_store.find(source);
            if (input_source == m_input_sources_store.end())
            {
                return false;
            }
            next_input_source = input_source->second;
            if (!next_input_source->is_available())
            {
                return false;
            }
        }
        if (next_input_source != m_platform_handler)
        {
            if (m_platform_handler != nullptr)
            {
                m_platform_handler->deactivate();
            }
            next_input_source->activate();
            m_platform_handler = next_input_source;
            return true;
        }
        return true;
    }

    // register the input source to the input source store
    auto Handler::register_input_source(std::shared_ptr<PlatformInputSource> input_source) -> void
    {
        std::string name = input_source->get_name();
        if (m_input_sources_store.find(name) == m_input_sources_store.end())
        {
            m_input_sources_store[name] = input_source;
        }
    }

    auto Handler::get_current_input_source() -> std::string
    {
        if (m_platform_handler == nullptr)
        {
            return "None";
        }
        return m_platform_handler->get_name();
    }
} // namespace RC::Input
