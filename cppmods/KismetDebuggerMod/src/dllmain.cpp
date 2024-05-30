#include <string>

#include <GUI/GUITab.hpp>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <KismetDebugger.hpp>

class KismetDebuggerMod : public RC::CppUserModBase
{
private:
    RC::GUI::KismetDebuggerMod::Debugger m_debugger{};

public:
    KismetDebuggerMod() : CppUserModBase()
    {
        ModName = STR("KismetDebugger");
        ModVersion = STR("1.0");
        ModDescription = STR("Debugging interface for kismet bytecode");
        ModAuthors = STR("truman");

        register_tab(STR("Kismet Debugger"), [](CppUserModBase* mod) {
            UE4SS_ENABLE_IMGUI()
            dynamic_cast<KismetDebuggerMod*>(mod)->m_debugger.render();
        });
    }

    ~KismetDebuggerMod() override = default;
};

#define KISMET_DEBUGGER_MOD_API __declspec(dllexport)
extern "C"
{
    KISMET_DEBUGGER_MOD_API RC::CppUserModBase* start_mod()
    {
        return new KismetDebuggerMod();
    }

    KISMET_DEBUGGER_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}

