#include <GUI/DX11.hpp>

#include <backends/imgui_impl_dx11.h>
#include <d3d11.h>
#include <imgui.h>

namespace RC::GUI
{
    static ID3D11Device* g_pd3dDevice = NULL;
    static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
    static IDXGISwapChain* g_pSwapChain = NULL;
    static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    auto Backend_DX11::init() -> void
    {
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    }

    auto Backend_DX11::imgui_backend_newframe() -> void
    {
        ImGui_ImplDX11_NewFrame();
    }

    auto Backend_DX11::render(const float clear_color_with_alpha[4]) -> void
    {
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    auto Backend_DX11::shutdown() -> void
    {
        ImGui_ImplDX11_Shutdown();
    }

    auto Backend_DX11::cleanup() -> void
    {
        CleanupDeviceD3D();
    }

    auto Backend_DX11::create_device() -> bool
    {
        return CreateDeviceD3D(static_cast<HWND>(m_os_backend->get_window_handle()));
    }

    auto Backend_DX11::cleanup_device() -> void
    {
        CleanupDeviceD3D();
    }

    auto Backend_DX11::handle_window_resize(int64_t param_1, int64_t param_2) -> void
    {
        if (g_pd3dDevice != NULL && param_1 != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(param_2), (UINT)HIWORD(param_2), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
    }

    auto Backend_DX11::on_os_backend_set() -> void
    {
    }

    // Helper functions

    bool CreateDeviceD3D(HWND hWnd)
    {
        // Setup swap chain
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = {
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_0,
        };
        if (D3D11CreateDeviceAndSwapChain(NULL,
                                          D3D_DRIVER_TYPE_HARDWARE,
                                          NULL,
                                          createDeviceFlags,
                                          featureLevelArray,
                                          2,
                                          D3D11_SDK_VERSION,
                                          &sd,
                                          &g_pSwapChain,
                                          &g_pd3dDevice,
                                          &featureLevel,
                                          &g_pd3dDeviceContext) != S_OK)
        {
            return false;
        }

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D()
    {
        CleanupRenderTarget();
        if (g_pSwapChain)
        {
            g_pSwapChain->Release();
            g_pSwapChain = NULL;
        }
        if (g_pd3dDeviceContext)
        {
            g_pd3dDeviceContext->Release();
            g_pd3dDeviceContext = NULL;
        }
        if (g_pd3dDevice)
        {
            g_pd3dDevice->Release();
            g_pd3dDevice = NULL;
        }
    }

    void CreateRenderTarget()
    {
        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    void CleanupRenderTarget()
    {
        if (g_mainRenderTargetView)
        {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = NULL;
        }
    }
} // namespace RC::GUI
