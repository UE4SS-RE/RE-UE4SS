#pragma once

#include <GUI/GUI.hpp>

namespace RC::GUI
{
    class Backend_DX11 : public GfxBackendBase
    {
      public:
        ~Backend_DX11() = default;

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
    };
} // namespace RC::GUI
