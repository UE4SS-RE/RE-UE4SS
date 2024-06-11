#include <Mod/CppMod.hpp>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

#include <vector>

namespace RC
{
    CppUserModBase::CppUserModBase()
    {
        if (ModIntendedSDKVersion.empty())
        {
            ModIntendedSDKVersion = to_ue(std::format(SYSSTR("{}.{}.{}"), UE4SS_LIB_VERSION_MAJOR, UE4SS_LIB_VERSION_MINOR, UE4SS_LIB_VERSION_HOTFIX));
        }
    }

    CppUserModBase::~CppUserModBase()
    {
        for (const auto& tab : GUITabs)
        {
            if (tab)
            {
                UE4SSProgram::get_program().remove_gui_tab(tab);
            }
        }
        GUITabs.clear();

        UE4SSProgram::get_program().m_input_handler.get_events_safe([&](auto& key_set) {
            std::erase_if(key_set.key_data, [&](auto& item) -> bool {
                auto& [_, key_data] = item;
                bool were_all_events_registered_from_this_mod = true;
                std::erase_if(key_data, [&](Input::KeyData& key_data) -> bool {
                    // custom_data == 1: Bind came from Lua, and custom_data2 is nullptr.
                    // custom_data == 2: Bind came from C++, and custom_data2 is a pointer to KeyDownEventData. Must free it.
                    auto event_data = static_cast<KeyDownEventData*>(key_data.custom_data2);
                    if (key_data.custom_data == 2 && event_data && event_data->mod == this)
                    {
                        delete event_data;
                        return true;
                    }
                    else
                    {
                        were_all_events_registered_from_this_mod = false;
                        return false;
                    }
                });

                return were_all_events_registered_from_this_mod;
            });
        });
    }

    auto CppUserModBase::register_tab(UEStringViewType tab_name, GUI::GUITab::RenderFunctionType render_function) -> void
    {
        auto& tab = GUITabs.emplace_back(std::make_shared<GUI::GUITab>(tab_name, render_function, this));
        UE4SSProgram::get_program().add_gui_tab(tab);
    }

    auto CppUserModBase::register_keydown_event(Input::Key key, const Input::EventCallbackCallable& callback, uint8_t custom_data) -> void
    {
        UE4SSProgram::get_program().register_keydown_event(key, callback, 2, new KeyDownEventData{custom_data, this});
    }

    auto CppUserModBase::register_keydown_event(Input::Key key,
                                                const Input::Handler::ModifierKeyArray& callback,
                                                const Input::EventCallbackCallable& modifier_keys,
                                                uint8_t custom_data) -> void
    {
        UE4SSProgram::get_program().register_keydown_event(key, callback, modifier_keys, 2, new KeyDownEventData{custom_data, this});
    }
} // namespace RC
