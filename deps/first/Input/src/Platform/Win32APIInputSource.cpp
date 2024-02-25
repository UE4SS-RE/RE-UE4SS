#include <Input/Handler.hpp>
#include <Input/KeyDef.hpp>
#include <Input/Common.hpp>
#include <Input/Platform/Win32APIInputSource.hpp>

#define NOMINMAX
#include <Windows.h>

auto Win32InputSource::is_program_focused() -> bool
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    wchar_t current_window_class_name[MAX_PATH];
    if (!GetClassNameW(hwnd, current_window_class_name, MAX_PATH)) return false;
    for (const auto& active_window_class : m_active_window_classes)
    {Win32InputSourceWin32InputSource
        if (wcscmp(current_window_class_name, active_window_class) == 0)
        {
            return true;
        }
    }
    return false;
}

std::vector<InputEvent>& Win32InputSource::process_event(Handler* handler) 
{
    if (!is_program_focused())
    {
        return std::nullopt;
    }

    m_input_events.clear();
    
    if (! m_activated)
    {
        return m_input_events;
    }
    
    bool skip_this_frame = !handler->get_allow_input();

    if (m_any_keys_are_down)
    {
        skip_this_frame = true;
    }
    
    bool any_keys_are_down = false;

    // Check if any modifier keys are down
    ModifierKeys modifier_keys{};
    for (auto& [modifier_key, key_is_down] : m_modifier_keys_down)
    {
        if (GetAsyncKeyState(modifier_key))
        {
            modifier_keys |= modifier_key;
            key_is_down = true;
        }
        else
        {
            key_is_down = false;
        }
    }

    auto& subscribed_keys = handler->get_subscribed_keys();
    
    for (int key = 0; key < max_keys; ++key)
    {
        if (subscribed_keys[key])
        {
            auto keyed = GetAsyncKeyState(key);
            if (keyed && !m_key_down[key])
            {
                any_keys_are_down = true;
                m_key_down[key] = true;
                m_input_events.emplace_back({static_cast<Key>(key), modifier_keys});
            } 
            else if (!keyed && m_key_down[key])
            {
                m_key_down[key] = false;
            }
        }
    }
    
    
    if (any_keys_are_down)
    {
        m_any_keys_are_down = true;
    }
    else
    {
        m_any_keys_are_down = false;
    }
    
    if (skip_this_frame)
    {
        m_input_events.clear();
    }
    return m_input_events;
}
