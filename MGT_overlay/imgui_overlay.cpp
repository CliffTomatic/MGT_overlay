// Core personal libraraies
#include "framework.h"
#include "MGT_overlay.h"
#include "magnifierOverlay.h"
#include <fstream>
#include <string>
#include <sstream>

// Core ImGui
#include "imgui.h"

// ImGui platform + renderer bindings
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

// Windows & DirectX headers
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <fstream>

// Global vars (used by ImGui's DX11 backend)
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;


// Initialize DirectX11 and the Dear ImGui
void Init_ImGui_DirectX11(HWND hMenuInstance) {
    CreateDeviceD3D(hMenuInstance); // <- creates device + context + swapchain
    CreateRenderTarget();         // <- gets backbuffer from swapchain

    // Init ImGui Overlay
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(hMenuInstance);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

}

bool CreateDeviceD3D(HWND hMenuInstance) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hMenuInstance;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, 1, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext);

    return SUCCEEDED(result);
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))))
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    // Fix: explicitly set viewport to match swapchain buffer
    DXGI_SWAP_CHAIN_DESC sd = {};
    g_pSwapChain->GetDesc(&sd);

    D3D11_VIEWPORT vp = {};
    vp.Width = (FLOAT)sd.BufferDesc.Width;
    vp.Height = (FLOAT)sd.BufferDesc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;

    g_pd3dDeviceContext->RSSetViewports(1, &vp);
}


void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void LoadOverlaySettings()
{
    width = 400;   // hard reset to defaults
    height = 400;
    zoomFactor = 2.0f;

    std::ifstream file("config.txt");
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;            // holds the “label=” part
        float  fval;
        int    ival;

        if (line.rfind("zoom=", 0) == 0 && (iss >> key >> fval))
        {
            if (fval >= 1.f && fval <= 5.f)         zoomFactor = fval;
        }
        else if (line.rfind("width=", 0) == 0 && (iss >> key >> ival))
        {
            if (ival > 0 && ival <= 3000)           width = ival;
        }
        else if (line.rfind("height=", 0) == 0 && (iss >> key >> ival))
        {
            if (ival > 0 && ival <= 3000)           height = ival;
        }
        else if (line.rfind("hotkey=", 0) == 0 && (iss >> key >> std::hex >> ival))
        {
            if (ival >= 0x01 && ival <= 0xFE)       userHotkey = ival;
        }
    }
}

void SaveOverlaySettings()
{
    std::ofstream file("config.txt");
    if (!file.is_open()) return;

    file << "zoom=   " << zoomFactor << '\n';
    file << "width=  " << width << '\n';
    file << "height= " << height << '\n';

    file << std::hex << std::uppercase;            // write VK code in hex
    file << "hotkey= 0x" << userHotkey << '\n';
}