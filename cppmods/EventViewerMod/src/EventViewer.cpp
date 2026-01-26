#include <EventViewer.hpp>

// EventViewerMod: UE4SS mod entry point and ImGui tab wiring.
//
// This is the glue between UE4SS' mod lifecycle (start_mod/on_unreal_init) and our
// ImGui renderer + middleware.

#include <Unreal/Hooks/Hooks.hpp>
#include <Client.hpp>
#include <UE4SSProgram.hpp>
#include <UEngine.hpp>

namespace RC::EventViewerMod
{
    EventViewerMod::EventViewerMod()
    {
        ModName = STR("EventViewerMod");
        ModAuthors = STR("wildcherry");
        ModDescription = STR(
            "Lets you view a live call stack of ProcessEvent and ProcessInternal, with additional call frequency tracking.");
        ModVersion = STR("1.0.0");

        register_tab(STR("EventViewer"), [](CppUserModBase* mod)
        {
            UE4SS_ENABLE_IMGUI();

            // Avoid unnecessary expensive dynamic_cast
            ImGui::BeginDisabled(!static_cast<EventViewerMod*>(mod)->m_unreal_loaded.test()); // NOLINT(*-pro-type-static-cast-downcast)
            auto& style = ImGui::GetStyle();
            const auto old_size = style.GrabMinSize;
            style.GrabMinSize = 30.0f;
            Client::GetInstance().render();
            style.GrabMinSize = old_size;
            ImGui::EndDisabled();
        });
    }

    void EventViewerMod::on_unreal_init()
    {
        Unreal::Hook::RegisterEngineTickPreCallback(
            [this](auto&, Unreal::UEngine*, float, bool)
            {
                m_unreal_loaded.test_and_set();
            },
            {true, true, STR("EventViewerMod"), STR("InstallHook")});
    }
} // namespace RC::EventViewerMod
