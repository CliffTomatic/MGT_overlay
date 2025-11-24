#include "framework.h"
#include "MGT_overlay.h"
#include "MagnifierOverlay.h"

// Register Overlay Class and create it as a style
ATOM RegisterOverlayClass(HINSTANCE hInstance)
{
    if (!overlayClassRegistered) {
        overlayClassRegistered = true;
        WNDCLASSEXW owcex;

        owcex.cbSize = sizeof(WNDCLASSEX);

        owcex.style = CS_HREDRAW | CS_VREDRAW; // Redraw x and y pixels when resized
        owcex.cbClsExtra = NULL;
        owcex.cbWndExtra = NULL;
        owcex.lpfnWndProc = WndProc; // Event Handling Method
        owcex.hInstance = hInstance; // App handle used for loading resources
        owcex.hIcon = nullptr; // Don't load icon
        owcex.hCursor = LoadCursor(nullptr, IDC_ARROW); // Load standard cursor
        owcex.hbrBackground = nullptr; // Don't draw background, fixes flickering
        owcex.lpszMenuName = nullptr; // Don't add a menu since it's an overlay
        owcex.lpszClassName = MAG_OVERLAY_CLASS_NAME;
        owcex.hIconSm = nullptr;

        return RegisterClassExW(&owcex);
    }
    return TRUE;
}

// Initiate screen magnifier for overlay
BOOL InitMagnifier(HINSTANCE hInstance)
{
    if (!MagInitialize()) {
        // Magnification failed to initialize
        MessageBox(nullptr, L"MagInitialize failed!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // DPI (Dots per inch) get the resolution of monitor before drawing graphics
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Register Class Name first
    if (!RegisterOverlayClass(hInstance)) {
        // Registration Failed
        MessageBox(nullptr, L"Overlay Class Registration Failed", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }



    // Build Transparent Window
    pOverlayHandle = CreateWindowExW(
        // Extended styles
        WS_EX_LAYERED | // per-pixel alpha blending or transparency (SetLayeredWindowAttributes)
        WS_EX_NOACTIVATE | // Prevents window from becoming foreground
        WS_EX_TOPMOST | // Keeps window always on top
        WS_EX_TRANSPARENT, // Makes window transparent to mouse input (click-through)

        MAG_OVERLAY_CLASS_NAME, // Classname
        L"MagnificationOverlay", // Window Title
        // DW Styles
        WS_POPUP | // Floating - no border or title
        WS_VISIBLE, // Visible instantly after creation
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!pOverlayHandle) {
        DWORD errCode = GetLastError();
        wchar_t errMsg[256];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errCode, 0, errMsg, 256, nullptr);
        MessageBoxW(nullptr, errMsg, L"Overlay Window Creation Failed", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (!pOverlayHandle || !IsWindow(pOverlayHandle)) {
        MessageBox(nullptr, L"Failed to create overlay window!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    SetLayeredWindowAttributes(pOverlayHandle, 0, 255, LWA_ALPHA); // Adjust transparency


    // Integrate Magnification Overlay
    cMagOverlayHandle = CreateWindow(
        WC_MAGNIFIER, // Magnifier class (Magnification Template)
        L"MagWnd",
        WS_CHILD | MS_SHOWMAGNIFIEDCURSOR,
        0, 0, width, height,
        pOverlayHandle, // Parent Overlay
        nullptr,
        hInstance,
        nullptr
    );

    if (!cMagOverlayHandle || !IsWindow(cMagOverlayHandle)) {
        MessageBox(nullptr, L"Failed to create magnifier child window!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ShowWindow(cMagOverlayHandle, SW_SHOW); // Ensure it's displayed

    // Set Zoom Scaling
    UpdateMagnificationZoomFactor(cMagOverlayHandle, zoomFactor);
    // Position screen
    UpdateMagnifierSource(cMagOverlayHandle, width, height);

    return TRUE;
}

// Update the Magnification screen's zoom factor
void UpdateMagnificationZoomFactor(HWND hwndMag, float newZoomFactor) {

    // Create the matrix with the zoom level amount
    MAGTRANSFORM matrix{ 0 }; // Zero out the matrix

    // Set scale
    matrix.v[0][0] = newZoomFactor; // Scale X
    matrix.v[1][1] = newZoomFactor; // Scale Y
    matrix.v[2][2] = 2.0f;       // Required

    // Apply Transform
    BOOL magWndTrans = MagSetWindowTransform(hwndMag, &matrix);

    //// Handle failure (usually from not calling MagInitialize or incorrect hwnd)
    //if (!magWndTrans) {
    //    DWORD err = GetLastError();
    //    MessageBox(nullptr, L"MagSetWindowTransform failed!", L"Error", MB_OK);
    //}
}

// When magnification resize, make sure window is centered
void UpdateMagnifierSource(HWND hwndMag, int newWidth, int newHeight) {
    if (!hwndMag || !IsWindow(hwndMag)) return;

    // Calculate the size of the area to sample, accounting for zoom
    int sourceWidth = static_cast<int>(newWidth / zoomFactor);
    int sourceHeight = static_cast<int>(newHeight / zoomFactor);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RECT srcRect;
    srcRect.left = (screenWidth - sourceWidth) / 2;
    srcRect.top = (screenHeight - sourceHeight) / 2;
    srcRect.right = srcRect.left + sourceWidth;
    srcRect.bottom = srcRect.top + sourceHeight;

    MagSetWindowSource(hwndMag, srcRect);
}
