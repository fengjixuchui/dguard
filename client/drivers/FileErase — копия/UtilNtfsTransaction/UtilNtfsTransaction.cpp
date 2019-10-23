// UtilNtfsTransaction.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <cstdlib>
#include <tchar.h>
#include <ctime>
#include <windows.h>
#include <KtmW32.h>
#include <CommandLine.h>
#include <SziDecodeError.h>
#include <vector>

void PrintHelp() {
	printf("UtilNtfsTransaction.exe [-debug|-f|-ac|-t|-ad|-aw|-aws|-commit|-rollback]\n");
	printf("\t-debug - Wait to debugger\n");
	printf("\t-f <FileName> - File name\n");
	printf("\t-fn <Number> - Posfix file name\n");
	printf("\t-ac - Create file\n");
	printf("\t-t - Truncate file\n");
	printf("\t-ad - Delete file\n");	
	printf("\t-aw <Size> - Write to file\n");
	printf("\t-aws <String> - Write to file\n");
	printf("\t-commit - Commit all actions\n");
	printf("\t-rollback - Rollback all actions\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "RU");
	szi::CommandLine	commandLine;
	commandLine.Parse(argc, argv);

	if(commandLine.IsCommand(L"-debug")) 
	{
		while(!::IsDebuggerPresent())
			::Sleep(100);
	}
		
	const std::wstring lFileName = commandLine.GetCommand(L"-f");
	const int lFileNumber = commandLine.GetCommandAsInt(L"-fn");
	const bool lActionCreate = commandLine.IsCommand(L"-ac");
	const bool lActionDelete = commandLine.IsCommand(L"-ad");
	const bool lFlagTrun = commandLine.IsCommand(L"-t");
	const int lSizeWrite = commandLine.GetCommandAsInt(L"-aw");
	const std::wstring lWriteString = commandLine.GetCommand(L"-aws");

	if ((!lFileName.empty()) && (lActionCreate || lActionDelete || lSizeWrite || !lWriteString.empty()))
	{
		HANDLE lTran = CreateTransaction(NULL, 0, TRANSACTION_DO_NOT_PROMOTE, 0, 0, INFINITE, NULL);
		if(INVALID_HANDLE_VALUE != lTran) 
		{
			printf("Transaction created\n");
			std::vector<HANDLE> lFiles;
			if (lActionCreate || lSizeWrite || !lWriteString.empty())
			{
				DWORD lDisp = OPEN_ALWAYS;
				if (lFlagTrun) lDisp |= TRUNCATE_EXISTING;
				
				if (lFileNumber)
				{
					for (int i = 0; i< lFileNumber; ++i)
					{
						const std::wstring lFileNameNumber = lFileName + std::to_wstring(i);
						HANDLE lFile = CreateFileTransacted(lFileNameNumber.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
																					NULL, lDisp, FILE_ATTRIBUTE_NORMAL, NULL, lTran, 0, NULL);
						if(lFile == INVALID_HANDLE_VALUE) 
						{
							const std::wstring lError = szi::SziDecodeError::DecodeError(::GetLastError(), L"");
							printf("File create error:%S\n", lError.c_str());
							break;
						} else
							lFiles.push_back(lFile);
					}
				} else
				{
					HANDLE lFile = CreateFileTransacted(lFileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
														NULL, lDisp, FILE_ATTRIBUTE_NORMAL, NULL, lTran, 0, NULL);
					if(lFile == INVALID_HANDLE_VALUE) 
					{
						const std::wstring lError = szi::SziDecodeError::DecodeError(::GetLastError(), L"");
						printf("File create error:%S\n", lError.c_str());
					} else
						lFiles.push_back(lFile);
				}				
			}

			if (lSizeWrite && !lFiles.empty())
			{
				DWORD	lWrited = 0;
				UCHAR*	lBuffer = (UCHAR*)malloc(lSizeWrite);
				
				std::srand((int)std::time(0));

				for(int i = 0; i < lSizeWrite; ++i) 
				{
					lBuffer[i] = (UCHAR)(std::rand() - std::rand());
				}

				for(auto itFileHandle : lFiles) 
				{
					if(!WriteFile(itFileHandle, lBuffer, lSizeWrite, &lWrited, NULL))
					{
						const std::wstring lError = szi::SziDecodeError::DecodeError(::GetLastError(), L"");
						printf("Write file error:%S\n", lError.c_str());
						break;
					}
				}
				free(lBuffer);
			}
			
			if(!lWriteString.empty() && !lFiles.empty()) 
			{
				DWORD	lWrited = 0;
				for(auto itFileHandle : lFiles) 
				{
					DWORD lStringLength = lWriteString.length() * sizeof(wchar_t);
					if(!WriteFile(itFileHandle, lWriteString.c_str(), lStringLength, &lWrited, NULL)) 
					{
						const std::wstring lError = szi::SziDecodeError::DecodeError(::GetLastError(), L"");
						printf("Write file error:%S\n", lError.c_str());
						break;
					}
				}
			}			

			for(auto itFileHandle : lFiles) {
				CloseHandle(itFileHandle);
			}
			lFiles.clear();

			if(lActionDelete) 
			{
				for(int i = 0; i < lFileNumber; ++i) 
				{
					const std::wstring lFileNameNumber = lFileName + std::to_wstring(i);
					if(DeleteFileTransacted(lFileNameNumber.c_str(), lTran)) 
					{
						printf("File %S delete success\n", lFileNameNumber.c_str());
					} else
					{
						const std::wstring lError = szi::SziDecodeError().DecodeError(::GetLastError(), L"");
						printf("Delete file '%S' transaction error:%S\n", lFileNameNumber.c_str(), lError.c_str());
					}
				}
				if (!lFileNumber)
				{					
					if(DeleteFileTransacted(lFileName.c_str(), lTran)) 
					{
						printf("File %S delete success\n", lFileName.c_str());
					} else 
					{
						const std::wstring lError = szi::SziDecodeError().DecodeError(::GetLastError(), L"");
						printf("Delete file '%S' transaction error:%S\n", lFileName.c_str(), lError.c_str());
					}
				}
			}

			if(commandLine.IsCommand(L"-commit")) 
			{
				CommitTransaction(lTran);
				printf("Transaction commit\n");
			} else if(commandLine.IsCommand(L"-rollback"))
			{
				RollbackTransaction(lTran);
			}
			
			if(!commandLine.IsCommand(L"-commit"))
				printf("Transaction rollback\n");

			CloseHandle(lTran);
			return 0;
		} else
		{
			const std::wstring lError = szi::SziDecodeError().DecodeError(::GetLastError(), L"");
			printf("Create transaction error:%S\n", lError.c_str());
		}
	}
	
	PrintHelp();
	return 0;
}

