
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <tchar.h>
#include <Windows.h>

#include <string>

#include <SziFileEraseDriver.h>
#include <SziFileSearch.h>
#include <SziRecycle.h>
#include <SziNamedPipe.h>
#include <CommandLine.h>


void PrintHelp() {
	printf("UtilFileErase.exe [-debug|-ae|-s|-c|-f]\n");
	printf("\t-debug - Wait to debugger\n");
	printf("\t-ae <Enable|Disable> - Enable/Disable auto erase file\n");
	printf("\t-s - Get Status auto erase file\n");
	printf("\t-m <Number> - Set sequence mask\n");
	printf("\t-c <Count>- Set count circle erase\n");
	printf("\t-f <FileName>- Full path to any file\n");
}

int _tmain(int argc, TCHAR *argv[]) 
{
	setlocale(LC_ALL, "RU");
	szi::CommandLine	commandLine;
	commandLine.Parse(argc, argv);

	if(commandLine.IsCommand(L"-debug")) {
		while(!::IsDebuggerPresent())
			::Sleep(100);
	}

	szi::drv::SziFileErase	erase;
	if(!erase.Open()) {
		wprintf(L"Failed: %s\n", erase.GetLastError().c_str());
		return 1;
	}
	
	const std::wstring action = commandLine.GetCommand(L"-ae");
	if(!action.empty()) {
		if(L"Enable" == action) {
			erase.EnableAutoErase();
		} else if(L"Disable" == action) {
			erase.DisableAutoErase();
		}
		return 0;
	}
	
	if(commandLine.IsCommand(L"-s")) {
		if(erase.IsAutoErase()) {
			wprintf(L"AutoErase Enable\n");
		} else
			wprintf(L"AutoErase Disable\n");
		return 0;
	}
	
	
	if(commandLine.IsCommand(L"-m")) {
		const unsigned char setMask = static_cast<const unsigned char>(commandLine.GetCommandAsInt(L"-m"));
		if(!erase.SetMask(setMask)) {
			wprintf(L"SetMask Failed: %s\n", erase.GetLastError().c_str());
		}
		return 0;
	}
	
	const std::wstring setCount = commandLine.GetCommand(L"-c");
	if(!setCount.empty()) {
		if(!erase.SetCountCircle(std::wcstol(setCount.c_str(), nullptr, 10))) {
			wprintf(L"SetCountCircle Failed: %s\n", erase.GetLastError().c_str());
		}
		return 0;
	}
	
	const std::wstring fileName = commandLine.GetCommand(L"-f");
	if(!fileName.empty()) {
		if(!erase.EraseFile(fileName)) {
			wprintf(L"EraseFile Failed: %s\n", erase.GetLastError().c_str());
		} else {
			HANDLE event = erase.GetEventComplete(fileName);
			if(WAIT_OBJECT_0 == WaitForSingleObject(event, INFINITE)) {
				const long lStatus = erase.GetNStatusComplete();
				if(lStatus == 0) {
					wprintf(L"Erase signaled\n");
				} else {
					std::wstring lMessage = erase.GetStatusComplete(lStatus);
					wprintf(L"Erase signaled (0x%X) %s\n", lStatus, lMessage.c_str());
				}
			}
		}
		return 0;
	}
	PrintHelp();
	return 0;

}