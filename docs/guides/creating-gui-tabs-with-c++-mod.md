# Creating GUI tabs with a C++ mod

> UE4SS already includes the ImGui library to render its console GUI, built from the [UE4SS-RE/imgui](https://github.com/UE4SS-RE/imgui) repo. Refer to [ImGui documentation in that repo](https://github.com/UE4SS-RE/imgui/tree/master/docs) on how to use ImGui-specific classes and methods for rendering actual buttons and textboxes and other window objects.

This guide will show how you create custom tabs for the GUI with a C++ mod, and the guide will take the form of comments in the code example below:
```c++
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

class MyAwesomeMod : public RC::CppUserModBase
{
private:
    int m_private_number{33};
    std::shared_ptr<GUI::GUITab> m_less_safe_tab{};

public:
    MyAwesomeMod() : CppUserModBase()
    {
        ModName = STR("MyAwesomeMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is my awesome mod");
        ModAuthors = STR("UE4SS Team");
        
        // The 'register_tab' function will tell UE4SS to render a tab.
        // Tabs registered this way will be automatically cleaned up when this C++ mod is destructed.
        // The first param is the display name of your tab.
        // The second param is a callback that UE4SS will use to render the contents of the tab.
        // The param to the callback is a pointer to your mod.
        register_tab(STR("My Test Tab"), [](CppUserModBase* instance) {
            // In this callback, you can start rendering the contents of your tab with ImGui. 
            ImGui::Text("This is the contents of the tab");
            
            // You can access members of your mod class with the 'instance' param.
            auto mod = dynamic_cast<MyAwesomeMod*>(instance);
            if (!mod)
            {
                // Something went wrong that caused the 'instance' to not be correctly set.
                // Let's abort the rest of the function so that you don't access an invalid pointer.
                return;
            }
            
            // You can access both public and private members.
            mod->render_some_stuff(mod->m_private_number);
        });
        
        // The 'UE4SSProgram::add_gui_tab' function is another way to tell UE4SS to render a tab.
        // This way of registering a tab will make you responsible for cleaning up the tab when your mod destructs.
        // Failure to clean up the tab on mod destruction will result in a crash.
        // It's recommended that you use 'register_tab' instead of this function.
        m_less_safe_tab = std::make_shared<GUI::GUITab>(STR("My Less Safe Tab"), [](CppUserModBase* instance) {
            // This callback is identical to the one used with 'register_tab' except 'instance' is always nullptr.
            ImGui::Text("This is the contents of the less safe tab");
        });
        UE4SSProgram::get_program().add_gui_tab(m_less_safe_tab);
    }

    ~MyAwesomeMod() override
    {
        // Because you created a tab with 'UE4SSProgram::add_gui_tab', you must manually remove it.
        // Failure to remove the tab will result in a crash.
        UE4SSProgram::get_program().remove_gui_tab(m_less_safe_tab);
    }
    
    auto on_ui_init() -> void override
    {
        // It's critical that you enable ImGui if you intend to use ImGui within the context of UE4SS.
        // If you don't do this, a crash will occur as soon as ImGui tries to render anything, for example in your tab.
        UE4SS_ENABLE_IMGUI()
    }
    
    auto render_some_stuff(int Number) -> void
    {
        auto calculated_value = Number + 1;
        ImGui::Text(std::format("calculated_value: {}", calculated_value).c_str());
    }
};

#define MY_AWESOME_MOD_API __declspec(dllexport)
extern "C"
{
    MY_AWESOME_MOD_API RC::CppUserModBase* start_mod()
    {
        return new MyAwesomeMod();
    }

    MY_AWESOME_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
```
