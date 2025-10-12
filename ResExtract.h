//This function will extract a binary resource.
//
//IDR_KBHOOKER           BINARY  MOVEABLE PURE   "kbhooker.dll"

#include <windows.h>
#include <iostream>
#include <fstream>

void ExtractResource(const WORD nID, LPCTSTR szFilename)
{
	const HINSTANCE hInstance = GetModuleHandle(NULL);
	HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(nID), _T("BINARY"));// _ASSERTE(hResource);
	if (hResource == NULL) // no resource found.
		return;
	HGLOBAL hFileResource = LoadResource(hInstance, hResource);// _ASSERTE(hFileResource);
	LPVOID lpFile = LockResource(hFileResource);

	DWORD dwSize = SizeofResource(hInstance, hResource);

	// Open the file and filemap
	HANDLE hFile = CreateFile(szFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hFilemap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, dwSize, NULL);

	// Get a pointer to write to
	LPVOID lpBaseAddress = MapViewOfFile(hFilemap, FILE_MAP_WRITE, 0, 0, 0);

	// Write the file
	CopyMemory(lpBaseAddress, lpFile, dwSize);

	// Unmap the file and close the handles
	UnmapViewOfFile(lpBaseAddress);
	CloseHandle(hFilemap);
	CloseHandle(hFile);
	
}
//FindResourceA (HMODULE hModule, LPCSTR lpName, LPCSTR lpType);
bool ExtractResourceRc(LPCSTR ResourceId, LPCSTR ResourceType, const char* dllPath) {

	std::ifstream f(dllPath);
	if (f.good()) {
		std::cout << "DLL file already exists. Skipping extraction." << std::endl;
		return true; // File already exists, no need to extract
	}
	f.close();
	
    std::cout << "Extraction." << std::endl;

	 // --- 1. Find and load the resource from within this executable ---
    HRSRC hRes = FindResourceA(NULL, ResourceId, ResourceType);
    if (!hRes) {
        std::cerr << "Error: Could not find the DLL resource." << std::endl;
        return false;
    }

    HGLOBAL hResLoad = LoadResource(NULL, hRes);
    if (!hResLoad) {
        std::cerr << "Error: Could not load the DLL resource." << std::endl;
        return false;
    }

    LPVOID pResData = LockResource(hResLoad);
    if (!pResData) {
        std::cerr << "Error: Could not lock the DLL resource." << std::endl;
        return false;
    }

    DWORD dwSize = SizeofResource(NULL, hRes);

    // --- 2. Write the resource data to a new file on disk ---
    //const char* dllPath = "hellodll.dll";
    std::ofstream dllFile(dllPath, std::ios::binary);
    if (!dllFile.is_open()) {
        std::cerr << "Error: Could not create DLL file on disk." << std::endl;
        return false;
    }
    dllFile.write(static_cast<const char*>(pResData), dwSize);
    dllFile.close();
    std::cout << "Successfully extracted hellodll.dll to the current directory." << std::endl;
	return true;
}