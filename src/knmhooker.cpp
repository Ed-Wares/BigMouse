// knmhooker.cpp : Defines the exported functions for the DLL application.
// This DLL sets low-level keyboard and mouse hooks and communicates events to a controlling application via WM_COPYDATA.

#include "knmhooker.h"// Include our smart header
#include <windows.h>
#include <stdlib.h>
#include <Psapi.h>
#include <stdio.h>

#define WH_KEYBOARD_LL 13 // only works for NT-XP+!
#define WH_MOUSE_LL 14 // only works for NT-XP+!


HANDLE hMappedFileKeyboard;
HANDLE hMappedFileMouse;
GLOBALDATA* pDataKeyboard;
GLOBALDATA* pDataMouse;
bool bStartingProcessKeyboard = false;
bool bStartingProcessMouse = false;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:      // the DLL is attached to a process. we have to set the MMF
			{
				DisableThreadLibraryCalls((HMODULE)hModule);
				char szBaseName[_MAX_FNAME]="\0", szTmp[_MAX_FNAME];
				char keyboardFN[_MAX_FNAME];
				char mouseFN[_MAX_FNAME];
				if (GetModuleBaseNameA(GetCurrentProcess(), (HMODULE)hModule, szTmp, sizeof(szTmp)))// compute MMF-filename from current module base name, uses Psapi
				_splitpath(szTmp, NULL, NULL, szBaseName, NULL);
				sprintf(keyboardFN,"%sKbHookSharedMem",szBaseName);
				sprintf(mouseFN,"%sMsHookSharedMem",szBaseName);

				hMappedFileKeyboard = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(GLOBALDATA), keyboardFN);
				pDataKeyboard = (GLOBALDATA*)MapViewOfFile(hMappedFileKeyboard, FILE_MAP_WRITE, 0, 0, 0);
				bStartingProcessKeyboard = (hMappedFileKeyboard != NULL) && (GetLastError() != ERROR_ALREADY_EXISTS);

				hMappedFileMouse = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(GLOBALDATA), mouseFN);
				pDataMouse = (GLOBALDATA*)MapViewOfFile(hMappedFileMouse, FILE_MAP_WRITE, 0, 0, 0);
				bStartingProcessMouse = (hMappedFileMouse != NULL) && (GetLastError() != ERROR_ALREADY_EXISTS);

				if(bStartingProcessKeyboard)       // if the MMF doesn't exist, we have the first instance
				{
					pDataKeyboard->g_hInstance  = (unsigned __int64)hModule;  // so set the instance handle
					pDataKeyboard->g_hWnd       = 0;     // and initialize the other handles
					pDataKeyboard->g_hHook      = 0;
				}

				if(bStartingProcessMouse)       // if the MMF doesn't exist, we have the first instance
				{
					pDataMouse->g_hInstance  = (unsigned __int64)hModule;  // so set the instance handle
					pDataMouse->g_hWnd       = 0;     // and initialize the other handles
					pDataMouse->g_hHook      = 0;
				}
			}
		break;

		case DLL_PROCESS_DETACH:
			CloseHandle(hMappedFileKeyboard);     // on detaching the DLL, close the MMF
			CloseHandle(hMappedFileMouse);     // on detaching the DLL, close the MMF
		break;
	}    
    return TRUE;
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{   
	// Always call next hook if nCode < 0
    if (nCode < 0) return CallNextHookEx((HHOOK)pDataKeyboard->g_hHook, nCode, wParam, lParam);	
	
	COPYDATASTRUCT  CDS;
	HEVENT          Event;

	CDS.dwData = 0;
	CDS.cbData = sizeof(Event);
	CDS.lpData = &Event;

	Event.lParam      = lParam;
	Event.wParam      = wParam;
	Event.nCode       = nCode;
	Event.dwHookType  = WH_KEYBOARD;

	//BOOL bRes = SendMessage((HWND)pDataKeyboard->g_hWnd, WM_COPYDATA, 0, (LPARAM)(VOID*)&CDS); 
	// Send safely using Timeout (Prevents source from freezing if target is unresponsive)
	DWORD_PTR dwResult;
	BOOL bRes = SendMessageTimeout((HWND)pDataKeyboard->g_hWnd, WM_COPYDATA, 0, (LPARAM)(VOID*)&CDS, SMTO_ABORTIFHUNG | SMTO_NORMAL, 100, &dwResult);
	return CallNextHookEx((HHOOK)pDataKeyboard->g_hHook, nCode, wParam, lParam);  // pass hook to next handler
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{   
	// Always call next hook if nCode < 0
    if (nCode < 0) return CallNextHookEx((HHOOK)pDataKeyboard->g_hHook, nCode, wParam, lParam);	

	COPYDATASTRUCT  CDS;
	HEVENT          Event;

	CDS.dwData = 0;
	CDS.cbData = sizeof(Event);
	CDS.lpData = &Event;

	Event.lParam      = lParam;
	Event.wParam      = wParam;
	Event.nCode       = nCode;
	Event.dwHookType  = WH_MOUSE;
	//LRESULT lRes = SendMessage((HWND)pDataMouse->g_hWnd, WM_COPYDATA, 0, (LPARAM)(VOID*)&CDS); // ask the controlling program if the hook should be passed
	// Send safely using Timeout (Prevents source from freezing if target is unresponsive)
	DWORD_PTR dwResult;
	BOOL bRes = SendMessageTimeout((HWND)pDataMouse->g_hWnd, WM_COPYDATA, 0, (LPARAM)(VOID*)&CDS, SMTO_ABORTIFHUNG | SMTO_NORMAL, 100, &dwResult);
	return CallNextHookEx((HHOOK)pDataMouse->g_hHook, nCode, wParam, lParam);  // pass hook to next handler
}

// This is the core logic that makes the header versatile for both building and using the DLL.
#ifdef BUILDING_DLL
BOOL SetKeyboardHook(HWND hWnd)
{
	if(bStartingProcessKeyboard) // if we're just starting the DLL for the first time,
	{
		pDataKeyboard->g_hWnd   = (unsigned __int64)hWnd; // remember the windows and hook handle for further instances
		pDataKeyboard->g_hHook  = (unsigned __int64)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardHookProc, (HINSTANCE)pDataKeyboard->g_hInstance, 0);   
		return(pDataKeyboard->g_hHook != 0);
	}
	else 
		return(false);
}

BOOL RemoveKeyboardHook()
{
	if(pDataKeyboard->g_hHook)       // if the hook is defined
	{
		pDataKeyboard->g_hWnd = 0;  // reset data
		pDataKeyboard->g_hHook = 0;
		return UnhookWindowsHookEx((HHOOK)pDataKeyboard->g_hHook);
	}
	return(true);
}

BOOL SetMouseHook(HWND hWnd)
{
	if(bStartingProcessMouse) // if we're just starting the DLL for the first time,
	{
		pDataMouse->g_hWnd   = (unsigned __int64)hWnd; // remember the windows and hook handle for further instances
		pDataMouse->g_hHook  = (unsigned __int64)SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseHookProc, (HINSTANCE)pDataMouse->g_hInstance, 0);   
		return(pDataMouse->g_hHook != 0);
	}
	else 
		return(false);
}

BOOL RemoveMouseHook()
{
	if(pDataMouse->g_hHook)       // if the hook is defined
	{
		pDataMouse->g_hWnd = 0;  // reset data
		pDataMouse->g_hHook = 0;
		return UnhookWindowsHookEx((HHOOK)pDataMouse->g_hHook);
	}
	return(true);
}

#endif // BUILDING_DLL