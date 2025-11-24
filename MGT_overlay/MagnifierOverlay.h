#pragma once

#include <windows.h>
#include "framework.h"
#include "MGT_overlay.h"

// Magnification Forward Declarations
void UpdateMagnifierSource(HWND hwndMag, int width, int height);
void UpdateMagnificationZoomFactor(HWND hwndMag, float newZoomFactor);
ATOM RegisterOverlayClass(HINSTANCE hInstance);
BOOL InitMagnifier(HINSTANCE hInstance);
