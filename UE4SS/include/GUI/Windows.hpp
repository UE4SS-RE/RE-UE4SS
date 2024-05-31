#pragma once

#include <GUI/GUI.hpp>

namespace RC::GUI
{
    class Backend_Windows : public OSBackendBase
    {
      public:
        ~Backend_Windows() = default;

      public:
        auto init() -> void override;
        auto imgui_backend_newframe() -> void override;
        auto create_window(int Loc_X, int Loc_Y, int Size_X, int Size_Y) -> void override;
        auto exec_message_loop(bool* exit_requested) -> void override;
        auto shutdown() -> void override;
        auto cleanup() -> void override;
        auto get_window_handle() -> void* override;
        auto get_window_size() -> WindowSize override;
        auto get_window_position() -> WindowPosition override;
        auto on_gfx_backend_set() -> void override;
    };
} // namespace RC::GUI
