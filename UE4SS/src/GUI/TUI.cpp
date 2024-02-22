#include <GUI/TUI.hpp>

#include <imtui/imtui.h>

#include <imtui/imtui-impl-text.h>
#include <imtui/imtui-impl-ncurses.h>

#include <cstdio>

namespace RC::GUI
{
    static ImTui::TScreen * g_screen = nullptr;

    void Backend_TUI::init() {
        //fprintf(stderr, "Backend_TUI::init\n");
        g_screen = ImTui_ImplNcurses_Init(true);

        // disable stderr
        freopen("./tui.log", "w+", stderr);
        setbuf(stderr, NULL);
        fprintf(stderr, "Backend_TUI::init\n");
        fflush(stderr);
        atexit([]() {
            // ensure that the screen is destroyed
            ImTui_ImplNcurses_Shutdown();
            g_screen = nullptr;
        });
    }

    void Backend_TUI::imgui_backend_newframe() {
        //fprintf(stderr, "Backend_TUI::imgui_backend_newframe\n");
        ImTui_ImplNcurses_NewFrame();
    }

    void Backend_TUI::create_window() {
        // Do nothing
    }

    void Backend_TUI::exec_message_loop(bool* exit_requested) {
        ImTui_ImplNcurses_ProcessEvent();
        *exit_requested = false;
    }

    void Backend_TUI::shutdown() {
        ImTui_ImplNcurses_Shutdown();
        g_screen = nullptr;
    }

    void Backend_TUI::cleanup() {
        // Do nothing
    }

    void* Backend_TUI::get_window_handle() {
        return g_screen;
    }

    WindowSize Backend_TUI::get_window_size() {
        if (g_screen) {
            return {g_screen->nx, g_screen->ny};
        } else {
            return {0, 0};
        }
    }

    void Backend_TUI::on_gfx_backend_set() {
        // Do nothing
    }

    void Backend_GfxTUI::init() {
        ImTui_ImplText_Init();
    }

    void Backend_GfxTUI::imgui_backend_newframe() {
        ImTui_ImplText_NewFrame();
    }

    void Backend_GfxTUI::render(const float clear_color_with_alpha[4]) {
        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
        ImTui_ImplNcurses_DrawScreen();
    }

    void Backend_GfxTUI::shutdown() {
        ImTui_ImplText_Shutdown();
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    void Backend_GfxTUI::cleanup() {
        // Do nothing
    }

    bool Backend_GfxTUI::create_device() {
        return true;
    }

    void Backend_GfxTUI::cleanup_device() {
        // Do nothing
    }

    void Backend_GfxTUI::handle_window_resize(int64_t param_1, int64_t param_2) {
        // Do nothing
    }

    void Backend_GfxTUI::on_os_backend_set() {
        // Do nothing
    }

    WindowSize Backend_GfxTUI::get_window_size() {
        return {g_screen->nx, g_screen->ny};
    }

    bool Backend_GfxTUI::exit_requested() {
        return false;
    }
}; // namespace RC::GUI
