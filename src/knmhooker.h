// knmhooker.h : header file
// Defines the exported functions for the DLL application.

#ifndef KNMHOOKER_DLL // to avoid multiple inclusion
#define KNMHOOKER_DLL "knmhooker.dll"

#include <windows.h>

typedef struct
{
	int nCode;
	DWORD dwHookType;
	WPARAM wParam;
	LPARAM lParam;
}HEVENT;

typedef struct
{
    // Use 'unsigned __int64' to force 8-byte storage for handles, supporting both 32-bit and 64-bit processes
	unsigned __int64 g_hHook; // HHOOK
	unsigned __int64 g_hWnd; // HWND
	unsigned __int64 g_hInstance; // HINSTANCE
}GLOBALDATA;

// This is the core logic that makes the header versatile for both building and using the DLL.
// #define BUILDING_DLL // Define this when testing compilation of the DLL itself
#ifdef BUILDING_DLL

    // If we are building the DLL, we want to export our functions.
    #define API_DECL __declspec(dllexport)
    // Exported functions to be called by the main executable
    extern "C" {
        API_DECL BOOL SetKeyboardHook(HWND hWnd);
        API_DECL BOOL RemoveKeyboardHook();
        API_DECL BOOL SetMouseHook(HWND hWnd);
        API_DECL BOOL RemoveMouseHook();
    }

#else // BUILDING_DLL

    // If we are using the DLL, we want to import its functions.
    #define API_DECL __declspec(dllimport)
    
    // For Dynamic Running of this DLL we can define function pointer types of each method.
    typedef BOOL (*SetKeyboardHookFunc)(HWND hWnd);
    typedef BOOL (*RemoveKeyboardHookFunc)();
    typedef BOOL (*SetMouseHookFunc)(HWND hWnd);
    typedef BOOL (*RemoveMouseHookFunc)();

    // Set a keyboard hook and send keyboard events to the specified window handle
    SetKeyboardHookFunc SetKeyboardHook;
    // Remove a previously set keyboard hook
    RemoveKeyboardHookFunc RemoveKeyboardHook;
    // Set a mouse hook and send mouse events to the specified window handle
    SetMouseHookFunc SetMouseHook;
    // Remove a previously set mouse hook
    RemoveMouseHookFunc RemoveMouseHook;

    // For Dynamic Running of this DLL we can define function to load each method.
    // This function loads the DLL and retrieves the function pointers.
    // HINSTANCE hDll = LoadLibraryA("knmhooker.dll");
    BOOL LoadDllFunctions(HINSTANCE hDll)
    {
        if (!hDll) return FALSE;

        SetKeyboardHook = (SetKeyboardHookFunc)GetProcAddress(hDll, "SetKeyboardHook");
        RemoveKeyboardHook = (RemoveKeyboardHookFunc)GetProcAddress(hDll, "RemoveKeyboardHook");
        SetMouseHook = (SetMouseHookFunc)GetProcAddress(hDll, "SetMouseHook");
        RemoveMouseHook = (RemoveMouseHookFunc)GetProcAddress(hDll, "RemoveMouseHook");        

        // Ensure all function pointers were loaded successfully
        return (SetKeyboardHook && RemoveKeyboardHook && SetMouseHook && RemoveMouseHook);
    }

#endif // BUILDING_DLL

#endif // KNMHOOKER_DLL