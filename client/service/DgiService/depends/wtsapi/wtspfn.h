//
// Burlutsky Stanislav
// burluckij@gmail.com
//

#pragma once

#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

#include <Wtsapi32.h>


namespace wtsapi
{
	//
	// Type declarations.
	//

	typedef BOOL (WINAPI *T_WTSQueryUserToken)(ULONG SessionId, PHANDLE phToken);

	typedef DWORD (WINAPI *T_WTSGetActiveConsoleSessionId)(VOID);

	typedef BOOL (WINAPI *T_WTSEnumerateSessionsA)(
		IN HANDLE hServer,
		   IN DWORD Reserved,
		   IN DWORD Version,
		   PWTS_SESSION_INFOA* ppSessionInfo,
		   _Out_ DWORD* pCount);

	typedef BOOL (WINAPI *T_WTSQuerySessionInformationA)(
		IN HANDLE hServer,
		   IN DWORD SessionId,
		   IN WTS_INFO_CLASS WTSInfoClass,
		   LPSTR* ppBuffer,
		   _Out_ DWORD* pBytesReturned
	);

	typedef BOOL(WINAPI *T_WTSQuerySessionInformationW)(
		IN HANDLE hServer,
		IN DWORD SessionId,
		IN WTS_INFO_CLASS WTSInfoClass,
		LPWSTR* ppBuffer,
		_Out_ DWORD* pBytesReturned);

	typedef	BOOL(WINAPI	*T_MyWTSLogoffSession)(
		_In_ HANDLE hServer,
		_In_ DWORD  SessionId,
		_In_ BOOL   bWait);

	typedef VOID (WINAPI *T_WTSFreeMemory)(IN PVOID pMemory);

	//
	// Stubs.
	// We should use this functions because msdn says that:
	// Minimum supported client for Wtsapi32 is Windows Vista.
	// Proof: https://msdn.microsoft.com/en-us/library/aa383840(v=vs.85).aspx
	// But! It works on my Windows XP 32 bit with Service pack 3.
	//

	VOID WINAPI MyWTSFreeMemory(IN PVOID pMemory);

	DWORD WINAPI MyWTSGetActiveConsoleSessionId(VOID);

	BOOL WINAPI MyWTSQueryUserToken(ULONG SessionId, PHANDLE phToken);

	BOOL WINAPI MyWTSEnumerateSessionsA(
		IN HANDLE hServer,
		   IN DWORD Reserved,
		   IN DWORD Version,
		   PWTS_SESSION_INFOA* ppSessionInfo,
		   _Out_ DWORD* pCount);

	BOOL WINAPI MyWTSQuerySessionInformationA(
		IN HANDLE hServer,
		   IN DWORD SessionId,
		   IN WTS_INFO_CLASS WTSInfoClass,
		   LPSTR* ppBuffer,
		   _Out_ DWORD* pBytesReturned
	);

	BOOL WINAPI MyWTSQuerySessionInformationW(
		IN HANDLE hServer,
		IN DWORD SessionId,
		IN WTS_INFO_CLASS WTSInfoClass,
		LPWSTR* ppBuffer,
		_Out_ DWORD* pBytesReturned
		);

	BOOL WINAPI	MyWTSLogoffSession(
		_In_ HANDLE hServer,
		_In_ DWORD  SessionId,
		_In_ BOOL   bWait);
}
