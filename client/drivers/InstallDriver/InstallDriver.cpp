// InstallDriver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <Setupapi.h>

//RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 path-to-inf\infname.inf

int _tmain(int argc, _TCHAR* argv[])
{
	WCHAR buffer[MAX_PATH * sizeof(WCHAR)] = TEXT("\0");
	if(argc == 2) 
	{
		wsprintf(buffer, TEXT("DefaultInstall 132 %s"), argv[1]);
		InstallHinfSection(NULL, NULL, buffer, 0);
	} 
	else if(argc == 3 && wcscmp(argv[2], L"Uninstall") == 0)
	{
		wsprintf(buffer, TEXT("DefaultUninstall 132 %s"), argv[1]);
		InstallHinfSection(NULL, NULL, buffer, 0);
	}
	else
	{
		printf("InstallDriver.exe <path to inf file>\n");
		printf("InstallDriver.exe <path to inf file> Uninstall\n");
	}

	return 0;
}

