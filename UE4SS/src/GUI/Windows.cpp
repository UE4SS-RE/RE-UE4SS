#include <GUI/Windows.hpp>

#include <GUI/DX11.hpp>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <tchar.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace RC::GUI
{
    static HWND s_hwnd{};
    static WNDCLASSEX s_wc{};
    static GfxBackendBase* s_gfx_backend{};

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    auto Backend_Windows::init() -> void
    {
        ImGui_ImplWin32_Init(s_hwnd);
    }

    auto Backend_Windows::imgui_backend_newframe() -> void
    {
        ImGui_ImplWin32_NewFrame();
    }

    auto Backend_Windows::create_window(int loc_x, int loc_y, int size_x, int size_y) -> void
    {
        StringType title_bar_text{STR("UE4SS Debugging Tools")};
        if (dynamic_cast<Backend_DX11*>(m_gfx_backend))
        {
            title_bar_text.append(STR(" (DX11)"));
        }

        // ImGui_ImplWin32_EnableDpiAwareness()
        s_wc.cbSize = sizeof(WNDCLASSEX);
        // wc.style = CS_CLASSDC;
        s_wc.style = CS_HREDRAW | CS_VREDRAW;
        s_wc.lpfnWndProc = WndProc;
        s_wc.cbClsExtra = 0L;
        s_wc.cbWndExtra = 0L;
        s_wc.hInstance = GetModuleHandle(NULL);
        s_wc.hIcon = NULL;
        s_wc.hCursor = NULL;
        // wc.hbrBackground = NULL;
        s_wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        s_wc.lpszMenuName = NULL;
        s_wc.lpszClassName = title_bar_text.c_str();
        s_wc.hIconSm = NULL;
        ::RegisterClassEx(&s_wc);
        s_hwnd = ::CreateWindow(s_wc.lpszClassName, title_bar_text.c_str(), WS_OVERLAPPEDWINDOW, loc_x, loc_y, size_x, size_y, NULL, NULL, s_wc.hInstance, NULL);

        if (!m_gfx_backend->create_device())
        {
            m_gfx_backend->cleanup_device();
            ::UnregisterClass(s_wc.lpszClassName, s_wc.hInstance);
            throw std::runtime_error{"Was unable to create d3d device"};
        }

        ::ShowWindow(s_hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(s_hwnd);
    }

    auto Backend_Windows::exec_message_loop(bool* exit_requested) -> void
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                *exit_requested = true;
            }
        }
    }

    auto Backend_Windows::shutdown() -> void
    {
        ImGui_ImplWin32_Shutdown();
    }

    auto Backend_Windows::cleanup() -> void
    {
        ::DestroyWindow(s_hwnd);
        ::UnregisterClass(s_wc.lpszClassName, s_wc.hInstance);
    }

    auto Backend_Windows::get_window_handle() -> void*
    {
        return s_hwnd;
    }

    auto Backend_Windows::get_window_size() -> WindowSize
    {
        RECT current_window_rect{};
        GetWindowRect(s_hwnd, &current_window_rect);
        return {static_cast<int32_t>(current_window_rect.right - current_window_rect.left), static_cast<int32_t>(current_window_rect.bottom - current_window_rect.top)};
    }

    auto Backend_Windows::get_window_position() -> WindowPosition
    {
        RECT current_window_rect{};
        GetWindowRect(s_hwnd, &current_window_rect);
        return {static_cast<int32_t>(current_window_rect.left), static_cast<int32_t>(current_window_rect.top)};
    }

    auto Backend_Windows::on_gfx_backend_set() -> void
    {
        s_gfx_backend = m_gfx_backend;
    }

    // Win32 message handler
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        {
            return true;
        }

        switch (msg)
        {
        case WM_SIZE:
            s_gfx_backend->handle_window_resize(wParam, lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
            { // Disable ALT application menu
                return 0;
            }
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }
} // namespace RC::GUI
