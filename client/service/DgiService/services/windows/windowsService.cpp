//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "../../stdafx.h"


#include <filesystem>

#include "windowsService.h"
#include "../../helpers/internal/helpers.h"
// #include "thrift_server/services/manager/synchronizationServices.h"
#include "../../logic/common/DgiEngine.h"

namespace service
{
	namespace windows
	{
		SERVICE_STATUS DgiSrv::svcStatus;
		PVOID DgiSrv::pServiceContextArgument;
		LPTHREAD_START_ROUTINE DgiSrv::pfnServiceRoutine;
		SERVICE_STATUS_HANDLE DgiSrv::SvcStatusHandle;

		logfile& getLog()
		{
			return ::logic::common::DgiEngine::getConf().getLog(LOG_MAIN_SRV);
		}

		bool DgiSrv::start(LPTHREAD_START_ROUTINE _pfnServiceHandler, PVOID _pContextArg)
		{
			logfile& log = getLog();
			SERVICE_TABLE_ENTRY DispatchTable[] =
			{
				{ WINDOWS_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)mainRoutine },
				{ NULL, NULL }
			};

			pServiceContextArgument = _pContextArg;
			pfnServiceRoutine = _pfnServiceHandler;

			//
			//	This call returns when the service has stopped. 
			//	The process should simply terminate when the call returns.
			//

			if (!StartServiceCtrlDispatcher(DispatchTable))
			{
				log.printLastErrorCode("StartServiceCtrlDispatcher failed");
				reportEvent(TEXT("StartServiceCtrlDispatcher"));
				return false;
			}

			log.print("The service stopped.");

			return true;
		}

		void DgiSrv::mainRoutine(DWORD dwArgc, LPTSTR* lpszArgv)
		{
			auto fn = std::string(__FUNCTION__);

			logfile& log = getLog();

			// Register the handler function for the service.
			SvcStatusHandle = RegisterServiceCtrlHandlerW(WINDOWS_SERVICE_NAME, ctrlHandler);
			if (!SvcStatusHandle)
			{
				log.printLastErrorCode(fn + ": RegisterServiceCtrlHandler failed");
				reportEvent(TEXT("RegisterServiceCtrlHandler"));
				return;
			}

			// These SERVICE_STATUS members remain as set here.
			svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
			svcStatus.dwServiceSpecificExitCode = 0;

			// Report initial status to the SCM.
			reportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

			DWORD tid;
			auto serviceRoutineThread = CreateThread(NULL,
				1024 * 1024 * 10, // Stack size - 10 MB.
				(LPTHREAD_START_ROUTINE)pfnServiceRoutine,
				pServiceContextArgument,
				0,
				&tid);

			if (!serviceRoutineThread)
			{
				log.printLastErrorCode(fn + ": error - couldn't create main service worker(..) thread.");
				reportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
				return;
			}

			// Report running status when initialization is complete.
			reportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
			log.print(fn + ": service thread is running");

			//
			//	Wait for termination signals.
			//

			bool wokeupReason = ::logic::common::DgiEngine::getSync().waitForProcessTermination();

			if (wokeupReason)
			{
				log.print(fn + ": terminating process event is received. Start completion procedures.");
				// sync::SysEvents::get().raiseEventShutdownThriftServices();

				// Send to windows service manager pending messages until main Data Guard thread finished.
				//
				while ( WaitForSingleObject(serviceRoutineThread, (1000-50) ) != WAIT_OBJECT_0)
				{
					log.print(fn + ": process stopping is delayed. Terminating is continuing.");

					reportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);
				}

				log.print(fn + ": internal services were stopped. Operation completed correctly.");
			}
			else
			{
				log.print(fn + ": error - service thread was awoken but by the error.");
			}

			log.print(fn + ": windows service stopped.");
			reportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
			CloseHandle(serviceRoutineThread);
		}

		void DgiSrv::ctrlHandler(DWORD dwCtrl)
		{
			auto fn = std::string(__FUNCTION__);
			logfile& log = getLog();

			switch (dwCtrl)
			{
			case SERVICE_CONTROL_STOP:
				reportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 1000);

				log.print(fn + ": SERVICE_CONTROL_STOP received and pended.");

				::logic::common::DgiEngine::getSync().setProcessTermination();
				::logic::common::DgiEngine::shutdown();

				// sync::SysEvents::get().raiseEventCompleteProcess();
				break;

			case SERVICE_CONTROL_INTERROGATE:
				log.print(fn + ": SERVICE_CONTROL_INTERROGATE");
				break;

			case SERVICE_CONTROL_SHUTDOWN:
				log.print(fn + ": SERVICE_CONTROL_SHUTDOWN");
				// sync::SysEvents::get().raiseEventShutdownThriftServices();
				// sync::SysEvents::get().raiseEventCompleteProcess();
				::logic::common::DgiEngine::getSync().setShutdown();
				::logic::common::DgiEngine::getSync().setProcessTermination();

				::logic::common::DgiEngine::shutdown();

				//
				//	To do any clean up tasks here...
				//

				break;

			default:
				log.printEx("%s: was called with control code 0x%x.", __FUNCTION__, dwCtrl);
				break;
			}
		}

		void DgiSrv::reportEvent(LPTSTR szFunction)
		{
			HANDLE hEventSource;
			LPCTSTR lpszStrings[2];
			TCHAR Buffer[80];

			hEventSource = RegisterEventSource(NULL, WINDOWS_SERVICE_NAME);

			if (NULL != hEventSource)
			{
				StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

				lpszStrings[0] = WINDOWS_SERVICE_NAME;
				lpszStrings[1] = Buffer;

				ReportEvent(hEventSource,// event log handle
					EVENTLOG_ERROR_TYPE, // event type
					0, // event categorys
					DgiSrv::SvcErrorCode, // event identifier
					NULL, // no security identifier
					2, // size of lpszStrings array
					0, // no binary data
					lpszStrings, // array of strings
					NULL); // no binary data

				DeregisterEventSource(hEventSource);
			}
		}

		void DgiSrv::reportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
		{
			static DWORD dwCheckPoint = 1;

			// Fill in the SERVICE_STATUS structure.
			svcStatus.dwCurrentState = dwCurrentState;
			svcStatus.dwWin32ExitCode = dwWin32ExitCode;
			svcStatus.dwWaitHint = dwWaitHint;

			if (dwCurrentState == SERVICE_START_PENDING)
			{
				svcStatus.dwControlsAccepted = 0;
			}
			else
			{
				svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
			}

			if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
			{
				svcStatus.dwCheckPoint = 0;
			}
			else
			{
				svcStatus.dwCheckPoint = dwCheckPoint++;
			}

			// Report the status of the service to the SCM.
			SetServiceStatus(SvcStatusHandle, &svcStatus);
		}
	}
}
