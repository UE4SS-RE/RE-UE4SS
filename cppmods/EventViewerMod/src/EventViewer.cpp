#include <EventViewer.hpp>

// EventViewerMod: UE4SS mod entry point and ImGui tab wiring.
//
// This is the glue between UE4SS' mod lifecycle (start_mod/on_unreal_init) and our
// ImGui renderer + middleware.

#include <Unreal/Hooks/Hooks.hpp>
#include <Client.hpp>
#include <DynamicOutput/Output.hpp>

#include "UEngine.hpp"

namespace RC::EventViewerMod
{

    EventViewerMod::EventViewerMod()
    {
        ModName = STR("EventViewerMod");
        ModAuthors = STR("wildcherry");
        ModDescription = STR("Lets you view a live call stack of ProcessEvent and ProcessInternal, with additional call frequency tracking.");
        ModVersion = STR("1.0.0");
    }

    void EventViewerMod::on_unreal_init()
    {
        Unreal::Hook::RegisterEngineTickPreCallback(
                [this](auto&, Unreal::UEngine* e, float, bool) {
                    register_tab(STR("EventViewer"), [](CppUserModBase* mod) {
                        UE4SS_ENABLE_IMGUI();
                        auto& style = ImGui::GetStyle();
                        const auto old_size = style.GrabMinSize;
                        style.GrabMinSize = 30.0f;
                        Client::GetInstance().render();
                        style.GrabMinSize = old_size;
                    });
                    // e->GetNamePrivate().GetComparisonIndex();
                    Output::send<LogLevel::Verbose>(STR("Installed EventViewerMod GUI!"));
                },
                {true, true, STR("EventViewerMod"), STR("InstallHook")});
    }
} // namespace RC::EventViewerMod