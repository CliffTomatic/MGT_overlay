#pragma once

#include <d3d11.h>
#include <windows.h>

// External variables (declared in .cpp)
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;

// Core functions
void Init_ImGui_DirectX11(HWND hWnd);
bool CreateDeviceD3D(HWND hWnd);
void CreateRenderTarget();
void CleanupRenderTarget();
void CleanupDeviceD3D();

// Saving features
void LoadOverlaySettings();
void SaveOverlaySettings();
