#pragma once

#include <GUI/GUI.hpp>

namespace RC::GUI
{
    class Backend_TUI : public OSBackendBase
    {
      public:
        auto init() -> void override;
        auto imgui_backend_newframe() -> void override;
        auto create_window() -> void override;
        auto exec_message_loop(bool* exit_requested) -> void override;
        auto shutdown() -> void override;
        auto cleanup() -> void override;
        auto get_window_handle() -> void* override;
        auto get_window_size() -> WindowSize override;
        auto on_gfx_backend_set() -> void override;
    };

    class Backend_GfxTUI : public GfxBackendBase
    {
      public:
        ~Backend_GfxTUI() = default;

      public:
        auto init() -> void override;
        auto imgui_backend_newframe() -> void override;
        auto render(const float clear_color_with_alpha[4]) -> void override;
        auto shutdown() -> void override;
        auto cleanup() -> void override;
        auto create_device() -> bool override;
        auto cleanup_device() -> void override;
        auto handle_window_resize(int64_t param_1, int64_t param_2) -> void override;
        auto on_os_backend_set() -> void override;
        auto get_window_size() -> WindowSize override;
        auto exit_requested() -> bool override;
        auto set_backend_properties(BackendProperty &properties) -> void override;
    };
} // namespace RC::GUI
