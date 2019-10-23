#include "DgiDecodeError.h"
#include <windows.h>

szi::SziDecodeError::SziDecodeError() 
{
}


szi::SziDecodeError::~SziDecodeError() 
{
}

std::wstring szi::SziDecodeError::DecodeError(unsigned long NTStatusMessage, const std::wstring& module)
{
	LPVOID lpMessageBuffer = NULL;
	HMODULE Hand = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
	if(!module.empty()) 
	{
		Hand = LoadLibrary(module.c_str());
		flags |= FORMAT_MESSAGE_FROM_HMODULE;
	}

	std::wstring errorStr;
	if(!FormatMessage(flags, Hand, NTStatusMessage, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMessageBuffer, 0, NULL)) 
	{
		errorStr = L"Format message failed";
	} else 
	{
		errorStr.assign((PWCHAR)lpMessageBuffer);
		errorStr.append(L"Code:" + std::to_wstring(NTStatusMessage));
		LocalFree(lpMessageBuffer);
	}
	if(Hand != NULL) 
	{
		FreeLibrary(Hand);
	}
	return errorStr;
}

std::wstring szi::SziDecodeError::DecodeLastError()
{
	DWORD lLastError = GetLastError();
	return DecodeError(lLastError, L"");
}
