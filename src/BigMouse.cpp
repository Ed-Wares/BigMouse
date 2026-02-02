// BigMouse.cpp : Defines the entry point for the application.
// This application uses a DLL to set low-level keyboard and mouse hooks and displays a magnified mouse cursor.

#include "BigMouse.h"
#include "knmhooker.h"
#include "ResExtract.h"

// Global Variables:
HINSTANCE hInst;					 // current instance
TCHAR szTitle[MAX_LOADSTRING];		 // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

HWND mainWnd;
HRGN oldRegion;
HRGN winRegion;
HRGN borderRegion;

HCURSOR oldCursor;
HCURSOR newCursor;

double magnifyRegion = 3;

HBRUSH whiteBrsh;
HBRUSH blackBrsh;


// Function to redirect standard I/O to a new console window for debugging.
void RedirectIOToConsole()
{
	// Create a new console window
	if (!AllocConsole())
	{
		// Handle error if console could not be created
		// MessageBox(NULL, "Failed to create the console.", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	// Re-open standard I/O streams to the new console
	// This is the tricky part that makes std::cout and std::cin work.
	FILE *fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	// Clear the iostream state
	std::cout.clear();
	std::cin.clear();
	std::cerr.clear();
}

// This function enables deep DPI awareness for the application.
void EnableDeepDPIAwareness() {
    // Try to load the modern Windows 10/11 function dynamically
    typedef BOOL(WINAPI* SetDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = LoadLibrary(TEXT("user32.dll"));
    if (hUser32) {
        SetDpiAwarenessContextProc setDpi = (SetDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpi) {
            setDpi((DPI_AWARENESS_CONTEXT)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4
            FreeLibrary(hUser32);
            return;
        }
        FreeLibrary(hUser32);
    }
    SetProcessDPIAware(); // Fallback for Windows 7/8
}

// The main entry point for the Windows application.
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    EnableDeepDPIAwareness(); // This stops the "Growing" window effect on the multiple monitors.
	MSG msg;
	HACCEL hAccelTable;

	if (lpCmdLine && *lpCmdLine)
	{
		// Handle command line arguments if needed
		if (strcmp((char *)lpCmdLine, "/debug") == 0 || strcmp((char *)lpCmdLine, "-debug") == 0)
		{
			RedirectIOToConsole(); // Open a console window for debugging output
		}
	}

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BIGMOUSE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Extract the DLL resource if it doesn't already exist
	// ExtractResource(IDR_KNMHOOKER, KNMHOOKER_FILE);
	ExtractResourceRc("KNMHOOKER_FILE", "DLL", KNMHOOKER_DLL);
	BOOL dllStatus = LoadDllFunctions(LoadLibraryA(KNMHOOKER_DLL));
	if (!dllStatus)
	{
		MessageBoxA(NULL, "Failed to load hook functions from DLL.", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_BIGMOUSE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	FreeConsole(); // Clean up the console when the app exits

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_BIGMOUSE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = (LPCTSTR)IDC_BIGMOUSE;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

// Set a new custom cursor (a blank cursor in this case)
void SetNewMouseCursor()
{
	// oldCursor = GetCursor(); //save current cursor, which might be IDC_WAIT
	oldCursor = LoadCursor(NULL, IDC_ARROW); // Loads the standard arrow cursor
	oldCursor = CopyCursor(oldCursor);
	unsigned char andbits[128];
	unsigned char xorbits[128];
	for (int i = 0; i < 128; i++)
	{
		andbits[i] = 255;
		xorbits[i] = 0;
	}
	newCursor = CreateCursor((HINSTANCE)GetModuleHandle(NULL), 0, 0, 32, 32, andbits, xorbits);
	// HCURSOR hCursor = LoadCursor(NULL, IDC_WAIT);
	SetSystemCursor(newCursor, OCR_NORMAL);
}

// Create a custom-shaped region for the window
HRGN CreateWindowRegion(HWND win, int offsetX, int offsetY, double magnify)
{
	HRGN wrg;
	POINT pol[7]; // polygon points
	// Create a polygon region that looks like a magnified mouse cursor
	HRGN rg;
	pol[0].x = offsetX;
	pol[0].y = (offsetY);
	pol[1].x = offsetX + (100 * magnify);
	pol[1].y = offsetY + (180 * magnify);
	pol[2].x = offsetX + (55 * magnify);
	pol[2].y = offsetY + (197 * magnify);
	pol[3].x = offsetX + (80 * magnify);
	pol[3].y = offsetY + (250 * magnify);
	pol[4].x = offsetX + (70 * magnify);
	pol[4].y = offsetY + (250 * magnify);
	pol[5].x = offsetX + (45 * magnify);
	pol[5].y = offsetY + (200 * magnify);
	pol[6].x = offsetX;
	pol[6].y = offsetY + (220 * magnify);
	rg = CreatePolygonRgn(pol, 7, ALTERNATE);
	return rg;
}

// Process the data received from the hook DLL
bool OnCopyHookData(COPYDATASTRUCT *pCopyDataStruct) // WM_COPYDATA lParam will have this struct
{
	if (pCopyDataStruct->cbData != sizeof(HEVENT))
		return false;
	HEVENT Event;
	memcpy(&Event, (HEVENT *)pCopyDataStruct->lpData, sizeof(HEVENT)); // transfer data to internal variable
	if (Event.dwHookType == WH_KEYBOARD)							   // keyboard event
	{
		KBDLLHOOKSTRUCT *pkh = (KBDLLHOOKSTRUCT *)Event.lParam;
		if (Event.wParam == WM_KEYDOWN || Event.wParam == WM_SYSKEYDOWN)
		{
			std::cout << "Key Down: " << pkh->vkCode << std::endl;
			if (pkh->vkCode == VkKeyScan('7') && GetKeyState(VK_CONTROL) < 0) // press Ctrl+7 to close the program
			{
				std::cout << "shutdown called" << std::endl;
				DestroyWindow(mainWnd);
			}
			if (pkh->vkCode == VkKeyScan('/') && GetKeyState(VK_CONTROL) < 0) // press Ctrl+/? to open about dialog
			{
				std::cout << "about called" << std::endl;
				DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, NULL, (DLGPROC)About);
			}
			else if (pkh->vkCode == VkKeyScan('0') && GetKeyState(VK_CONTROL) < 0) // press Ctrl+0 to bring to front
			{
				SetForegroundWindow(mainWnd);
				// SetWindowPos(mainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				// SetFocus(mainWnd);
			}
			else if (pkh->vkCode == VkKeyScan('=') && GetKeyState(VK_CONTROL) < 0) // CTRL+ '=' key is pressed magnify mouse cursor size
			{
				magnifyRegion += .2;
				std::cout << "Magnify win region: " << magnifyRegion << std::endl;
				// SetWindowRgn(hWnd,oldRegion,TRUE);
				if (winRegion)
				{
					DeleteObject(winRegion);
					DeleteObject(borderRegion);
				}
				winRegion = CreateWindowRegion(mainWnd, WIN_REGION_OFFSET_X, WIN_REGION_OFFSET_Y, magnifyRegion);
				borderRegion = CreateWindowRegion(mainWnd, REGION_BORDER_OFFSET_X, REGION_BORDER_OFFSET_Y, magnifyRegion - REGION_BORDER_MAGNIFY);
				SetWindowRgn(mainWnd, winRegion, TRUE);
				InvalidateRect(mainWnd, NULL, TRUE);
			}
			else if (pkh->vkCode == VkKeyScan('-') && GetKeyState(VK_CONTROL) < 0) // CTRL + '-' key is pressed decrease mouse cursor size
			{
				magnifyRegion -= .2;
				std::cout << "Magnify win region: " << magnifyRegion << std::endl;
				// SetWindowRgn(hWnd,oldRegion,TRUE);
				if (winRegion)
				{
					DeleteObject(winRegion);
					DeleteObject(borderRegion);
				}
				winRegion = CreateWindowRegion(mainWnd, WIN_REGION_OFFSET_X, WIN_REGION_OFFSET_Y, magnifyRegion);
				borderRegion = CreateWindowRegion(mainWnd, REGION_BORDER_OFFSET_X, REGION_BORDER_OFFSET_Y, magnifyRegion - REGION_BORDER_MAGNIFY);
				SetWindowRgn(mainWnd, winRegion, TRUE);
				InvalidateRect(mainWnd, NULL, TRUE);
			}
		}
		// return wkvn->KeyboardData(pkh->vkCode,Event.wParam);
	}
	else if (Event.dwHookType == WH_MOUSE) // mouse event
	{
		MSLLHOOKSTRUCT *pmh = (MSLLHOOKSTRUCT *)Event.lParam;
		HWND sourceHwnd = WindowFromPoint(pmh->pt);
		char tmp[50];
		if (Event.wParam == WM_LBUTTONDOWN)
		{
			std::cout << "Mouse left button down: X=" << pmh->pt.x << ", Y=" << pmh->pt.y << std::endl;
		}
		else if (Event.wParam == WM_MOUSEMOVE)
		{
			std::cout << "Mouse Move: HWND=" << (uintptr_t)sourceHwnd << ", X=" << pmh->pt.x << ", Y=" << pmh->pt.y << std::endl;
			SetWindowPos(mainWnd, HWND_TOPMOST, pmh->pt.x - 1, pmh->pt.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
			// return wkvn->MouseMoveData(pmh->pt.x,pmh->pt.y);
		}
	}
	return false;
}

// create and display the main application window along with setting up hooks
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance;					// Store instance handle in our global variable
	HWND hDummy = NULL;
	//hDummy = CreateWindowEx(0, TEXT("STATIC"),TEXT("HiddenParent"),WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);  
	//DWORD dwExStyle = WS_EX_TOOLWINDOW; // to hide from alt-tab
	DWORD dwExStyle = WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT; 
    DWORD dwStyle = WS_POPUP;
	// Define the desired client area rectangle
	RECT wr = {0, 0, 1500, 1500}; // Left, Top, Right, Bottom
	// Adjust the rectangle to account for the window's frame and title bar
	AdjustWindowRectEx(&wr, dwStyle, FALSE, dwExStyle);
	hWnd = CreateWindowEx(dwExStyle, szWindowClass, szTitle, dwStyle, wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top, hDummy, NULL, hInstance, NULL);
	if (!hWnd) return FALSE;
	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA); // Set window opacity to fully opaque (255)
	oldRegion = CreateRectRgn(0, 0, 0, 0);
	GetWindowRgn(hWnd, oldRegion);

	whiteBrsh = CreateSolidBrush(RGB(255, 255, 255));
	blackBrsh = CreateSolidBrush(RGB(0, 0, 0));

	winRegion = CreateWindowRegion(hWnd, WIN_REGION_OFFSET_X, WIN_REGION_OFFSET_Y, magnifyRegion);
	borderRegion = CreateWindowRegion(hWnd, REGION_BORDER_OFFSET_X, REGION_BORDER_OFFSET_Y, magnifyRegion - REGION_BORDER_MAGNIFY);
	SetWindowRgn(hWnd, winRegion, TRUE);
	SetNewMouseCursor();
	mainWnd = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	SetKeyboardHook(hWnd);
	SetMouseHook(hWnd);

	return TRUE;
}

// Clean up resources and remove hooks before exiting
void Shutdown()
{
	SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0); // Restore default cursors
	RemoveMouseHook();
	RemoveKeyboardHook();
	if (winRegion)
	{
		DeleteObject(winRegion);
		winRegion = NULL;
	}
	if (borderRegion)
	{
		DeleteObject(borderRegion);
	}
	if (blackBrsh)
	{
		DeleteObject(blackBrsh);
		DeleteObject(whiteBrsh);
	}
	FreeLibrary(GetModuleHandleA(KNMHOOKER_DLL));
	PostQuitMessage(0);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//  WM_COPYDATA  - receive data from the hook DLL
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
        // Allow applications to send WM_COPYDATA to us
        ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD); 
		break;
	case WM_COMMAND: // menu selections
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, NULL, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
	{
	}
	break;
	case WM_COPYDATA: // Data received from the hook DLL
		return (OnCopyHookData((COPYDATASTRUCT *)lParam));
	break;
	case WM_PAINT:
	{	// Paint the window with white fill and black border
		hdc = BeginPaint(hWnd, &ps);
		FillRgn(hdc, winRegion, whiteBrsh); // Fill the region
		FrameRgn(hdc, borderRegion, blackBrsh, REGION_BORDER_THICKNESS, REGION_BORDER_THICKNESS);// Draw a black border around the region
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		Shutdown();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
