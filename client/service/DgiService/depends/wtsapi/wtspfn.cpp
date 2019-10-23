//
// Burlutsky Stanislav
// burluckij@gmail.com
//

#include "wtspfn.h"

namespace wtsapi
{
	LPVOID getFnAddr(const char* _name, const char* _libName = "wtsapi32.dll")
	{
		HMODULE hLib = LoadLibraryA(_libName);
		if (hLib == NULL)
		{
			return NULL;
		}

		if (_name == NULL)
		{
			return NULL;
		}

		return GetProcAddress(hLib, _name);
	}

	VOID WINAPI MyWTSFreeMemory(IN PVOID pMemory)
	{
		T_WTSFreeMemory pfn = static_cast<T_WTSFreeMemory>(getFnAddr("WTSFreeMemory"));
		if (pfn)
		{
			pfn(pMemory);
		}
	}

	DWORD WINAPI MyWTSGetActiveConsoleSessionId(VOID)
	{
		T_WTSGetActiveConsoleSessionId pfn = static_cast<T_WTSGetActiveConsoleSessionId>(getFnAddr("WTSGetActiveConsoleSessionId", "kernel32.dll"));
		if (pfn == NULL)
		{
			return 0;
		}

		return pfn();
	}

	BOOL WINAPI MyWTSQueryUserToken(ULONG SessionId, PHANDLE phToken)
	{
		T_WTSQueryUserToken pfn = static_cast<T_WTSQueryUserToken>(getFnAddr("WTSQueryUserToken"));
		if (pfn == NULL)
		{
			return FALSE;
		}

		return pfn(SessionId, phToken);
	}

	BOOL WINAPI MyWTSEnumerateSessionsA(
		IN HANDLE hServer,
		   IN DWORD Reserved,
		   IN DWORD Version,
		   PWTS_SESSION_INFOA* ppSessionInfo,
		   _Out_ DWORD* pCount)
	{
		//
		T_WTSEnumerateSessionsA pfn = static_cast<T_WTSEnumerateSessionsA>(getFnAddr("WTSEnumerateSessionsA"));
		if (pfn == NULL)
		{
			return FALSE;
		}

		return pfn(hServer, Reserved, Version, ppSessionInfo, pCount);
	}

	BOOL WINAPI MyWTSQuerySessionInformationA(
		IN HANDLE hServer,
		   IN DWORD SessionId,
		   IN WTS_INFO_CLASS WTSInfoClass,
		   LPSTR* ppBuffer,
		   _Out_ DWORD* pBytesReturned
	)
	{
		T_WTSQuerySessionInformationA pfn = static_cast<T_WTSQuerySessionInformationA>(getFnAddr("WTSQuerySessionInformationA"));
		if (pfn == NULL)
		{
			return FALSE;
		}
		return pfn(hServer, SessionId, WTSInfoClass, ppBuffer, pBytesReturned);
	}

	BOOL WINAPI MyWTSQuerySessionInformationW(
		IN HANDLE hServer,
		IN DWORD SessionId,
		IN WTS_INFO_CLASS WTSInfoClass,
		LPWSTR* ppBuffer,
		_Out_ DWORD* pBytesReturned
		)
	{
		T_WTSQuerySessionInformationW pfn = static_cast<T_WTSQuerySessionInformationW>(getFnAddr("WTSQuerySessionInformationW"));
		if (pfn == NULL)
		{
			return FALSE;
		}
		return pfn(hServer, SessionId, WTSInfoClass, ppBuffer, pBytesReturned);
	}

	BOOL WINAPI	MyWTSLogoffSession(
		_In_ HANDLE hServer,
		_In_ DWORD  SessionId,
		_In_ BOOL   bWait)
	{

		T_MyWTSLogoffSession pfn = static_cast<T_MyWTSLogoffSession>(getFnAddr("WTSLogoffSession"));
		if (pfn == NULL)
		{
			return FALSE;
		}
		return pfn(hServer, SessionId, bWait);
	}
}
