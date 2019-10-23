#include <windows.h>
#include <Guiddef.h>

#include "SziFileEraseRegCom.h"
#include "SziFileEraseMenu.h"

#define MENU_FILE_ALL					L"*"
#define MENU_FILE_DIRECTORY				L"Directory"
#define SHELLEXTFRIENDLYNAME			L"SziFileErase.FileContextMenuExt"
#define SHELLEXTFRIENDLYNAMECLASS		L"SziFileErase.FileContextMenuExt Class"

// {DDD8FFAE-BBA2-47E4-B537-DDE94356E0C1}
const GUID	CLSID_SziFileEraseDll = { 0xddd8ffae, 0xbba2, 0x47e4, { 0xb5, 0x37, 0xdd, 0xe9, 0x43, 0x56, 0xe0, 0xc1 } };
HINSTANCE   gInstance = NULL;
long        gDllRef = 0;


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) 
{
	switch(dwReason) 
	{
		case DLL_PROCESS_ATTACH:
			gInstance = hModule;
			DisableThreadLibraryCalls(hModule);
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) 
{
	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

	if(IsEqualCLSID(CLSID_SziFileEraseDll, rclsid)) 
	{
		szi::ClassFactory * pClassFactory = new (std::nothrow) szi::ClassFactory();
		if(pClassFactory)
		{
			hr = pClassFactory->QueryInterface(riid, ppv);
			pClassFactory->Release();
		} else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

STDAPI DllCanUnloadNow(void) 
{
	return gDllRef > 0 ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer(void) 
{
	HRESULT hr;
	wchar_t szModule[MAX_PATH] = L"\0";

	if(GetModuleFileName(gInstance, szModule, ARRAYSIZE(szModule)) == 0) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	hr = RegisterInprocServer(szModule, CLSID_SziFileEraseDll, SHELLEXTFRIENDLYNAMECLASS, L"Apartment");
	if(SUCCEEDED(hr))
	{
		hr = RegisterShellExtContextMenuHandler(MENU_FILE_ALL, CLSID_SziFileEraseDll, SHELLEXTFRIENDLYNAME);
		if(SUCCEEDED(hr))
		{
			hr = RegisterShellExtContextMenuHandler(MENU_FILE_DIRECTORY, CLSID_SziFileEraseDll, SHELLEXTFRIENDLYNAME);
			if(FAILED(hr))
			{
				UnregisterShellExtContextMenuHandler(MENU_FILE_ALL, CLSID_SziFileEraseDll);
			}
		}
	}
	return hr;
}

STDAPI DllUnregisterServer(void) 
{
	HRESULT hr = S_OK;
	wchar_t szModule[MAX_PATH] = L"\0";

	if(GetModuleFileName(gInstance, szModule, ARRAYSIZE(szModule)) == 0) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	
	hr = UnregisterInprocServer(CLSID_SziFileEraseDll);
	if(SUCCEEDED(hr))
	{
		UnregisterShellExtContextMenuHandler(MENU_FILE_ALL, CLSID_SziFileEraseDll);		
		UnregisterShellExtContextMenuHandler(MENU_FILE_DIRECTORY, CLSID_SziFileEraseDll);
	}

	return hr;
}