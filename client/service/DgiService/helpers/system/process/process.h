//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <vector>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

namespace sys
{
	namespace process
	{
		enum State
		{
			PsKilled,
			PsNotFound,
			PsNoAccess
		};

		struct ModuleInformation
		{
			unsigned long pid;
			void* pBase;
			unsigned long memorySize;
			std::wstring exeName;
			std::wstring exePath;
		};

		typedef std::vector<ModuleInformation> ListModules;

		struct ProcessInformation
		{
			unsigned long pid;
			unsigned long parentPid;
			std::wstring exeName;
			ListModules modules;
		};
		
		typedef std::vector<ProcessInformation> ListProcesses;

		class IMyProcess
		{
		public:
			virtual bool isItMine(const ProcessInformation&) = 0;
		};

		ListModules getModules(unsigned long _pid);

		ListProcesses getProcesses();

		State kill(std::wstring _processName);
		State kill(unsigned long _pid);

		bool killAndWait(DWORD _parentPid, DWORD _waitTime = 5 * 1000 );

		//
		// Returns true when finds information about process.
		//
		bool lookupProcess(IMyProcess& _search, std::vector<ProcessInformation> & _outInfo);

		// Returns true after creating child process.
		bool createChild(std::wstring _exeFilePath, std::wstring _commandLine, unsigned long& _outLastError, PROCESS_INFORMATION& _outPi);

		//
		// Start an application under session of the first active interactive user session.
		//
		// Works only for system services which run under 'localsystem' account.
		void runInteractive(std::wstring _exeFilePath, std::wstring _commandLine);
	}
}


