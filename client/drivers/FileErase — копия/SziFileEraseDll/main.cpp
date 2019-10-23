#include <stdlib.h>
#include <stdio.h>

#include <windows.h>

#include <SziFileEraseDriver.h>
#include <SziFileSearch.h>
#include <SziNamedPipe.h>
#include "SziFileEraseDialog.h"

#define SZI_ERASEFILE_MUTEX		L"Local\\SziFileEraseMenu"
#define SZI_FILEERASE_PIPE		L"\\\\.\\pipe\\SizMultiFileErase"
#define SZI_TIMEOUT_READ		2000
#define SZI_TIMEOUT_CLIENT_READ (SZI_TIMEOUT_READ * 5)

DWORD SziPipeReader(HWND, std::list<std::wstring>&, DWORD &);
DWORD WINAPI ThreadMessageBox(LPVOID lpParameter);
void SziEraseFileDir(LPWSTR objectName);

__declspec( dllexport )
void CALLBACK SziFileEraseW(HWND, HINSTANCE, LPWSTR lpszCmdLine, int)
{
	SziEraseFileDir(lpszCmdLine);	
}

__declspec( dllexport )
void CALLBACK SziFileErase(HWND, HINSTANCE, LPSTR lpszCmdLine, int)
{
	TCHAR	lObjectName[MAX_WINDOWSPATH] = L"\0";
	size_t	retLength = 0;
	setlocale(LC_ALL, "ru-RU");
	mbstowcs_s(&retLength, lObjectName, lpszCmdLine, strlen(lpszCmdLine));
	SziEraseFileDir(lObjectName);
}

bool SziFileIsDirectory(const std::wstring& objectName)
{
	bool lIsDir = false;
	DWORD lAttr = GetFileAttributes(objectName.c_str());

	if (lAttr != INVALID_FILE_ATTRIBUTES) {
		lIsDir = (lAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	}
	return lIsDir;
}

void SziEraseFileDir(LPWSTR objectName)
{
	TCHAR	lMessage[MAX_WINDOWSPATH] = L"\0";
	HWND	lDesktop = GetDesktopWindow();
	HANDLE	lMutex = CreateMutex(NULL, TRUE, SZI_ERASEFILE_MUTEX);	

	if(INVALID_HANDLE_VALUE != lMutex)
	{
		if(ERROR_ALREADY_EXISTS != GetLastError()) 
		{
			DWORD					lIsDelete = 0;
			std::list<std::wstring> lDirAndFiles = { objectName };
			std::list<std::wstring> lFiles;
			std::list<std::wstring> lDir;
			
			DWORD lResult = SziPipeReader(lDesktop, lDirAndFiles, lIsDelete);
			CloseHandle(lMutex);

			if(lResult)
			{
				swprintf_s(lMessage, DIALOG_WININIT_ERROR, lResult);
				MessageBox(lDesktop, lMessage, DIALOG_TITLE, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
			} else 
			{
				//Если юзер нажал ок
				if(lIsDelete)
				{
					szi::SziFileEraseDialog lFileEraser;
					bool lIsError = false;

					for(auto xObject : lDirAndFiles) 
					{
						if(!SziFileIsDirectory(xObject)) 
						{
							lFiles.push_back(xObject);
						} else 
						{
							lDir.push_back(xObject);
						}
					}

					if(lFileEraser.Init()) 
					{
						if(!lFileEraser.EraseFiles(lFiles))
						{
							lIsError = true;
						} else
						{
							if(!lFileEraser.EraseDir(lDir))
							{
								lIsError = true;
							}
						}
						
						lFileEraser.Destroy();
					} else
						lIsError = true;

					if (lIsError)
					{
						const std::wstring lError = lFileEraser.GetLastError();
						MessageBox(lDesktop, lError.c_str(), DIALOG_TITLE, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
					}
				}
			}
		} else
		{
			CloseHandle(lMutex);
			szi::SziNamedPipe pipe;
			if(pipe.Open(SZI_FILEERASE_PIPE, SZI_TIMEOUT_CLIENT_READ))
			{				
				pipe.Write(objectName);
				pipe.Close();
			}
		}
	} else
	{
		swprintf_s(lMessage, DIALOG_WININIT_ERROR, ::GetLastError());
		MessageBox(lDesktop, lMessage, DIALOG_TITLE, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	}
}

DWORD SziPipeReader(HWND desktop, std::list<std::wstring>& files, DWORD& exitCode)
{	
	szi::SziNamedPipe pipe;
	DWORD	lResult = 0;
	HANDLE	lThread = CreateThread(NULL, 0, ThreadMessageBox, desktop, 0, NULL);
	
	if(INVALID_HANDLE_VALUE == lThread) 
	{
		return ::GetLastError();
	}

	if(pipe.Create(SZI_FILEERASE_PIPE)) 
	{
		for(;;)
		{
			std::wstring lMessage;
			if(pipe.Read(lMessage, SZI_TIMEOUT_READ)) 
			{
				if(!lMessage.empty())
					files.push_back(lMessage);
			} else
				break;
		}
	} else 
	{
		lResult = pipe.GetCodeLastError();
	}

	WaitForSingleObject(lThread, INFINITE);
	GetExitCodeThread(lThread, &exitCode);
	CloseHandle(lThread);
	return lResult;
}

bool MessageBoxQuestion(HWND hDesktop) 
{
	return MessageBox(hDesktop, DIALOG_QUESTION, DIALOG_TITLE, MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL) == IDYES;
}

DWORD WINAPI ThreadMessageBox(LPVOID lpParameter)
{
	HWND hDesktop = (HWND)lpParameter;
	return MessageBoxQuestion(hDesktop);
}