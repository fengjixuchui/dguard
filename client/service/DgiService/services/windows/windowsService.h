//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../../helpers/internal/log.h"
#include <Windows.h>
#include <Strsafe.h>

namespace service
{
	namespace windows
	{
		class DgiSrv
		{
		public:
			static bool start(LPTHREAD_START_ROUTINE _pfnServiceHandler, PVOID _contextArg);

		private:
			static void WINAPI mainRoutine(DWORD dwArgc, LPTSTR* lpszArgv);
			static void WINAPI ctrlHandler(DWORD dwCtrl);
			static void WINAPI reportEvent(LPTSTR szFunction);
			static void WINAPI reportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);



			static SERVICE_STATUS svcStatus;
			static PVOID pServiceContextArgument;
			static LPTHREAD_START_ROUTINE pfnServiceRoutine;
			static SERVICE_STATUS_HANDLE SvcStatusHandle;

			static const DWORD SvcErrorCode = (0xC0020001L);
		};
	}
}

