#include <GUI/GUI.hpp>

#include <memory>

#include <Profiler/Profiler.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#include <GUI/BPMods.hpp>
#include <GUI/Dumpers.hpp>

#ifdef WIN32
#include <GUI/Windows.hpp>
#ifdef HAS_D3D11
#include <GUI/DX11.hpp>
#endif
#endif

#ifdef HAS_GLFW
#include <GUI/GLFW3_OpenGL3.hpp>
#endif

#ifdef HAS_TUI
#include <GUI/TUI.hpp>
#endif

#include <UE4SSProgram.hpp>
#include <Unreal/UnrealInitializer.hpp>
#ifdef TEXT
#undef TEXT
#endif

#ifdef HAS_GUI
#include "Roboto.hpp"
#include "FaSolid900.hpp"
#include <IconsFontAwesome5.h>
#elif defined(HAS_TUI)
#include <GUI/NerdFont.hpp>
#endif

#include <imgui.h>
namespace RC::GUI
{
    ImColor g_imgui_bg_color{0.22f, 0.22f, 0.22f, 1.00f};
    ImColor g_imgui_text_color = ImColor{0.95f, 0.95f, 0.95f, 1.00f};
    ImColor g_imgui_text_inactive_color = ImVec4{0.50f, 0.50f, 0.50f, 1.00f};
    ImColor g_imgui_text_editor_default_bg_color = g_imgui_bg_color;
    ImColor g_imgui_text_editor_default_text_color = g_imgui_text_color;
    ImColor g_imgui_text_editor_normal_bg_color = g_imgui_bg_color;
    ImColor g_imgui_text_editor_normal_text_color = g_imgui_text_color;
    ImColor g_imgui_text_editor_verbose_bg_color = ImColor{0, 55, 125, 255};
    ImColor g_imgui_text_editor_verbose_text_color = ImColor{255, 255, 255, 255};
    ImColor g_imgui_text_editor_warning_bg_color = ImColor{120, 60, 0, 250};
    ImColor g_imgui_text_editor_warning_text_color = ImColor{255, 255, 255, 255};
    ImColor g_imgui_text_editor_error_bg_color = ImColor{148, 36, 20, 255};
    ImColor g_imgui_text_editor_error_text_color = ImColor{255, 255, 255, 255};
    ImColor g_imgui_text_live_view_unreflected_data_color = ImColor{235, 128, 52, 255};
    ImColor g_imgui_text_green_color = ImColor{115, 235, 120, 255};
    ImColor g_imgui_text_blue_color = ImColor{135, 195, 250, 255};
    ImColor g_imgui_text_purple_color = ImColor{170, 145, 255, 255};

    std::vector<DebuggingGUI::EndOfFrameCallback> DebuggingGUI::s_end_of_frame_callbacks{};

    auto DebuggingGUI::is_valid() -> bool
    {
        return m_os_backend && m_gfx_backend;
    }

    auto DebuggingGUI::on_update() -> void
    {
        static bool show_window = true;
        static bool is_console_open = true;

        if (!is_valid())
        {
            return;
        }

        if (show_window)
        {
            ImGui::SetNextWindowPos({0, 0});
            auto current_window_size = m_os_backend->is_valid() ? m_os_backend->get_window_size() : m_gfx_backend->get_window_size();
            ImGui::SetNextWindowSize({static_cast<float>(current_window_size.x), static_cast<float>(current_window_size.y)});
            ImGui::Begin("MainWindow",
                         &show_window,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            if (ImGui::BeginTabBar("##MainTabBar", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem(ATTACH_ICON(ICON_FA_TERMINAL, " Console")))
                {
                    get_console().render_search_box();

                    ImGui::SameLine();

                    auto event_thread_busy = m_event_thread_busy || !UE4SSProgram::get_program().can_process_events();

                    if (event_thread_busy)
                    {
                        ImGui::BeginDisabled(true);
                    }
                    if (ImGui::Button(ATTACH_ICON(ICON_FA_ARCHIVE, " Dump Objects & Properties")))
                    {
                        m_event_thread_busy = true;
                        UE4SSProgram::get_program().queue_event(
                                [](void* data) {
                                    UE4SSProgram::dump_all_objects_and_properties(
                                            to_system(std::filesystem::path{UE4SSProgram::get_program().get_object_dumper_output_directory()} /
                                                      UE4SSProgram::m_object_dumper_file_name));
                                    static_cast<GUI::DebuggingGUI*>(data)->m_event_thread_busy = false;
                                },
                                this);
                    }
                    if (event_thread_busy)
                    {
                        ImGui::EndDisabled();
                    }

                    if (event_thread_busy)
                    {
                        ImGui::BeginDisabled(true);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ATTACH_ICON(ICON_FA_SYNC, " Restart All Mods")))
                    {
                        m_event_thread_busy = true;
                        UE4SSProgram::get_program().queue_event(
                                [](void* data) {
                                    UE4SSProgram::get_program().reinstall_mods();
                                    static_cast<GUI::DebuggingGUI*>(data)->m_event_thread_busy = false;
                                },
                                this);
                    }
                    if (event_thread_busy)
                    {
                        ImGui::EndDisabled();
                    }

                    get_console().render();

                    ImGui::EndTabItem();
                }

                bool listeners_are_required{};
                bool should_unset_listeners{};

                auto is_unreal_initialized = Unreal::UnrealInitializer::StaticStorage::bIsInitialized;
                if (!is_unreal_initialized)
                {
                    ImGui::BeginDisabled(true);
                }
                if (ImGui::BeginTabItem(ATTACH_ICON(ICON_FA_FILE_ALT, " Live View")))
                {
                    listeners_are_required = true;
                    m_live_view.set_listeners();
                    m_live_view.render();
                    ImGui::EndTabItem();
                }
                else
                {
                    should_unset_listeners = true;
                }

                if (ImGui::BeginTabItem(ATTACH_ICON(ICON_FA_EYE, " Watches")))
                {
                    listeners_are_required = true;
                    m_live_view.render_watches();
                    ImGui::EndTabItem();
                }
                else
                {
                    should_unset_listeners = true;
                }

                if (should_unset_listeners && !listeners_are_required)
                {
                    m_live_view.unset_listeners();
                }

                if (ImGui::BeginTabItem(ATTACH_ICON(ICON_FA_ARCHIVE, " Dumpers")))
                {
                    Dumpers::render();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(ATTACH_ICON(ICON_FA_PUZZLE_PIECE, " BP Mods")))
                {
                    BPMods::render();
                    ImGui::EndTabItem();
                }

                {
                    std::lock_guard<std::mutex> guard(m_tabs_mutex);
                    for (const auto& tab : m_tabs)
                    {
                        if (ImGui::BeginTabItem(to_string(tab->tab_name).c_str()))
                        {
                            if (tab->render_function)
                            {
                                tab->render_function(tab->get_owner());
                            }
                            ImGui::EndTabItem();
                        }
                    }
                }

                ImGui::EndTabBar();
                if (!is_unreal_initialized)
                {
                    ImGui::EndDisabled();
                }
            }

            ImGui::End();

            m_live_view.process_watches();
        }
    }

    static auto gui_setup_style() -> void
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding = ImVec2(8, 8);
#ifdef HAS_GUI
        style.FramePadding = ImVec2(12, 5);
#else
        style.FramePadding = ImVec2(0.5f, 0.5f);
#endif
        style.CellPadding = ImVec2(3, 3);
        style.ItemSpacing = ImVec2(8, 4);
        style.ItemInnerSpacing = ImVec2(4, 4);
        style.TouchExtraPadding = ImVec2(0, 0);
        style.IndentSpacing = 21;
        style.ScrollbarSize = 14;
        style.GrabMinSize = 12;

        style.WindowBorderSize = 0;
        style.ChildBorderSize = 1;
        style.PopupBorderSize = 0;
        style.FrameBorderSize = 0;
        style.TabBorderSize = 0;

        style.WindowRounding = 5;
        style.ChildRounding = 0;
        style.FrameRounding = 3;
        style.PopupRounding = 0;
        style.ScrollbarRounding = 9;
        style.GrabRounding = 3;
        style.LogSliderDeadzone = 4;
        style.TabRounding = 4;

        style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.50f, 0.50f);
        style.SelectableTextAlign = ImVec2(0.00f, 0.00f);

        style.DisplaySafeAreaPadding = ImVec2(3, 3);

        style.Colors[ImGuiCol_Text] = g_imgui_text_color;
        style.Colors[ImGuiCol_TextDisabled] = g_imgui_text_inactive_color;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.03f, 0.03f, 0.94f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = g_imgui_bg_color;
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.52f, 0.20f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.66f, 0.21f, 0.58f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.22f, 0.45f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.63f, 0.24f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.51f, 0.23f, 0.42f, 1.00f);
#ifdef HAS_GUI
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.59f, 0.22f, 0.48f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.63f, 0.24f, 0.50f, 1.00f);
#else
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.5f, 0.8f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.4f, 0.50f, 1.00f);
#endif
        style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.92f, 0.24f, 0.84f, 0.20f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.24f, 0.84f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.24f, 0.84f, 0.95f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.51f, 0.23f, 0.42f, 1.00f);
#ifdef HAS_GUI
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.59f, 0.22f, 0.48f, 1.00f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.63f, 0.24f, 0.50f, 1.00f);
#else
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.4f, 0.5f, 0.8f, 1.00f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.65f, 0.4f, 0.50f, 1.00f);
#endif
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.65f, 0.24f, 0.57f, 0.38f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    auto DebuggingGUI::main_loop_internal() -> void
    {
        if (!is_valid())
        {
            return;
        }

        m_is_open = true;

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        while (!m_exit_requested && !m_gfx_backend->exit_requested())
        {
            m_os_backend->exec_message_loop(&m_exit_requested);

            if (m_exit_requested)
            {
                break;
            }
            if (m_thread_stop_token.stop_requested())
            {
                break;
            }

            m_gfx_backend->imgui_backend_newframe();
            m_os_backend->imgui_backend_newframe();
            ImGui::NewFrame();

            try
            {
                on_update();
            }
            catch (std::exception& e)
            {
                if (!Output::has_internal_error())
                {
                    Output::send<LogLevel::Error>(SYSSTR("Error: {}\n"), to_system(e.what()));
                }
                else
                {
                    fprintf(stderr, "Internal Error: %s\n", e.what());
                    fflush(stderr);
                }

// You're not allowed to throw exceptions directly inside a frame!
// Use GUI::TRY to move exceptions to the end of the frame.
#ifdef WIN32
                __debugbreak();
#else
                asm("int3");
#endif
            }

            ImGui::Render();

            const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w};
            m_gfx_backend->render(clear_color_with_alpha);

            s_end_of_frame_callbacks.erase(std::remove_if(s_end_of_frame_callbacks.begin(),
                                                          s_end_of_frame_callbacks.end(),
                                                          [&](EndOfFrameCallback& callback) -> bool {
                                                              RC::TRY([&] {
                                                                  callback();
                                                              });
                                                              return true;
                                                          }),
                                           s_end_of_frame_callbacks.end());
        }

        m_is_open = false;
        m_exit_requested = false;
    }

    auto DebuggingGUI::execute_at_end_of_frame(EndOfFrameCallback callback) -> void
    {
        s_end_of_frame_callbacks.emplace_back(callback);
    }

    auto DebuggingGUI::setup(std::stop_token&& stop_token) -> void
    {
        if (!is_valid())
        {
            return;
        }
        m_thread_stop_token = stop_token;

        m_live_view.initialize();

        m_os_backend->create_window();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        gui_setup_style();
        io.Fonts->Clear();

        float base_font_size = 14 * UE4SSProgram::settings_manager.Debug.DebugGUIFontScaling;
#ifdef HAS_GUI
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false; // if true it will try to free memory and fail
        io.Fonts->AddFontFromMemoryTTF(Roboto, sizeof(Roboto), base_font_size, &font_cfg);

        float icon_font_size = base_font_size * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced;
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_cfg;
        icons_cfg.FontDataOwnedByAtlas = false; // if true it will try to free memory and fail
        icons_cfg.MergeMode = true;
        icons_cfg.PixelSnapH = true;
        icons_cfg.GlyphMinAdvanceX = icon_font_size;
        io.Fonts->AddFontFromMemoryTTF(FaSolid900, sizeof(FaSolid900), icon_font_size, &icons_cfg, icons_ranges);
#endif
        m_os_backend->init();
        m_gfx_backend->init();

        main_loop_internal();

        m_gfx_backend->shutdown();
        m_os_backend->shutdown();
        ImGui::DestroyContext();

        m_gfx_backend->cleanup();
        m_os_backend->cleanup();
    }

    auto DebuggingGUI::set_gfx_backend(GfxBackend backend) -> void
    {
        switch (backend)
        {
#ifdef HAS_D3D11
        case GfxBackend::DX11:
            m_gfx_backend = std::make_unique<Backend_DX11>();
            m_os_backend = std::make_unique<Backend_Windows>();
            break;
        case GfxBackend::GLFW3_OpenGL3:
            m_gfx_backend = std::make_unique<Backend_GLFW3_OpenGL3>();
            m_os_backend = std::make_unique<Backend_NoOS>();
            break;
#else
#ifdef HAS_GLFW
        case GfxBackend::GLFW3_OpenGL3:
            m_gfx_backend = std::make_unique<Backend_GLFW3_OpenGL3>();
            m_os_backend = std::make_unique<Backend_NoOS>();
            break;
#elif defined(HAS_TUI)
        case GfxBackend::TUI:
            m_gfx_backend = std::make_unique<Backend_GfxTUI>();
            m_os_backend = std::make_unique<Backend_TUI>();
#endif
#endif
        }

        m_gfx_backend->set_os_backend(m_os_backend.get());
        m_os_backend->set_gfx_backend(m_gfx_backend.get());
        m_gfx_backend->on_os_backend_set();
        m_os_backend->on_gfx_backend_set();
    }

    auto DebuggingGUI::add_tab(std::shared_ptr<GUITab> tab) -> void
    {
        std::lock_guard<std::mutex> guard(m_tabs_mutex);
        m_tabs.push_back(tab);
    }
    auto DebuggingGUI::remove_tab(std::shared_ptr<GUITab> tab) -> void
    {
        std::lock_guard<std::mutex> guard(m_tabs_mutex);
        m_tabs.erase(std::remove(m_tabs.begin(), m_tabs.end(), tab), m_tabs.end());
    }

    DebuggingGUI::~DebuggingGUI()
    {
        UE4SSProgram::get_program().stop_render_thread();
    }

    auto gui_thread(std::stop_token stop_token, DebuggingGUI* debugging_ui) -> void
    {
        ProfilerSetThreadName("UE4SS-GuiThread");

        if (!debugging_ui)
        {
            Output::send<LogLevel::Error>(SYSSTR("Could not start GUI render thread because 'debugging_ui' was nullptr."));
            return;
        }
        debugging_ui->setup(std::move(stop_token));
    }
} // namespace RC::GUI
