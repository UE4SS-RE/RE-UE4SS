#include <GUI/TUI.hpp>

#include <Input/Handler.hpp>
#include <Input/Platform/NcursesInputSource.hpp>

#include <imtui/imtui.h>

#include <imtui/imtui-impl-text.h>
#include <imtui/imtui-impl-ncurses.h>

#include <UE4SSProgram.hpp>

#include <cstdio>
#include <cstdlib>

namespace RC::GUI
{
    static ImTui::TScreen* g_screen = nullptr;
    static bool tui_shutdown = false;
    static std::shared_ptr<Input::NcursesInputSource> ncurses_source{};
    int saved_stderr = -1;
    void Backend_TUI::init()
    {
        // fprintf(stderr, "Backend_TUI::init\n");
        setenv("TERMINFO", UE4SSProgram::settings_manager.TUI.TERMINFO.c_str(), 0);
        if (!UE4SSProgram::settings_manager.TUI.LCALL.empty())
        {
            setenv("LC_ALL", UE4SSProgram::settings_manager.TUI.LCALL.c_str(), 0);
        }
        g_screen = ImTui_ImplNcurses_Init(true);
        auto source = Input::Handler::get_input_source("Ncurses");
        ncurses_source = std::dynamic_pointer_cast<Input::NcursesInputSource>(source);

        // disable stderr
        saved_stderr = dup(STDERR_FILENO);
        freopen("./imtui.log", "w", stderr);
        setbuf(stderr, NULL);
        fprintf(stderr, "Backend_TUI::init\n");
        fflush(stderr);
    }

    void Backend_TUI::imgui_backend_newframe()
    {
        // fprintf(stderr, "Backend_TUI::imgui_backend_newframe\n");
        if (ncurses_source)
        {
            ncurses_source->begin_frame();
        }
        ImTui_ImplNcurses_NewFrame([](int x) {
            if (ncurses_source)
            {
                ncurses_source->receive_input(x);
            }
        });
        if (ncurses_source)
        {
            ncurses_source->end_frame();
        }
    }

    void Backend_TUI::create_window()
    {
        // Do nothing
    }

    void Backend_TUI::exec_message_loop(bool* exit_requested)
    {
        ImTui_ImplNcurses_ProcessEvent();
        *exit_requested = false;
    }

    void Backend_TUI::shutdown()
    {
        ImTui_ImplNcurses_Shutdown();
        g_screen = nullptr;
        tui_shutdown = true;
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
        saved_stderr = -1;
        fprintf(stderr, "Backend_TUI::shutdown, stderr restored\n");
    }

    void Backend_TUI::cleanup()
    {
        // Do nothing
    }

    void* Backend_TUI::get_window_handle()
    {
        return g_screen;
    }

    WindowSize Backend_TUI::get_window_size()
    {
        if (g_screen)
        {
            return {g_screen->nx, g_screen->ny};
        }
        else
        {
            return {0, 0};
        }
    }

    void Backend_TUI::on_gfx_backend_set()
    {
        // Do nothing
    }

    void Backend_GfxTUI::init()
    {
        ImTui_ImplText_Init();
    }

    void Backend_GfxTUI::imgui_backend_newframe()
    {
        ImTui_ImplText_NewFrame();
    }

    void Backend_GfxTUI::render(const float clear_color_with_alpha[4])
    {
        if (tui_shutdown)
        {
            return;
        }
        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
        ImTui_ImplNcurses_DrawScreen();
    }

    void Backend_GfxTUI::shutdown()
    {
        ImTui_ImplText_Shutdown();
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    void Backend_GfxTUI::cleanup()
    {
        // Do nothing
    }

    bool Backend_GfxTUI::create_device()
    {
        return true;
    }

    void Backend_GfxTUI::cleanup_device()
    {
        // Do nothing
    }

    void Backend_GfxTUI::handle_window_resize(int64_t param_1, int64_t param_2)
    {
        // Do nothing
    }

    void Backend_GfxTUI::on_os_backend_set()
    {
        // Do nothing
    }

    WindowSize Backend_GfxTUI::get_window_size()
    {
        return {g_screen->nx, g_screen->ny};
    }

    bool Backend_GfxTUI::exit_requested()
    {
        return tui_shutdown;
    }
}; // namespace RC::GUI
