#pragma once

#include <functional>
#include <memory>
#include <thread>

#include <GUI/Console.hpp>
#include <GUI/GUITab.hpp>
#include <GUI/LiveView.hpp>
#include <Helpers/String.hpp>
#include <imgui.h>

struct ImGuiSettingsHandler;

namespace RC::GUI
{
    class GUITab; // dunno why forward declaration is necessary

    enum class GfxBackend
    {
        DX11,
        GLFW3_OpenGL3,
    };

    enum class OSBackend
    {
        Windows,
    };

    struct WindowSize
    {
        int32_t x;
        int32_t y;
    };

    struct WindowPosition
    {
        int32_t x;
        int32_t y;
    };

    class GfxBackendBase
    {
      protected:
        class OSBackendBase* m_os_backend{};

      public:
        GfxBackendBase() = default;
        virtual ~GfxBackendBase() = default;

      public:
        auto set_os_backend(OSBackendBase* backend)
        {
            m_os_backend = backend;
        }

      public:
        virtual auto init() -> void = 0;
        virtual auto imgui_backend_newframe() -> void = 0;
        virtual auto render(const float clear_color_with_alpha[4]) -> void = 0;
        virtual auto shutdown() -> void = 0;
        virtual auto cleanup() -> void = 0;
        virtual auto create_device() -> bool = 0;
        virtual auto cleanup_device() -> void = 0;
        virtual auto handle_window_resize(int64_t param_1, int64_t param_2) -> void = 0;
        virtual auto on_os_backend_set() -> void = 0;
        virtual auto get_window_size() -> WindowSize
        {
            return {};
        };
        virtual auto get_window_position() -> WindowPosition
        {
            return {};
        };
        virtual inline auto exit_requested() -> bool
        {
            return false;
        };
    };

    class OSBackendBase
    {
      protected:
        class GfxBackendBase* m_gfx_backend{};
        bool m_is_valid{true};

      public:
        OSBackendBase() = default;
        virtual ~OSBackendBase() = default;

      public:
        auto set_gfx_backend(GfxBackendBase* backend)
        {
            m_gfx_backend = backend;
        }
        auto is_valid() -> bool
        {
            return m_is_valid;
        }

      public:
        virtual auto init() -> void = 0;
        virtual auto imgui_backend_newframe() -> void = 0;
        virtual auto create_window(int loc_x = 100, int loc_y = 100, int size_x = 1280, int size_y = 800) -> void = 0;
        virtual auto exec_message_loop(bool* exit_requested) -> void = 0;
        virtual auto shutdown() -> void = 0;
        virtual auto cleanup() -> void = 0;
        virtual auto get_window_handle() -> void* = 0;
        virtual auto get_window_size() -> WindowSize = 0;
        virtual auto get_window_position() -> WindowPosition = 0;
        virtual auto on_gfx_backend_set() -> void = 0;
    };

    class Backend_NoOS : public OSBackendBase
    {
      public:
        inline Backend_NoOS() : OSBackendBase()
        {
            m_is_valid = false;
        }
        ~Backend_NoOS() = default;

      public:
        inline auto init() -> void override
        {
        }
        inline auto imgui_backend_newframe() -> void override
        {
        }
        inline auto create_window(int loc_x, int loc_y, int size_x, int size_y) -> void override
        {
        }
        inline auto exec_message_loop([[maybe_unused]] bool* exit_requested) -> void override
        {
        }
        inline auto shutdown() -> void override
        {
        }
        inline auto cleanup() -> void override
        {
        }
        inline auto get_window_handle() -> void* override
        {
            return nullptr;
        }
        inline auto get_window_size() -> WindowSize override
        {
            return {};
        }
        inline auto get_window_position() -> WindowPosition override
        {
            return {};
        }
        inline auto on_gfx_backend_set() -> void override
        {
        }
    };

    extern ImColor g_imgui_bg_color;
    extern ImColor g_imgui_text_color;
    extern ImColor g_imgui_text_inactive_color;
    extern ImColor g_imgui_text_editor_default_bg_color;
    extern ImColor g_imgui_text_editor_default_text_color;
    extern ImColor g_imgui_text_editor_normal_bg_color;
    extern ImColor g_imgui_text_editor_normal_text_color;
    extern ImColor g_imgui_text_editor_verbose_bg_color;
    extern ImColor g_imgui_text_editor_verbose_text_color;
    extern ImColor g_imgui_text_editor_warning_bg_color;
    extern ImColor g_imgui_text_editor_warning_text_color;
    extern ImColor g_imgui_text_editor_error_bg_color;
    extern ImColor g_imgui_text_editor_error_text_color;
    extern ImColor g_imgui_text_live_view_unreflected_data_color;
    extern ImColor g_imgui_text_green_color;
    extern ImColor g_imgui_text_blue_color;
    extern ImColor g_imgui_text_purple_color;

    class DebuggingGUIBase
    {
      private:
        Console m_console{};

      public:
        virtual ~DebuggingGUIBase() = default;

      public:
        auto get_console() -> Console&
        {
            return m_console;
        };

      public:
        virtual auto setup(std::stop_token&& token) -> void = 0;
    };

    // template<Backend SelectedBackend>
    class DebuggingGUI /* : public DebuggingGUIBase*/
    {
      public:
        using EndOfFrameCallback = std::function<void()>;

        struct WindowSettings
        {
            int pos_x = 100;
            int pos_y = 100;
            int size_x = 1280;
            int size_y = 800;
        };

      private:
        std::unique_ptr<GfxBackendBase> m_gfx_backend{};
        std::unique_ptr<OSBackendBase> m_os_backend{};
        Console m_console{};
        LiveView m_live_view{};
        std::stop_token m_thread_stop_token{};
        bool m_is_open{};
        bool m_exit_requested{};
        std::vector<std::shared_ptr<GUITab>> m_tabs;
        std::mutex m_tabs_mutex;
        std::string m_imgui_ini_file{};
        WindowSettings m_backend_window_settings;

      public:
        bool m_event_thread_busy{};

      private:
        static std::vector<EndOfFrameCallback> s_end_of_frame_callbacks;

      public:
        DebuggingGUI() = default;
        ~DebuggingGUI();

      public:
        auto is_valid() -> bool;
        auto is_open() -> bool
        {
            return m_is_open;
        };
        auto setup(std::stop_token&& token) -> void /* override*/;
        auto get_console() -> Console&
        {
            return m_console;
        };
        auto get_live_view() -> LiveView&
        {
            return m_live_view;
        };
        auto set_gfx_backend(GfxBackend) -> void;
        auto add_tab(std::shared_ptr<GUITab> tab) -> void;
        auto remove_tab(std::shared_ptr<GUITab> tab) -> void;

      private:
        auto on_update() -> void;
        auto main_loop_internal() -> void;

      private:
        // TODO: Move ImGui data saves to their own object
        std::chrono::time_point<std::chrono::steady_clock> m_imgui_last_save = std::chrono::steady_clock::now();
        static auto imgui_ue4ss_data_should_save() -> bool;
        static auto imgui_ue4ss_data_read_open(ImGuiContext*, ImGuiSettingsHandler*, const char* name) -> void*;
        static auto imgui_ue4ss_data_read_line(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) -> void;
        static auto imgui_ue4ss_data_write_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) -> void;

      public:
        static auto execute_at_end_of_frame(EndOfFrameCallback callback) -> void;
    };

    auto gui_thread(std::stop_token stop_token, DebuggingGUI* debugging_ui) -> void;

    // Helper function for executing code that can throw exceptions in the middle of a frame.
    // Moves the exception to the end of the frame so that we can ImGUI errors.
    template <typename CodeToTry>
    auto TRY(CodeToTry code_to_try)
    {
        DebuggingGUI::execute_at_end_of_frame([&] {
            code_to_try();
        });
    }
} // namespace RC::GUI
