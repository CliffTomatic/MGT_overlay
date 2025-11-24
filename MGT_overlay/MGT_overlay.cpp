// MGT_overlay.cpp : Defines the entry point for the application.

#include "framework.h"
#include <commctrl.h>
#include "MGT_overlay.h"
#include "MagnifierOverlay.h"
#include <algorithm>

// ImGui includes
#include "imgui_overlay.h"

// Refresh Rate
#include <mmsystem.h>

// ImGui platform + renderer bindings
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


// Windows & DirectX headers
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // main instance
HWND mainMenuHandle; // Specific parent window
HWND pOverlayHandle;  // Specific parent Overlay window
HWND cMagOverlayHandle; // Specific child Magnification window
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
LPCTSTR MAG_OVERLAY_CLASS_NAME{ L"MagOverlayWnd" };
bool overlayStarted{ false };
bool showOverlayControls = true;
bool shouldShutdownOverlay = false;
bool overlayClassRegistered = false;
// Refresh Rates
UINT g_timerId = 0;
constexpr UINT g_timerIntervalMs = 4; // ~240Hz 

// User Preference Global Variables
HWND hSliderZoom;
HWND hTextboxZoom;

// Show/Hide tracker
bool overlayVisible = true;  // Controls visibility without destroying overlay
// hot‑key (user‑editable)
int   userHotkey = 0x5A;   // default = Z
bool  awaitingHotkeyInput = false;  // waiting for user to press new key
const int HOTKEY_ID = 1;

int width{ 400 }; // 400px DEFAULT
int height{ 400 }; // 400px DEFAULT
float zoomFactor{ 2 }; // 2x zoom DEFAULT



// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void StopMagnifierTimer();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MGTOVERLAY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    // after InitInstance(...)
    if (!RegisterHotKey(mainMenuHandle, HOTKEY_ID, 0, userHotkey)) {
        wchar_t buf[128];
        swprintf_s(buf, L"RegisterHotKey failed - error %lu", GetLastError());
        MessageBoxW(nullptr, buf, L"Hot-key error", MB_OK | MB_ICONERROR);
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MGTOVERLAY));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // Show/Hide Toggle
        if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID)
        {
            // handle here (avoids relying on WndProc if VK_NULL owner)
            if (overlayStarted)
            {
                overlayVisible = !overlayVisible;
                if (pOverlayHandle)    ShowWindow(pOverlayHandle, overlayVisible ? SW_SHOW : SW_HIDE);
                if (cMagOverlayHandle) ShowWindow(cMagOverlayHandle, overlayVisible ? SW_SHOW : SW_HIDE);
            }
            continue; // skip Translate/Dispatch
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!IsIconic(mainMenuHandle))
        {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2(420, 210), ImGuiCond_FirstUseEver);

            // Keep the menu open as long as the overlay is running
            if (overlayStarted) {
                showOverlayControls = true;
            }

            if (showOverlayControls && ImGui::Begin("Magnification Overlay Menu", &showOverlayControls))
            {
                const int minSize = 100;
                const int maxSize = 3000;
                const float minZoom = 1.0f;
                const float maxZoom = 5.0f;
                // Hot-key
                ImGui::SeparatorText("Toggle Hot-key");
                if (awaitingHotkeyInput) {
                    ImGui::Text("Press any key ...");
                    for (int vk = 0x08; vk <= 0xFE; ++vk) {
                        if (GetAsyncKeyState(vk) & 0x8000) {
                            UnregisterHotKey(mainMenuHandle, HOTKEY_ID);
                            userHotkey = vk;
                            RegisterHotKey(mainMenuHandle, HOTKEY_ID, 0, userHotkey);
                            awaitingHotkeyInput = false;
                            break;
                        }
                    }
                }
                else {
                    ImGui::Text("Current key: %c", (userHotkey >= 0x20 && userHotkey <= 0x7E) ? userHotkey : '?');
                    if (ImGui::Button("Change Hotkey")) awaitingHotkeyInput = true;
                }

                // Zoom
                ImGui::Text("Zoom:");
                ImGui::PushID("zoom");
                if (ImGui::SliderFloat("##zoomSlider", &zoomFactor, minZoom, maxZoom, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
                    SaveOverlaySettings();
                }
                ImGui::PopID();

                // Width
                ImGui::Text("Width:");
                ImGui::PushID("width");
                ImGui::SetNextItemWidth(80);
                if (ImGui::InputInt("##widthInput", &width)) {
                    width = std::clamp(width, minSize, maxSize);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                if (ImGui::Button("-##width")) {
                    width = std::max(minSize, width - 10);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                if (ImGui::Button("+##width")) {
                    width = std::min(maxSize, width + 10);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                ImGui::SliderInt("##widthSlider", &width, minSize, maxSize, "%d px", ImGuiSliderFlags_AlwaysClamp);
                ImGui::PopID();

                // Height
                ImGui::Text("Height:");
                ImGui::PushID("height");
                ImGui::SetNextItemWidth(80);
                if (ImGui::InputInt("##heightInput", &height)) {
                    height = std::clamp(height, minSize, maxSize);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                if (ImGui::Button("-##height")) {
                    height = std::max(minSize, height - 10);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                if (ImGui::Button("+##height")) {
                    height = std::min(maxSize, height + 10);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                ImGui::SliderInt("##heightSlider", &height, minSize, maxSize, "%d px", ImGuiSliderFlags_AlwaysClamp);
                ImGui::PopID();

                // Action Buttons
                if (ImGui::Button("Uniform Scale")) {
                    const float scaleFactor = 1.1f;
                    width = std::clamp(static_cast<int>(width * scaleFactor), minSize, maxSize);
                    height = std::clamp(static_cast<int>(height * scaleFactor), minSize, maxSize);
                    SaveOverlaySettings();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset")) {
                    width = 400;
                    height = 400;
                    zoomFactor = 2.0f;
                    SaveOverlaySettings();
                }

                // Start/Stop Overlay
                if (ImGui::Button(overlayStarted ? "Stop Overlay" : "Start Overlay")) {
                    SaveOverlaySettings();
                    if (!overlayStarted) {
                        if (InitMagnifier(hInstance)) {
                            RECT rect;
                            GetWindowRect(mainMenuHandle, &rect);
                            SetWindowPos(mainMenuHandle, nullptr,
                                rect.left, rect.top,
                                rect.right - rect.left + 1, rect.bottom - rect.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);

                            SetWindowPos(mainMenuHandle, nullptr,
                                rect.left, rect.top,
                                rect.right - rect.left, rect.bottom - rect.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);

                            StartMagnifierTimer();
                            overlayStarted = true;
                        }
                        else {
                            MessageBox(nullptr, L"Failed to initialize magnification overlay.", L"Error", MB_OK | MB_ICONERROR);
                        }
                    }
                    else {
                        // Defer overlay destruction until after rendering
                        shouldShutdownOverlay = true;
                        overlayStarted = false;
                    }
                }
            }
            ImGui::End();

            ImGui::Render();
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_pSwapChain->Present(1, 0);

            // Deferred cleanup to avoid ImGui race conditions
            if (shouldShutdownOverlay)
            {
                shouldShutdownOverlay = false;
                StopMagnifierTimer();

                if (cMagOverlayHandle && IsWindow(cMagOverlayHandle)) {
                    DestroyWindow(cMagOverlayHandle);
                    cMagOverlayHandle = nullptr;
                }

                if (pOverlayHandle && IsWindow(pOverlayHandle)) {
                    DestroyWindow(pOverlayHandle);
                    pOverlayHandle = nullptr;
                }
            }
        }

        // Overlay window positioning & magnification updates
        if (overlayStarted && overlayVisible && pOverlayHandle && cMagOverlayHandle) {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int overlayX = (screenWidth - width) / 2;
            int overlayY = (screenHeight - height) / 2;

            SetWindowPos(pOverlayHandle, HWND_TOPMOST, overlayX, overlayY, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
            SetWindowPos(cMagOverlayHandle, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
            UpdateMagnifierSource(cMagOverlayHandle, width, height);
            UpdateMagnificationZoomFactor(cMagOverlayHandle, zoomFactor);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MGTOVERLAY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0/*MAKEINTRESOURCEW(IDC_MGTOVERLAY) */ ;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // Imgui parameters
    LoadOverlaySettings();

   hInst = hInstance; // Store instance handle in our global variable

   mainMenuHandle = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!mainMenuHandle)
   {
      return FALSE;
   }

   ShowWindow(mainMenuHandle, nCmdShow);
   UpdateWindow(mainMenuHandle);

   // Initialize imgui and Dx11
   Init_ImGui_DirectX11(mainMenuHandle);


   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//  
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true; // Let ImGui handle input if it wants to

    switch (message)
    {
    case WM_HOTKEY:
        if (wParam == 1 && overlayStarted)        // only toggle if overlay is running
        {
            overlayVisible = !overlayVisible;

            if (pOverlayHandle)       ShowWindow(pOverlayHandle, overlayVisible ? SW_SHOW : SW_HIDE);
            if (cMagOverlayHandle)    ShowWindow(cMagOverlayHandle, overlayVisible ? SW_SHOW : SW_HIDE);
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                SaveOverlaySettings();
                if (overlayStarted) {
                    StopMagnifierTimer();
                }

                if (cMagOverlayHandle && IsWindow(cMagOverlayHandle)) {
                    DestroyWindow(cMagOverlayHandle);
                    cMagOverlayHandle = nullptr;
                }

                if (pOverlayHandle && IsWindow(pOverlayHandle)) {
                    DestroyWindow(pOverlayHandle);
                    pOverlayHandle = nullptr;
                }

                //DestroyWindow(hWnd); // Quit app
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        break;
    case WM_DESTROY:
        SaveOverlaySettings();
        if (hWnd == mainMenuHandle)  // Only quit if the main window is being closed
        {
            if (overlayStarted)
                StopMagnifierTimer();

            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            CleanupDeviceD3D();

            PostQuitMessage(0);
        }
        else if (hWnd == pOverlayHandle || hWnd == cMagOverlayHandle)
        {
            if (hWnd == pOverlayHandle) {
                pOverlayHandle = nullptr;
            }
            else if (hWnd == cMagOverlayHandle) {
                cMagOverlayHandle = nullptr;
            }

            // Let the overlay close silently without killing the app
        }
        return 0;


    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ---- TIMERS FOR MAGNIFICATION API
void StartMagnifierTimer() {
    if (g_timerId == 0) {
        timeBeginPeriod(1); // Request 1ms timer precision
        g_timerId = timeSetEvent(g_timerIntervalMs, 0, TimerCallback, 0, TIME_PERIODIC);
    }
}

void StopMagnifierTimer() {
    if (g_timerId != 0) {
        timeKillEvent(g_timerId);
        g_timerId = 0;
        timeEndPeriod(1);
    }
}

// Timer to redraw Magnifier Overlay
void CALLBACK TimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
    if (!overlayVisible || !cMagOverlayHandle || !IsWindow(cMagOverlayHandle)) return;

    UpdateMagnificationZoomFactor(cMagOverlayHandle, zoomFactor);
    UpdateMagnifierSource(cMagOverlayHandle, width, height);
    RedrawWindow(cMagOverlayHandle, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}




