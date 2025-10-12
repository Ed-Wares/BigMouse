// BigMouse.cpp : Defines the entry point for the application.
// This application uses a DLL to set low-level keyboard and mouse hooks and displays a magnified mouse cursor.

#include "stdafx.h"
#include "BigMouse.h"
#include "knmhooker.h"
#include "ResExtract.h"
#include <WinUser.h>

#define MAX_LOADSTRING 100
#define MOUSEEVENTF_WHEEL 0x0800 
#define WM_MOUSEWHEEL 0x020A
#define OCR_NORMAL 32512
#define OCR_IBEAM 32513

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HWND mainWnd;
HRGN oldRegion;

HCURSOR oldCursor;
HCURSOR newCursor;

double magnify = 3;

// Function to redirect standard I/O to a new console window for debugging.
void RedirectIOToConsole() {
    // 1. Create a new console window
    if (!AllocConsole()) {
        // Handle error if console could not be created
        //MessageBox(NULL, "Failed to create the console.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // 2. Re-open standard I/O streams to the new console
    // This is the tricky part that makes std::cout and std::cin work.
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);

    // 3. Clear the iostream state
    std::cout.clear();
    std::cin.clear();
    std::cerr.clear();
}

// The main entry point for the Windows application.
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	if (lpCmdLine && *lpCmdLine) 
	{
		// Handle command line arguments if needed
		if(strcmp((char *)lpCmdLine, "/debug") == 0 || strcmp((char *)lpCmdLine, "-debug") == 0) 
		{
			RedirectIOToConsole(); // Open a console window for debugging output
		}
	}

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BIGMOUSE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Extract the DLL resource if it doesn't already exist
	//ExtractResource(IDR_KNMHOOKER, KNMHOOKER_FILE);
	ExtractResourceRc("KNMHOOKER_FILE", "DLL", "kbhooker.dll");
	BOOL dllStatus = LoadDllFunctions(LoadLibraryA("kbhooker.dll"));
	if (!dllStatus) {
		MessageBoxA(NULL, "Failed to load hook functions from DLL.", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
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

	return (int) msg.wParam;
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

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_BIGMOUSE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_BIGMOUSE;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

// Set a new custom cursor (a blank cursor in this case)
void SetNewMouseCursor()
{
	//oldCursor = GetCursor(); //save current cursor, which might be IDC_WAIT
	oldCursor = LoadCursor(NULL, IDC_ARROW); //Loads the standard arrow cursor
	oldCursor = CopyCursor(oldCursor);
	unsigned char andbits[128];
	unsigned char xorbits[128];
	for (int i = 0 ; i < 128 ; i++)
	{
		andbits[i] = 255;
		xorbits[i] = 0;
	}
	newCursor = CreateCursor((HINSTANCE)GetModuleHandle(NULL), 0, 0, 32, 32, andbits, xorbits);
	//HCURSOR hCursor = LoadCursor(NULL, IDC_WAIT);
	SetSystemCursor(newCursor, OCR_NORMAL);
}

// Process the data received from the hook DLL
bool OnCopyHookData(COPYDATASTRUCT* pCopyDataStruct) // WM_COPYDATA lParam will have this struct
{
	if( pCopyDataStruct->cbData!=sizeof(HEVENT))
		return false;
	HEVENT Event;
	memcpy(&Event, (HEVENT*)pCopyDataStruct->lpData, sizeof(HEVENT)); // transfer data to internal variable
	if (Event.dwHookType == WH_KEYBOARD) // keyboard event
	{
		KBDLLHOOKSTRUCT* pkh = (KBDLLHOOKSTRUCT*) Event.lParam;
		if (Event.wParam == WM_KEYDOWN || Event.wParam == WM_SYSKEYDOWN)
		{
			std::cout << "Key Down: " << pkh->vkCode << std::endl;
			if (pkh->vkCode==VkKeyScan('7') && GetKeyState(VK_CONTROL) < 0) // press Ctrl+7 to close the program
			{
				DestroyWindow(mainWnd);
			}
			else if (pkh->vkCode==VkKeyScan('0') && GetKeyState(VK_CONTROL) < 0) // press Ctrl+0 to bring to front
			{
				SetForegroundWindow(mainWnd);
				//SetWindowPos(mainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				//SetFocus(mainWnd);
			}
		}
		//return wkvn->KeyboardData(pkh->vkCode,Event.wParam);
	}
	else if (Event.dwHookType == WH_MOUSE) // mouse event
	{
		MSLLHOOKSTRUCT* pmh = (MSLLHOOKSTRUCT*) Event.lParam;
		char tmp[50];
		if (Event.wParam == WM_LBUTTONDOWN)
		{
			std::cout << "Mouse left button down: X=" << pmh->pt.x << ", Y=" << pmh->pt.y << std::endl;
		}
		else if (Event.wParam == WM_MOUSEMOVE)
		{
			std::cout << "Mouse Move: X=" << pmh->pt.x << ", Y=" << pmh->pt.y << std::endl;
			SetWindowPos(mainWnd, HWND_TOPMOST, pmh->pt.x-2, pmh->pt.y-2, 0 ,0, SWP_NOSIZE); 
			//return wkvn->MouseMoveData(pmh->pt.x,pmh->pt.y);
		}
	}
	return false;
}

// Create a custom-shaped region for the window
HRGN CreateWindowRegion(HWND win,int x ,int y)
{
	HRGN wrg;
	POINT pol[7];
	HRGN rg;
	pol[0].x = x; pol[0].y = (y);
	pol[1].x = x + (100 * magnify); pol[1].y = y + (180 * magnify);
	pol[2].x = x + (55 * magnify); pol[2].y = y + (197 * magnify);
	pol[3].x = x + (80 * magnify); pol[3].y = y + (250 * magnify);
	pol[4].x = x + (70 * magnify); pol[4].y = y + (250 * magnify);
	pol[5].x = x + (45 * magnify); pol[5].y = y + (200 * magnify);
	pol[6].x = x; pol[6].y = y + (220 * magnify);
	rg = CreatePolygonRgn(pol,7,ALTERNATE);
	return rg;
}


// create and display the main application window along with setting up hooks
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   //hWnd = CreateWindow(szWindowClass, szTitle, WS_DLGFRAME, 0, 0, 800, 900, NULL, NULL, hInstance, NULL);
   hWnd = CreateWindowEx(WS_EX_TOOLWINDOW,szWindowClass, szTitle, WS_DLGFRAME, 0, 0, 800, 900, NULL, NULL, hInstance, NULL);
   if (!hWnd)
   {
      return FALSE;
   }
   oldRegion = CreateRectRgn(0,0,0,0);
   GetWindowRgn(hWnd, oldRegion);
   HRGN rg = CreateWindowRegion(hWnd,4,GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYSIZE) + 5);
   SetWindowRgn(hWnd,rg,TRUE);
   DeleteObject(rg);
   SetNewMouseCursor();
   mainWnd = hWnd;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   SetKeyboardHookFp(hWnd);
   SetMouseHookFp(hWnd);

   return TRUE;
}

// Clean up resources and remove hooks before exiting
void Shutdown()
{
	RemoveMouseHookFp();
	RemoveKeyboardHookFp();
	SetSystemCursor(oldCursor, OCR_NORMAL); //restore mouse
	DestroyCursor(newCursor);
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
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
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
			if (VkKeyScan('+') == wParam || wParam == 187)
			{
				magnify += .2;
				//SetWindowRgn(hWnd,oldRegion,TRUE);
				HRGN rg = CreateWindowRegion(hWnd,4,GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYSIZE) + 5);
				SetWindowRgn(hWnd,rg,TRUE);
				DeleteObject(rg);                
				InvalidateRect(hWnd, NULL, TRUE);
			}
			else if (VkKeyScan('-') == wParam)
			{
				magnify -= .2;
				//SetWindowRgn(hWnd,oldRegion,TRUE);
				HRGN rg = CreateWindowRegion(hWnd,4,GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYSIZE) + 5);
				SetWindowRgn(hWnd,rg,TRUE);
				DeleteObject(rg);
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		break;
		case WM_COPYDATA:
			return (OnCopyHookData((COPYDATASTRUCT *) lParam));
		break;
		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);
			HRGN rg = CreateWindowRegion(hWnd,1,8);
			//GetWindowRgn(hWnd, rg);
			HBRUSH hbrsh = CreateSolidBrush(RGB(0, 0, 0));
			FrameRgn(hdc, rg, hbrsh, 2, 2);
			DeleteObject(rg);
			DeleteObject(hbrsh);
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

