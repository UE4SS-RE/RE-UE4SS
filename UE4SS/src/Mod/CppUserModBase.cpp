#include <Mod/CppMod.hpp>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

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
#ifdef HAS_UI
        for (const auto& tab : GUITabs)
        {
            if (tab)
            {
                UE4SSProgram::get_program().remove_gui_tab(tab);
            }
        }
        GUITabs.clear();
#endif
    }

#ifdef HAS_UI
    auto CppUserModBase::register_tab(UEStringViewType tab_name, GUI::GUITab::RenderFunctionType render_function) -> void
    {
        auto& tab = GUITabs.emplace_back(std::make_shared<GUI::GUITab>(tab_name, render_function, this));
        UE4SSProgram::get_program().add_gui_tab(tab);
    }
#endif
} // namespace RC
