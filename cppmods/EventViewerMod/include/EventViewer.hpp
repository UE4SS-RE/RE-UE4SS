#pragma once

// EventViewerMod: UE4SS mod entry + ImGui tab registration.
//
// UE4SS calls start_mod() (see dllmain.cpp) to create this mod instance. On Unreal init we register
// a new ImGui tab and route rendering to Client.

#include <atomic>
#include <Mod/CppUserModBase.hpp>

namespace RC::EventViewerMod
{
    class EventViewerMod : public CppUserModBase
    {
      public:
        EventViewerMod();
        auto on_unreal_init() -> void override;

      private:
        std::atomic_flag m_unreal_loaded{};
    };
} // namespace RC::EventViewerMod
