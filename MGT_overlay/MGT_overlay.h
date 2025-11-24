#pragma once

// Basic Windows + Standard Headers
#define NOMINMAX
#undef min
#undef max

#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <string>

// ImGui and resource headers
#include "imgui_overlay.h"
#include "resource.h"

// Constants
#define MAX_LOADSTRING 100
extern const int HOTKEY_ID;
extern const UINT g_timerIntervalMs;

// Global Handles
extern HINSTANCE hInst;
extern HWND mainMenuHandle;
extern HWND pOverlayHandle;
extern HWND cMagOverlayHandle;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];
extern LPCTSTR MAG_OVERLAY_CLASS_NAME;

// Overlay State
extern int width;
extern int height;
extern float zoomFactor;
extern HWND hSliderZoom;
extern HWND hTextboxZoom;

extern bool overlayStarted;
extern bool overlayVisible;
extern bool showOverlayControls;
extern bool shouldShutdownOverlay;
extern bool overlayClassRegistered;

// Hotkey
extern int userHotkey;
extern bool awaitingHotkeyInput;

// Timer
extern UINT g_timerId;

// Function Declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

// Timer
void StartMagnifierTimer();
void StopMagnifierTimer();
void CALLBACK TimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

// Overlay Config
void SaveOverlaySettings();
void LoadOverlaySettings();

// ImGui
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
