#include "SziRecycle.h"
#include <Windows.h>
#include <vector>
#include <string>

#define PATH_LOCALRECYCLE	L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"
#define PATH_BITBUCKET		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume"
#define MAX_VOLUMES			255

szi::SziBitBucket::SziBitBucket() 
{
}

szi::SziBitBucket::~SziBitBucket() 
{
}

bool szi::SziBitBucket::SetBySid(const std::wstring& sid, const bool enable)
{
	HKEY	lregBitBucketKey = NULL;
	LONG	lresult = 0;
	std::vector<std::wstring> lvolums;
	const std::wstring lbitBucketpath(sid + L"\\" PATH_BITBUCKET);

	lresult = RegOpenKeyEx(HKEY_USERS, lbitBucketpath.c_str(), NULL, KEY_READ, &lregBitBucketKey);
	if (NO_ERROR == lresult)
	{
		for(int i = 0; i < MAX_VOLUMES; i++)
		{
			WCHAR lbuffer[255] = L"\0";
			lresult = RegEnumKey(lregBitBucketKey, i, lbuffer, sizeof(lbuffer)/sizeof(WCHAR));
			if (NO_ERROR == lresult)
			{
				lvolums.push_back(std::wstring(lbuffer));				
			} else
				break;			
		}
		RegCloseKey(lregBitBucketKey);

		for(auto it = lvolums.begin(); it != lvolums.end(); ++it)
		{
			HKEY lregVolumeKey = NULL;
			const std::wstring lvolumePath(sid + L"\\" PATH_BITBUCKET L"\\" + *it);
			lresult = RegOpenKeyEx(HKEY_USERS, lvolumePath.c_str(), NULL, KEY_WRITE, &lregVolumeKey);
			if(NO_ERROR == lresult) 
			{
				const DWORD lvalue = enable ? 1 : 0;
				lresult = RegSetValueEx(lregVolumeKey, L"NukeOnDelete", NULL, REG_DWORD, (const BYTE*)&lvalue, sizeof(lvalue));
				if(NO_ERROR != lresult) break;

				RegCloseKey(lregVolumeKey);
			}
		}
	}
	return NO_ERROR == lresult ? true : false;
}

bool szi::SziBitBucket::SetLocal(const bool enable) 
{
	HKEY	lregRecycleKey = NULL;
	LONG	lresult = 0;
	
	lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PATH_LOCALRECYCLE, NULL, KEY_WRITE, &lregRecycleKey);
	if(NO_ERROR == lresult) 
	{		
		const DWORD lvalue = enable ? 1 : 0;
		lresult = RegSetValueEx(lregRecycleKey, L"NoRecycleFiles", NULL, REG_DWORD, (const BYTE*)&lvalue, sizeof(lvalue));
		RegCloseKey(lregRecycleKey);				
	}
	return NO_ERROR == lresult ? true : false;
}