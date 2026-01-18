#pragma once


// EventViewerMod: UE4SS mod entry + ImGui tab registration.
//
// UE4SS calls start_mod() (see dllmain.cpp) to create this mod instance. On Unreal init we register
// a new ImGui tab and route rendering to Client.

#include <concurrentqueue.h>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

namespace RC::EventViewerMod
{
    class EventViewerMod : public RC::CppUserModBase
    {
    public:
        EventViewerMod();
        auto on_unreal_init() -> void override;
    };
}