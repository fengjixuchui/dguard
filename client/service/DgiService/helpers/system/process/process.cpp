//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "process.h"
#include <Windows.h>

#ifndef _PSAPI_H_
#include <Psapi.h>
#endif

#ifndef _INC_TOOLHELP32
#include <TlHelp32.h>
#endif

#include "../../internal/helpers.h"
//#include "../../user manager/userManagerHelpers.h"

#pragma comment(lib, "psapi")

namespace sys
{
	namespace process
	{
		ListModules getModules(unsigned long _pid)
		{
			ListModules modules;

			MODULEENTRY32W me32;
			HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
			PMODULEENTRY32W the_module = NULL;

			hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _pid);
			if (hModuleSnap != INVALID_HANDLE_VALUE)
			{
				me32.dwSize = sizeof(MODULEENTRY32W);
				if (Module32FirstW(hModuleSnap, &me32))
				{
					do
					{
						ModuleInformation info;
						info.pid = _pid;
						info.pBase = me32.modBaseAddr;
						info.memorySize = me32.modBaseSize;
						info.exeName = me32.szModule;
						info.exePath = me32.szExePath;

						modules.push_back(info);

					} while (Module32NextW(hModuleSnap, &me32));
				}

				CloseHandle(hModuleSnap);
			}

			return modules;
		}

		ListProcesses getProcesses()
		{
			ListProcesses processes;

			HANDLE hProcessSnap = INVALID_HANDLE_VALUE;
			PROCESSENTRY32W pe32;
			PPROCESSENTRY32W the_process = NULL;

			hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if ((hProcessSnap != INVALID_HANDLE_VALUE))
			{
				pe32.dwSize = sizeof(PROCESSENTRY32W);
				if (Process32FirstW(hProcessSnap, &pe32))
				{
					do
					{
						ProcessInformation pi;
						pi.pid = pe32.th32ProcessID;
						pi.parentPid = pe32.th32ParentProcessID;
						pi.exeName = pe32.szExeFile;
						pi.modules = std::move(getModules(pe32.th32ProcessID));

						processes.push_back(pi);

					} while (Process32NextW(hProcessSnap, &pe32));
				}

				CloseHandle(hProcessSnap);
			}

			return processes;
		}

		State kill(std::wstring _processName)
		{
			auto killed = State::PsNotFound;

			sys::process::ListProcesses processes = sys::process::getProcesses();
			for (auto process : processes)
			{
				if (strings::equalStrings(_processName, process.exeName))
				{
					killed = kill(process.pid);
				}
			}

			return killed;
		}

		sys::process::State kill(unsigned long _pid)
		{
			auto killed = State::PsNoAccess;

			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, _pid);
			if (hProcess != NULL)
			{
				if (TerminateProcess(hProcess, 1))
				{
					killed = State::PsKilled;
				}
				else
				{
					//killed = State::
				}

				CloseHandle(hProcess);
			}
			
			return killed;
		}

		bool killAndWait(DWORD _parentPid, DWORD _waitTime /* = 5 * 1000 */)
		{
			bool killed = false;
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, _parentPid);
			if (hProcess)
			{
				if (killed = (TerminateProcess(hProcess, 0) == TRUE))
				{
					WaitForSingleObject(hProcess, _waitTime);
				}

				CloseHandle(hProcess);
			}

			return killed;
		}

		bool lookupProcess(IMyProcess& _search, std::vector<ProcessInformation> & _outInfo)
		{
			bool found = false;
			sys::process::ListProcesses processes = sys::process::getProcesses();
			for (auto proc : processes)
			{
				if (_search.isItMine(proc))
				{
					_outInfo.push_back(proc);
					found = true;
				}
			}

			return found;
		}

		bool createChild(std::wstring _exeFilePath, std::wstring _commandLine, unsigned long& _outLastError, PROCESS_INFORMATION& _outPi)
		{
			STARTUPINFOW si = { 0 };
			PROCESS_INFORMATION pi = { 0 };
			SECURITY_ATTRIBUTES sa = { 0 };
			wchar_t szCmdLine[32768 + 1] = { 0 };

			si.cb = sizeof( si );
			sa.bInheritHandle = true;
			sa.lpSecurityDescriptor = NULL;
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);

			// Send to the child process next arguments in a command line:
			// 1. PID of the parent process;
			// 2. ID of the last updater version.
			//wsprintfA(szCmdLine, "child_of:%d#package_id:%I64d", GetCurrentProcessId(), _distrId);

			wcscpy(szCmdLine, _commandLine.c_str());

			if (CreateProcessW(_exeFilePath.c_str(),
				szCmdLine,
				&sa,
				&sa,
				true,
				0,
				0,
				0,
				&si,
				&pi))
			{
				_outPi = pi;
				return true;

// 				// Here we should wait when child process kills parent.
// 				WaitForSingleObject(pi.hThread, INFINITE);
// 
// 				CloseHandle(pi.hThread);
// 				CloseHandle(pi.hProcess);
			}
			
			_outLastError = GetLastError();
			return false;
		}

// 		void runInteractive(std::wstring _exeFilePath, std::wstring _commandLine)
// 		{
// 			bool success = false;
// 			int errorCode = 0;
// 			HANDLE hUserSessionToken = user_manager::getTokenOfActiveUser(&errorCode);
// 
// 			if (success = (hUserSessionToken != 0))
// 			{
// 				WCHAR cmdLine[1024 * 32] = { 0 };
// 				wcscpy(cmdLine, _commandLine.c_str());
// 
// 				success = (ERROR_SUCCESS == user_manager::createProcessForUser(hUserSessionToken, _exeFilePath.c_str(), cmdLine, logfile("")));
// 			}
// 
// 			// success
// 			return;
// 		}

	}
}
