#include <GUI/GUI.hpp>

#include <memory>

#include <ExceptionHandling.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <GUI/DX11.hpp>
#include <GUI/GLFW3_OpenGL3.hpp>
#include <GUI/Windows.hpp>
#include <GUI/Dumpers.hpp>
#include <GUI/BPMods.hpp>

#include <Unreal/UnrealInitializer.hpp>
#include <UE4SSProgram.hpp>
#ifdef TEXT
#undef TEXT
#endif

#include <imgui.h>
#include "Roboto.hpp"

namespace RC::GUI
{
    ImColor g_imgui_bg_color{40, 40, 40, 255};
    ImColor g_imgui_bg_hover_color = ImColor{60, 60, 60, 255};
    ImColor g_imgui_bg_active_color = ImColor{50, 50, 50, 255};
    ImColor g_imgui_bg_header_color{50, 50, 50, 255};
    ImColor g_imgui_bg_header_hover_color = ImColor{70, 70, 70, 255};
    ImColor g_imgui_bg_header_active_color = ImColor{60, 60, 60, 255};
    ImColor g_imgui_text_color = ImColor{204, 204, 204, 255};
    ImColor g_imgui_text_inactive_color = ImColor{204, 204, 204, 200};
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

    std::vector<DebuggingGUI::EndOfFrameCallback> DebuggingGUI::s_end_of_frame_callbacks{};

    auto DebuggingGUI::is_valid() -> bool
    {
        return m_os_backend && m_gfx_backend;
    }

    auto DebuggingGUI::on_update() -> void
    {
        static bool show_window = true;
        static bool is_console_open = true;

        if (!is_valid()) { return; }

        if (show_window)
        {
            ImGui::SetNextWindowPos({0, 0});
            auto current_window_size = m_os_backend->is_valid() ? m_os_backend->get_window_size() : m_gfx_backend->get_window_size();
            ImGui::SetNextWindowSize({static_cast<float>(current_window_size.x), static_cast<float>(current_window_size.y)});
            ImGui::Begin("MainWindow", &show_window, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            if (ImGui::BeginTabBar("##MainTabBar", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Console"))
                {
                    get_console().render_search_box();

                    ImGui::SameLine();

                    auto event_thread_busy = m_event_thread_busy || !UE4SSProgram::get_program().can_process_events();

                    if (event_thread_busy) { ImGui::BeginDisabled(true); }
                    if (ImGui::Button("Dump Objects & Properties"))
                    {
                        m_event_thread_busy = true;
                        UE4SSProgram::get_program().queue_event([](void* data) {
                            UE4SSProgram::dump_all_objects_and_properties(UE4SSProgram::get_program().get_object_dumper_output_directory() + STR("\\") + UE4SSProgram::m_object_dumper_file_name);
                            static_cast<GUI::DebuggingGUI*>(data)->m_event_thread_busy = false;
                        }, this);
                    }
                    if (event_thread_busy) { ImGui::EndDisabled(); }

                    if (event_thread_busy) { ImGui::BeginDisabled(true); }
                    ImGui::SameLine();
                    if (ImGui::Button("Restart All Mods"))
                    {
                        m_event_thread_busy = true;
                        UE4SSProgram::get_program().queue_event([](void* data) {
                            UE4SSProgram::get_program().reinstall_mods();
                            static_cast<GUI::DebuggingGUI*>(data)->m_event_thread_busy = false;
                        }, this);
                    }
                    if (event_thread_busy) { ImGui::EndDisabled(); }

                    get_console().render();

                    ImGui::EndTabItem();
                }

                bool listeners_are_required{};
                bool should_unset_listeners{};

                auto is_unreal_initialized = Unreal::UnrealInitializer::StaticStorage::bIsInitialized;
                if (!is_unreal_initialized) { ImGui::BeginDisabled(true); }
                if (ImGui::BeginTabItem("Live View"))
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

                if (ImGui::BeginTabItem("Watches"))
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

                if (ImGui::BeginTabItem("Dumpers"))
                {
                    Dumpers::render();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("BP Mods"))
                {
                    BPMods::render();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
                if (!is_unreal_initialized) { ImGui::EndDisabled(); }
            }

            ImGui::End();

            m_live_view.process_watches();
        }
    }

    static auto gui_setup_style() -> void
    {
        ImGui::StyleColorsDark();
        //ImGui::GetIO().FontGlobalScale = 2.0f;
        // prefer to reload font + rebuild ImFontAtlas + call style.ScaleAllSizes().
        //ImGui::GetStyle().ScaleAllSizes(2.0f);
        //ImGui::GetIO().DisplayFramebufferScale.x = 2;
        //ImGui::GetIO().DisplayFramebufferScale.y = 2;
        //ImGui::GetIO().Fonts->Build();
    }

    auto DebuggingGUI::main_loop_internal() -> void
    {
        if (!is_valid()) { return; }

        m_is_open = true;

        auto& style = ImGui::GetStyle();
        //style.Colors[ImGuiCol_FrameBg] = ImVec4{0.118f, 0.118f, 0.118f, 1.0f};
        //style.Colors[ImGuiCol_Tab] = ImVec4{0.118f, 0.118f, 0.118f, 1.0f};
        //style.Colors[ImGuiCol_TabHovered] = ImVec4{0.118f, 0.118f, 0.118f, 1.0f};
        //style.Colors[ImGuiCol_TabActive] = ImVec4{0.118f, 0.118f, 0.118f, 1.0f};
        //style.Colors[ImGuiCol_Text] = ImVec4{0.800f, 0.800f, 0.800f, 1.0f};
        style.Colors[ImGuiCol_FrameBg] = g_imgui_bg_color;
        style.Colors[ImGuiCol_Tab] = g_imgui_bg_color;
        style.Colors[ImGuiCol_TabHovered] = g_imgui_bg_hover_color;
        style.Colors[ImGuiCol_TabActive] = g_imgui_bg_active_color;
        style.Colors[ImGuiCol_Header] = g_imgui_bg_header_color;
        style.Colors[ImGuiCol_HeaderHovered] = g_imgui_bg_header_hover_color;
        style.Colors[ImGuiCol_HeaderActive] = g_imgui_bg_header_active_color;
        style.Colors[ImGuiCol_Text] = g_imgui_text_color;

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
                    Output::send<LogLevel::Error>(STR("Error: {}\n"), to_wstring(e.what()));
                }
                else
                {
                    printf_s("Internal Error: %s\n", e.what());
                }

                // You're not allowed to throw exceptions directly inside a frame!
                // Use GUI::TRY to move exceptions to the end of the frame.
                abort();
            }

            ImGui::Render();

            const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w,
                    clear_color.y * clear_color.w,
                    clear_color.z * clear_color.w,
                    clear_color.w};
            m_gfx_backend->render(clear_color_with_alpha);


            s_end_of_frame_callbacks.erase(std::remove_if(s_end_of_frame_callbacks.begin(), s_end_of_frame_callbacks.end(), [&](EndOfFrameCallback& callback) -> bool {
                RC::TRY([&] {
                    callback();
                });
                return true;
            }), s_end_of_frame_callbacks.end());
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
        if (!is_valid()) { return; }
        m_thread_stop_token = stop_token;

        m_live_view.initialize();

        m_os_backend->create_window();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        
        gui_setup_style();
        io.Fonts->Clear();
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false; // if true it will try to free memory and fail
        io.Fonts->AddFontFromMemoryTTF(Roboto, sizeof(Roboto), 14 * UE4SSProgram::settings_manager.Debug.DebugGUIFontScaling, &font_cfg);
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
            case GfxBackend::DX11:
                m_gfx_backend = std::make_unique<Backend_DX11>();
                m_os_backend = std::make_unique<Backend_Windows>();
                break;
            case GfxBackend::GLFW3_OpenGL3:
                m_gfx_backend = std::make_unique<Backend_GLFW3_OpenGL3>();
                m_os_backend = std::make_unique<Backend_NoOS>();
                break;
        }

        m_gfx_backend->set_os_backend(m_os_backend.get());
        m_os_backend->set_gfx_backend(m_gfx_backend.get());
        m_gfx_backend->on_os_backend_set();
        m_os_backend->on_gfx_backend_set();
    }

    DebuggingGUI::~DebuggingGUI()
    {
        UE4SSProgram::get_program().stop_render_thread();
    }

    auto gui_thread(std::stop_token stop_token, DebuggingGUI* debugging_ui) -> void
    {
        if (!debugging_ui)
        {
            Output::send<LogLevel::Error>(STR("Could not start GUI render thread because 'debugging_ui' was nullptr."));
            return;
        }
        debugging_ui->setup(std::move(stop_token));
    }
}

