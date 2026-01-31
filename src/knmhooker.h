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
	HHOOK g_hHook;
	HWND g_hWnd;
	HANDLE g_hInstance;
}GLOBALDATA;

// This is the core logic that makes the header versatile for both building and using the DLL.
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

#else

    // If we are using the DLL, we want to import its functions.
    #define API_DECL __declspec(dllimport)
    
    // For Dynamic Running of this DLL we can define function pointer types of each method.
    typedef BOOL (*SetKeyboardHookFunc)(HWND hWnd);
    typedef BOOL (*RemoveKeyboardHookFunc)();
    typedef BOOL (*SetMouseHookFunc)(HWND hWnd);
    typedef BOOL (*RemoveMouseHookFunc)();

    // Inline functions that return static Function pointers to hold the addresses of the DLL functions
    inline SetKeyboardHookFunc& GetSetKeyboardHookFunc()
    {
        static SetKeyboardHookFunc funcPtr = nullptr;
        return funcPtr;
    }

    inline RemoveKeyboardHookFunc& GetRemoveKeyboardHookFunc()
    {
        static RemoveKeyboardHookFunc funcPtr = nullptr;
        return funcPtr;
    }

    inline SetMouseHookFunc& GetSetMouseHookFunc()
    {
        static SetMouseHookFunc funcPtr = nullptr;
        return funcPtr;
    }

    inline RemoveMouseHookFunc& GetRemoveMouseHookFunc()
    {
        static RemoveMouseHookFunc funcPtr = nullptr;
        return funcPtr;
    }

    // For Dynamic Running of this DLL we can define function to load each method.
    // This function loads the DLL and retrieves the function pointers.
    // HINSTANCE hDll = LoadLibraryA("knmhooker.dll");
    BOOL LoadDllFunctions(HINSTANCE hDll)
    {
        if (!hDll) return FALSE;

        GetSetKeyboardHookFunc() = (SetKeyboardHookFunc)GetProcAddress(hDll, "SetKeyboardHook");
        GetRemoveKeyboardHookFunc() = (RemoveKeyboardHookFunc)GetProcAddress(hDll, "RemoveKeyboardHook");
        GetSetMouseHookFunc() = (SetMouseHookFunc)GetProcAddress(hDll, "SetMouseHook");
        GetRemoveMouseHookFunc() = (RemoveMouseHookFunc)GetProcAddress(hDll, "RemoveMouseHook");        

        // Ensure all function pointers were loaded successfully
        return (GetSetKeyboardHookFunc() && GetRemoveKeyboardHookFunc() && GetSetMouseHookFunc() && GetRemoveMouseHookFunc());
    }

    // Convenience macros to call the functions via the function pointers

    // Set a keyboard hook and send keyboard events to the specified window handle
    #define SetKeyboardHook GetSetKeyboardHookFunc()
    // Remove a previously set keyboard hook
    #define RemoveKeyboardHook GetRemoveKeyboardHookFunc()
    // Set a mouse hook and send mouse events to the specified window handle
    #define SetMouseHook GetSetMouseHookFunc()
    // Remove a previously set mouse hook
    #define RemoveMouseHook GetRemoveMouseHookFunc()

#endif


#endif