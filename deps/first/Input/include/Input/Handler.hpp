#ifndef IO_INPUT_HANDLER_HPP
#define IO_INPUT_HANDLER_HPP

// TODO: Abstract more... need to get rid of Windows.h from InputHandler.cpp

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <array>
#include <mutex>

#include <Input/Common.hpp>
#include <Input/KeyDef.hpp>

namespace RC::Input
{
    using EventCallbackCallable = std::function<void()>;

    struct InputEvent
    {
        Key key;
        ModifierKeys modifier_keys{};
    };

    struct KeyData
    {
        ModifierKeys required_modifier_keys{};
        EventCallbackCallable callback{};
        uint8_t custom_data{};
        void* custom_data2{};
        bool requires_modifier_keys{};
        bool is_down{};
    };

    struct KeySet
    {
        std::unordered_map<Key, std::vector<KeyData>> key_data;
    };

    class PlatformInputSource;
    class RC_INPUT_API Handler
    {
      private:
        // std::vector<KeySet> m_key_sets{};
        KeySet m_key_set{};
        bool m_allow_input{true};
        std::array<bool, max_keys> m_subscribed_keys{};

        std::shared_ptr<PlatformInputSource> m_platform_handler;
        std::mutex m_event_mutex;

      public:
        Handler(){};

        // Input source and event processing
      public:
        auto set_input_source(std::string source) -> bool;
        auto process_event() -> void;

        // Interfaces for UE4SS and ModSystem for event registration
      public:
        auto init() -> void;

        auto register_keydown_event(Input::Key, EventCallbackCallable, uint8_t custom_data = 0, void* custom_data2 = nullptr) -> void;

        using ModifierKeyArray = std::array<Input::ModifierKey, max_modifier_keys>;
        auto register_keydown_event(Input::Key, const ModifierKeyArray&, const EventCallbackCallable&, uint8_t custom_data = 0, void* custom_data2 = nullptr)
                -> void;

        auto is_keydown_event_registered(Input::Key) -> bool;
        auto is_keydown_event_registered(Input::Key, const ModifierKeyArray&) -> bool;

        auto get_events_safe(std::function<void(KeySet&)>) -> void;
        auto clear_subscribed_keys() -> void;
        auto clear_subscribed_key(Key k) -> void;

        auto has_event_on_key(Input::Key key) -> bool;
        auto get_subscribed_keys() const -> const std::array<bool, max_keys>&
        {
            return m_subscribed_keys;
        }

        auto get_allow_input() -> bool;
        auto set_allow_input(bool new_value) -> void;

        auto get_current_input_source() -> std::string;

      private:
        static std::unordered_map<std::string, std::shared_ptr<PlatformInputSource>> m_input_sources_store;
        static auto register_input_source(std::shared_ptr<PlatformInputSource> input_source) -> void;

      public:
        static auto get_input_source(std::string source) -> std::shared_ptr<PlatformInputSource>
        {
            if (m_input_sources_store.find(source) != m_input_sources_store.end())
            {
                return m_input_sources_store[source];
            }
            return nullptr;
        }
    };
} // namespace RC::Input

#endif // IO_INPUT_HANDLER_HPP
