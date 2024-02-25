#pragma once

#include <vector>
#include <unordered_map>
#include <Input/KeyDef.hpp>
#include <Input/Common.hpp>

#include <Input/PlatformInputSource.hpp>

namespace RC::Input
{
    class Win32AsyncInputSource : public PlatformInputSource
    {
        private:
            std::vector<const wchar_t*> m_active_window_classes{};
            std::unordered_map<ModifierKey, bool> m_modifier_keys_down{};
            bool m_any_keys_are_down{};
            bool m_activated {false};
            std::array<bool, max_keys> m_key_down{};

        private:
            /// SAFETY: Only update and return m_input_events 
            /// in the process_event function
            std::vector<InputEvent> m_input_events{};
        public:
            template <typename... WindowClasses>
          explicit Win32AsyncInputSource(WindowClasses... window_classes)
            {
                static_assert(std::conjunction<std::is_same<const wchar_t*, WindowClasses>...>::value, "WindowClasses must be of type const wchar_t*");

                m_modifier_keys_down.emplace(ModifierKey::SHIFT, false);
                m_modifier_keys_down.emplace(ModifierKey::CONTROL, false);
                m_modifier_keys_down.emplace(ModifierKey::ALT, false);

                register_window_classes(window_classes...);
            }

        private:
            template <typename WindowClass>
            auto register_window_classes(WindowClass window_class) -> void
            {
                m_active_window_classes.emplace_back(window_class);
            }

            template <typename WindowClass, typename... WindowClasses>
            auto register_window_classes(WindowClass window_class, WindowClasses... window_classes) -> void
            {
                m_active_window_classes.emplace_back(window_class);
                register_window_classes(window_classes...);
            }

            auto are_modifier_keys_down(const std::vector<ModifierKey>&) -> bool;
            auto is_program_focused() -> bool;  

        public:
            bool is_available() override { return true; };

            bool activate() override { m_key_down.fill(false); return m_activated = true; };

            bool deactivate() override { m_activated = false; return true; };
            
            std::vector<InputEvent>& process_event(Handler* handler) override;
            ~Win32AsyncInputSource() = default;
            int source_priority() override { return 0; }

            const char* get_name() override { return "Win32Async"; }
    };
};
