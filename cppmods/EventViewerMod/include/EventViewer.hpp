#pragma once
#include <concurrentqueue.h>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

namespace RC::EventViewerMod
{
    // this goes in dllmain, renderer/helper structs go here in RC::GUI::EventViewerMod
    class EventViewerMod : RC::CppUserModBase
    {
    public:
        EventViewerMod();
        auto on_unreal_init() -> void override;
        auto on_ui_init() -> void override;
    };
}