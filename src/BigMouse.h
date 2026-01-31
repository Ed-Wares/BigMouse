#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <WinUser.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <iostream>

#include "resource.h"

// Constants
#define MAX_LOADSTRING 100
#define MOUSEEVENTF_WHEEL 0x0800
#define WM_MOUSEWHEEL 0x020A
#define OCR_NORMAL 32512
#define OCR_IBEAM 32513

#define WIN_REGION_OFFSET_X 0
#define WIN_REGION_OFFSET_Y 0
#define REGION_BORDER_OFFSET_X 0
#define REGION_BORDER_OFFSET_Y 0
#define REGION_BORDER_MAGNIFY 0
#define REGION_BORDER_THICKNESS 6

// Function declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void SetNewMouseCursor();
HRGN CreateWindowRegion(HWND win, int offsetX, int offsetY, double magnify);
bool OnCopyHookData(COPYDATASTRUCT *pCopyDataStruct, HWND sourceHwnd);
void Shutdown();
void RedirectIOToConsole();


