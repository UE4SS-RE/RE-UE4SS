#include <Mod/CppUserModBase.hpp>
#include <Mod/CppMod.hpp>
#include <GUI/GUITab.hpp>
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

    auto CppUserModBase::register_tab(std::wstring_view tab_name) -> void
    {
        GUITab = std::make_shared<GUI::GUITab>(tab_name, this);
        UE4SSProgram::get_program().add_gui_tab(GUITab);
    }
}
