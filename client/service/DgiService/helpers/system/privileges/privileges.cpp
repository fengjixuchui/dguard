//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//



#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

#include "privileges.h"

namespace sys
{
	namespace privileges
	{
		struct
		{
			char* szPrivName;
		}

		privilages[] = {
			"SeCreateTokenPrivilege", "SeAssignPrimaryTokenPrivilege", "SeLockMemoryPrivilege", "SeIncreaseQuotaPrivilege",
			"SeUnsolicitedInputPrivilege", "SeMachineAccountPrivilege", "SeTcbPrivilege",
			"SeSecurityPrivilege", "SeTakeOwnershipPrivilege", "SeLoadDriverPrivilege", "SeSystemProfilePrivilege",
			"SeSystemtimePrivilege", "SeProfileSingleProcessPrivilege", "SeIncreaseBasePriorityPrivilege",
			"SeCreatePagefilePrivilege", "SeCreatePermanentPrivilege", "SeBackupPrivilege",
			"SeRestorePrivilege", "SeShutdownPrivilege", "SeDebugPrivilege", "SeAuditPrivilege",
			"SeSystemEnvironmentPrivilege", "SeChangeNotifyPrivilege", "SeRemoteShutdownPrivilege", "SeUndockPrivilege",
			"SeSyncAgentPrivilege", "SeEnableDelegationPrivilege", "SeManageVolumePrivilege"
		};

		bool getPrivilege(const std::string& privName)
		{
			HANDLE hToken;
			LUID lpLuid;
			TOKEN_PRIVILEGES NewState;

			if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			{
				if (LookupPrivilegeValueA(NULL, privName.c_str(), &lpLuid))
				{
					NewState.PrivilegeCount = 1;
					NewState.Privileges[0].Luid = lpLuid;
					NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
					AdjustTokenPrivileges(hToken, FALSE, &NewState, sizeof(NewState), NULL, NULL);
					return true;
				}

				CloseHandle(hToken);
			}

			return false;
		}

		void getMaxPrivileges(void)
		{
			int count = ((int)(sizeof privilages / sizeof privilages[0]));

			for (int i = 0; i < count; i++)
			{
				getPrivilege(privilages[i].szPrivName);
			}
		}
	}
}
