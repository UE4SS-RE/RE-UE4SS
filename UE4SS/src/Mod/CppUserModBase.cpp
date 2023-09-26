#include <Mod/CppMod.hpp>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

namespace RC
{
    CppUserModBase::CppUserModBase()
    {
        if (ModIntendedSDKVersion.empty())
        {
            ModIntendedSDKVersion = std::format(STR("{}.{}.{}"), UE4SS_LIB_VERSION_MAJOR, UE4SS_LIB_VERSION_MINOR, UE4SS_LIB_VERSION_HOTFIX);
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
    }

    auto CppUserModBase::register_tab(std::wstring_view tab_name, GUI::GUITab::RenderFunctionType render_function) -> void
    {
        auto& tab = GUITabs.emplace_back(std::make_shared<GUI::GUITab>(tab_name, render_function, this));
        UE4SSProgram::get_program().add_gui_tab(tab);
    }
} // namespace RC
