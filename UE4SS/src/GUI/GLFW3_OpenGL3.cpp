#include <GUI/GLFW3_OpenGL3.hpp>

#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

namespace RC::GUI
{
    static void glfw_error_callback(int error, const char* description)
    {
        Output::send<LogLevel::Error>(STR("Glfw Error {}: {}\n"), error, to_wstring(description));
    }

    auto Backend_GLFW3_OpenGL3::init() -> void
    {
        // Setup window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            throw std::runtime_error{"Was unable to initialize glfw"};
        }

        // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only

        // Create window with graphics context
        m_window = glfwCreateWindow(1280, 800, "UE4SS Debugging Tools (OpenGL 3)", NULL, NULL);
        if (m_window == NULL)
        {
            throw std::runtime_error{"Was unable to create glfw window"};
        }
        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1); // Enable vsync

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            throw std::runtime_error{"Was unable to initialize glad"};
        }

        int left, top, right, bottom;
        glfwGetWindowFrameSize(m_window, &left, &top, &right, &bottom);
        glfwSetWindowSize(m_window, 1280 - left - right, 800 - top - bottom);

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    auto Backend_GLFW3_OpenGL3::imgui_backend_newframe() -> void
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
    }

    auto Backend_GLFW3_OpenGL3::render(const float clear_color_with_alpha[4]) -> void
    {
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }

    auto Backend_GLFW3_OpenGL3::shutdown() -> void
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    auto Backend_GLFW3_OpenGL3::cleanup() -> void
    {
    }

    auto Backend_GLFW3_OpenGL3::create_device() -> bool
    {
        return false;
    }

    auto Backend_GLFW3_OpenGL3::cleanup_device() -> void
    {
    }

    auto Backend_GLFW3_OpenGL3::handle_window_resize(int64_t param_1, int64_t param_2) -> void
    {
    }

    auto Backend_GLFW3_OpenGL3::on_os_backend_set() -> void
    {
    }

    auto Backend_GLFW3_OpenGL3::get_window_size() -> WindowSize
    {
        int left, top, right, bottom;
        glfwGetWindowFrameSize(m_window, &left, &top, &right, &bottom);

        int w, h;
        glfwGetWindowSize(m_window, &w, &h);
        return {w + left + right, h + top + bottom};
    }

    auto Backend_GLFW3_OpenGL3::get_window_position() -> WindowPosition
    {
        int left, top, right, bottom;
        glfwGetWindowFrameSize(m_window, &left, &top, &right, &bottom);

        return {left, top};
    }

    auto Backend_GLFW3_OpenGL3::exit_requested() -> bool
    {
        return glfwWindowShouldClose(m_window);
    }
} // namespace RC::GUI
