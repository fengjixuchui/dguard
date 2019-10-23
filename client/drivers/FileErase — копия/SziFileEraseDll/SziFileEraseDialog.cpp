#include "SziFileEraseDialog.h"

#include <SziFileEraseDriver.h>
#include <SziFileSearch.h>
#include <SziNamedPipe.h>

szi::SziFileEraseDialog::SziFileEraseDialog()
	:dialog_(NULL)
{
}


szi::SziFileEraseDialog::~SziFileEraseDialog() 
{	
	
}

bool szi::SziFileEraseDialog::Init() 
{
	HWND	hDesktop = GetDesktopWindow();
	HRESULT lHresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool	lResult = false;
	TCHAR	lMessage[MAX_WINDOWSPATH] = L"\0";

	if(SUCCEEDED(lHresult))
	{
		lHresult = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&dialog_));
		lResult = SUCCEEDED(lHresult);
		if (lResult)
		{
			dialog_->SetTitle(DIALOG_TITLE);
			dialog_->SetLine(0, L"", TRUE, NULL);
			dialog_->StartProgressDialog(hDesktop, NULL, PROGDLG_MODAL | PROGDLG_NOCANCEL | PROGDLG_MARQUEEPROGRESS, NULL);
		} else
		{
			swprintf_s(lMessage, DIALOG_WININIT_ERROR, ::GetLastError());
			error_ = lMessage;
		}
	} else
	{
		swprintf_s(lMessage, DIALOG_WININIT_ERROR, ::GetLastError());
		error_ = lMessage;
	}
	return lResult;
}

void szi::SziFileEraseDialog::Destroy() 
{
	if(dialog_ != NULL) 
	{
		dialog_->StopProgressDialog();
		dialog_->Release();
		dialog_ = NULL;
	}
	CoUninitialize();
}

bool szi::SziFileEraseDialog::EraseFiles(const szi::SziFileEraseDialog::ListFiles& files)
{
	szi::drv::SziFileErase	lErase;
	TCHAR					lMessage[MAX_WINDOWSPATH] = L"\0";

	error_.clear();
	if(lErase.Open()) 
	{
		for(auto it = files.begin(); it != files.end(); ++it)
		{
			dialog_->SetLine(0, it->c_str(), TRUE, NULL);
			
			if (IsAccessWrite(*it))
			{
				if(!lErase.EraseFile(*it)) {
					swprintf_s(lMessage, DIALOG_ERROR, ::GetLastError());
					error_ = lMessage;
					break;
				}

				HANDLE lEvent = lErase.GetEventComplete(*it);
				WaitForSingleObject(lEvent, INFINITE);
				const long status = lErase.GetNStatusComplete();
				if(status) {
					swprintf_s(lMessage, DIALOG_ERROR, status);
					error_ = lMessage;
					break;
				}
			} else 
			{
				break;
			}
		}
	} else
	{
		error_ = DIALOG_INIT_ERROR;
	}

	return error_.empty();
}

bool szi::SziFileEraseDialog::EraseDir(const szi::SziFileEraseDialog::ListFiles& files)
{
	szi::drv::SziFileErase	lErase;
	TCHAR					lMessage[MAX_WINDOWSPATH] = L"\0";

	error_.clear();
	if(lErase.Open())
	{
		for(auto itDir = files.begin(); itDir != files.end(); ++itDir)
		{
			szi::FileSearch::ListFiles filesSearch;
			szi::FileSearch().Search(*itDir, L"", filesSearch);

			for(auto itFile = filesSearch.begin(); itFile != filesSearch.end(); ++itFile) 
			{
				const std::wstring lFileName(*itDir + std::wstring(L"\\") + *itFile);
				dialog_->SetLine(0, itFile->c_str(), FALSE, NULL);

				if (IsAccessWrite(lFileName))
				{
					if(!lErase.EraseFile(lFileName)) {
						swprintf_s(lMessage, DIALOG_ERROR, ::GetLastError());
						error_ = lMessage;
						break;
					}

					HANDLE lEvent = lErase.GetEventComplete(lFileName);
					WaitForSingleObject(lEvent, INFINITE);
					const long status = lErase.GetNStatusComplete();
					if(status) {
						swprintf_s(lMessage, DIALOG_ERROR, status);
						error_ = lMessage;
						break;
					}
				} else {
					break;
				}
			}

			if(error_.empty()) 
			{
				if(IsAccessWrite(*itDir))
				{
					if (lErase.EraseFile(*itDir))
					{
						HANDLE lEvent = lErase.GetEventComplete(*itDir);
						WaitForSingleObject(lEvent, INFINITE);
						const long status = lErase.GetNStatusComplete();
						if(status) {
							swprintf_s(lMessage, DIALOG_ERROR, status);
							error_ = lMessage;
						}
					} else {
						swprintf_s(lMessage, DIALOG_ERROR, ::GetLastError());
						error_ = lMessage;
					}
				}
			}

			if (!error_.empty()) break;
		}
	} else 
	{
		error_ = DIALOG_INIT_ERROR;
	}
	return error_.empty();
}

std::wstring szi::SziFileEraseDialog::GetLastError() const
{
	return error_;
}

bool szi::SziFileEraseDialog::IsAccessWrite(const std::wstring& FileName) const
{
	bool	lResult = false;
	HANDLE	lFile = NULL;
	TCHAR	lMessage[MAX_WINDOWSPATH] = L"\0";
	DWORD	lFileAttr = 0;
	DWORD	lFlags = 0;
	
	lFileAttr = GetFileAttributes(FileName.c_str());
	if(lFileAttr != INVALID_FILE_ATTRIBUTES) 
	{
		if((lFileAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) 
		{
			lFlags = FILE_FLAG_BACKUP_SEMANTICS;
		} else 
		{
			lFlags = FILE_ATTRIBUTE_NORMAL;
		}
		
		lFile = CreateFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, lFlags, NULL);
		if(lFile != INVALID_HANDLE_VALUE) 
		{
			lResult = true;
			CloseHandle(lFile);
		} else 
		{
			DWORD lLastError = ::GetLastError();
			if (ERROR_SHARING_VIOLATION == lLastError)
			{
				swprintf_s(lMessage, DIALOG_ERROR_20, FileName.c_str());
			} else 
			{
				swprintf_s(lMessage, DIALOG_WININIT_ERROR, lLastError);
			}
			error_ = lMessage;
		}
	}
	else {
		swprintf_s(lMessage, DIALOG_WININIT_ERROR, ::GetLastError());
		error_ = lMessage;
	}

	return lResult;
}