//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//
// This file contains set of helper functions for managing users and groups.
//

#include "windowsUsers.h"

#include <Lm.h>
#include <string>
#include <array>
#include <Userenv.h>
#include "../../internal/helpers.h"
#include "../../../depends/wtsapi/wtspfn.h"

#include <Sddl.h>


#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Userenv.lib")


namespace sys
{
	namespace users
	{
		std::wstring getReason(NET_API_STATUS nStatus)
		{
			std::wstring reason(L"" + std::to_wstring(nStatus));
			switch (nStatus)
			{
				case NERR_PasswordTooLong:  reason = L"Пароль слишком длинный"; break;
				case NERR_PasswordTooShort:  reason = L"Пароль слишком короткий"; break;
				case NERR_PasswordNotComplexEnough:  reason = L"Пароль недостаточно сложный"; break;
				case NERR_PasswordFilterError:  reason = L"Пароль не удовлетворяет палитикам фильтра"; break;
				case NERR_PasswordHistConflict:  reason = L"Пароль не может быть использован сейчас"; break;
				case NERR_InvalidComputer:  reason = L"Не корректное наименование компьютера"; break;
				case NERR_NotPrimary:  reason = L"Операция может быть выполненна только на основном контроллере домена (PDC)"; break;
				case NERR_GroupExists:  reason = L"Группа уже существует"; break;
				case NERR_UserExists:  reason = L"Пользователь уже существует"; break;
				case NERR_GroupNotFound:  reason = L"The local group does not exist"; break;
				case ERROR_ACCESS_DENIED:  reason = L"The user does not have access to the requested information"; break;
				case ERROR_NO_SUCH_MEMBER:  reason = L"Specified member do not exist"; break;
				case ERROR_INVALID_MEMBER:  reason = L"Member cannot be added because their account type is invalid"; break;
			}

			return reason;
		}

		bool addLocalGroup(LPWSTR _name, LPWSTR _comment, LPCWSTR _servername /* = nullptr */)
		{
			LOCALGROUP_INFO_1 info;
			info.lgrpi1_name = _name;
			info.lgrpi1_comment = _comment;

			NET_API_STATUS status = NetLocalGroupAdd(_servername, 1, (LPBYTE)&info, NULL);

			return status == NERR_Success;
		}

		bool addUser(std::wstring& _error,
			logfile& _log,
			LPWSTR _name,
			LPWSTR _fullName,
			LPWSTR _password,
			bool _giveAdminRights)
		{
			bool success = false;
			USER_INFO_1 ui = { 0 };
			DWORD dwError = 0;

			ui.usri1_name = _name;
			ui.usri1_password = _password;

			//ui.usri1_priv = usrType; если верить stackoverflow, то единственное допустимое значение этого поля - USER_PRIV_USER
			// https://stackoverflow.com/questions/34475203/netuseradd-permission-issue-incorrect-paramater

			ui.usri1_priv = USER_PRIV_USER;
			ui.usri1_home_dir = NULL;
			ui.usri1_comment = NULL;
			ui.usri1_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
			ui.usri1_script_path = NULL;

			NET_API_STATUS nStatus = NetUserAdd(NULL, 1, (LPBYTE)&ui, &dwError);
			if ((nStatus == NERR_Success) || (NERR_UserExists == nStatus))
			{
				success = setUserFullName(_name, _fullName);

				// Добавить пользователя в группу администраторов.
				if (_giveAdminRights)
				{
					success = giveAdminRightsToLocalUser(_log, _name);
					if (!success)
					{
						_log.print(std::string(__FUNCTION__) + " giveAdminRightsToLocalUser fail");
						_error = L"addUserToLocalGroup fail (see error log for detail)";
					}
				}
			}
			else
			{
				_log.print(std::string(__FUNCTION__) + " _name: " + strings::ws_s(_name) + "; _fullName: " + strings::ws_s(_fullName) + "; _password: " + strings::ws_s(_password) + "; nStatus: " + std::to_string(nStatus) + "; dwError: " + std::to_string(dwError));
				_error = L"Ошибка доменной политики: " + getReason(nStatus) + L"; dwError: " + std::to_wstring(dwError);
			}
			return success;
		}

		bool giveAdminRightsToLocalUser(logfile& _log, std::wstring _username)
		{
			// Если хоть в одну из групп добавить удастся - значит Ок.
			//return addUserToLocalGroup(_log, const_cast<LPWSTR>(_username.c_str()), constants::ADMINISTRATORS_RU.c_str()) ||
			//	addUserToLocalGroup(_log, const_cast<LPWSTR>(_username.c_str()), constants::ADMINISTRATORS_EN.c_str());
			return false;
		}

		void removeAdminRightsOfLocalUser(std::wstring _username)
		{
			//deleteUserFromLocalGroup(const_cast<LPWSTR>(_username.c_str()), constants::ADMINISTRATORS_RU.c_str()); // RU
			//deleteUserFromLocalGroup(const_cast<LPWSTR>(_username.c_str()), constants::ADMINISTRATORS_EN.c_str()); // EN
		}

		bool deleteUserFromLocalGroup(LPWSTR _domainAndName, LPCWSTR _groupname, LPCWSTR _servername /* = nullptr */)
		{
			LOCALGROUP_MEMBERS_INFO_3 userInfo;
			userInfo.lgrmi3_domainandname = _domainAndName;
			NET_API_STATUS status = NetLocalGroupDelMembers(_servername, _groupname, 3, (PBYTE)&userInfo, 1);
			return (status == NERR_Success) || (status == ERROR_MEMBER_NOT_IN_ALIAS);
		}

		bool addUserToLocalGroup(logfile& _log, LPWSTR _domainAndName, LPCWSTR _groupname, LPCWSTR _servername /* = nullptr */)
		{
			LOCALGROUP_MEMBERS_INFO_3 userInfo;
			userInfo.lgrmi3_domainandname = _domainAndName;
			NET_API_STATUS status = NetLocalGroupAddMembers(_servername, _groupname, 3, (PBYTE)&userInfo, 1);

			if (status == NERR_Success || status == ERROR_MEMBER_IN_ALIAS)
			{
				return true;
			}
			else
			{
				_log.print(std::string(__FUNCTION__) + " _domainAndName: " + strings::ws_s(_domainAndName) + "; _groupname: " + strings::ws_s(_groupname) + "; status: " + std::to_string(status));
				return false;
			}
		}

		bool setUserPassword(LPWSTR _password, LPCWSTR _username, LPCWSTR _servername)
		{
			USER_INFO_1003 user_info;
			user_info.usri1003_password = _password;

			NET_API_STATUS status = NetUserSetInfo(_servername, _username, 1003, (PBYTE)&user_info, NULL);

			return status == NERR_Success;
		}

		bool setUserFullName(LPCWSTR _user, LPWSTR _fullName, LPCWSTR _servername /* = nullptr */)
		{
			USER_INFO_1011 info;
			info.usri1011_full_name = _fullName;

			NET_API_STATUS status = NetUserSetInfo(_servername, _user, 1011, (PBYTE)&info, NULL);

			return status == NERR_Success;
		}

		bool setUserPrivilege(LPCWSTR _user, DWORD _privelege, LPCWSTR _servername /* = nullptr */)
		{
			USER_INFO_1005 info;
			info.usri1005_priv = _privelege;

			NET_API_STATUS status = NetUserSetInfo(_servername, _user, 1005, (PBYTE)&info, NULL);

			return status == NERR_Success;
		}

		bool deleteUserAccount(LPCWSTR _username, LPCWSTR _servername /* = nullptr */)
		{
			NET_API_STATUS net_status = NetUserDel(_servername, _username);

			return net_status == NERR_Success;
		}

		std::wstring getUserFullName(LPCWSTR _user, LPCWSTR _servername /* = nullptr */)
		{
			std::wstring fullname;
			PUSER_INFO_4 pinfo = NULL;

			NET_API_STATUS status = NetUserGetInfo(_servername, _user, 4, (LPBYTE*)&pinfo);

			if (status == NERR_Success)
			{
				fullname = pinfo->usri4_full_name;
				NetApiBufferFree(pinfo);
			}

			return fullname;
		}

		void getGroupsUserBelongsTo(LOCAL_GROUPS& _list, LPCWSTR _userName, LPCWSTR _servername /* = nullptr */)
		{
			ULONG  lTotal = 0, lReturned = 0, lIndex = 0;
			NET_API_STATUS netStatus;
			LOCALGROUP_USERS_INFO_0* info;
			LOCAL_GROUPS result;

			do
			{
				netStatus = NetUserGetLocalGroups(_servername, _userName, 0, LG_INCLUDE_INDIRECT, (PBYTE*)&info, MAX_PREFERRED_LENGTH, &lReturned, &lTotal);

				if ((netStatus == ERROR_MORE_DATA) || (netStatus == NERR_Success))
				{
					LOCAL_GROUP lg;
					ZeroMemory(&lg, sizeof lg);

					for (lIndex = 0; lIndex < lReturned; lIndex++)
					{
						lstrcpyW(lg.name, info[lIndex].lgrui0_name);
						result.push_back(lg);
					}

					NetApiBufferFree(info);
				}
			} while (netStatus == ERROR_MORE_DATA);

			if (result.size() != 0)
			{
				_list.swap(result);
			}
		}

		void getLocalUsers(LOCAL_USERS& _list, LPCWSTR _servername /* = nullptr */)
		{
			DWORD lResume = 0;
			ULONG  lTotal = 0, lReturned = 0, lIndex = 0;
			NET_API_STATUS netStatus;
			USER_INFO_1* info;

			LOCAL_USERS users;

			do {
				netStatus = NetUserEnum(_servername, 1, NULL, (PBYTE*)&info, MAX_PREFERRED_LENGTH, &lReturned, &lTotal, &lResume);

				if ((netStatus == ERROR_MORE_DATA) || (netStatus == NERR_Success))
				{
					LOCAL_USER_ACCOUNT lu;
					ZeroMemory(&lu, sizeof lu);

					for (lIndex = 0; lIndex < lReturned; lIndex++)
					{
						lstrcpyW(lu.name, info[lIndex].usri1_name);
						lstrcpyW(lu.comment, info[lIndex].usri1_comment);
						lu.flags = info[lIndex].usri1_flags;

						users.push_back(lu);
					}

					NetApiBufferFree(info);
				}
			} while (netStatus == ERROR_MORE_DATA);

			if (users.size() != 0)
			{
				_list.swap(users);
			}
		}

		bool doesUserExist(std::wstring _username)
		{
			LOCAL_USERS users;
			getLocalUsers(users);

			for (auto user : users)
			{
				if (strings::equalStrings(_username, user.name))
				{
					return true;
				}
			}

			return false;
		}

		void getLocalGroups(LOCAL_GROUPS& _list, LPCWSTR _servername /*= NULL*/)
		{
			ULONG_PTR lResume = 0;
			ULONG  lTotal = 0;
			ULONG  lReturned = 0;
			ULONG  lIndex = 0;
			NET_API_STATUS netStatus;
			LOCALGROUP_INFO_1* pinfoGroup;

			LOCAL_GROUPS groups;

			do {
				netStatus = NetLocalGroupEnum(_servername, 1, (PBYTE*)&pinfoGroup, MAX_PREFERRED_LENGTH, &lReturned, &lTotal, &lResume);

				if ((netStatus == ERROR_MORE_DATA) || (netStatus == NERR_Success))
				{
					LOCAL_GROUP lg;
					ZeroMemory(&lg, sizeof lg);

					for (lIndex = 0; lIndex < lReturned; lIndex++)
					{
						lstrcpyW(lg.name, pinfoGroup[lIndex].lgrpi1_name);
						lstrcpyW(lg.comment, pinfoGroup[lIndex].lgrpi1_comment);

						groups.push_back(lg);
					}

					NetApiBufferFree(pinfoGroup);
				}

			} while (netStatus == ERROR_MORE_DATA);

			if (groups.size() != 0)
			{
				_list.swap(groups);
			}
		}

		std::wstring getNameOfActiveUser()
		{
			std::wstring username;
			LPWSTR lpUser = NULL;
			DWORD dwUserLength = 0;
			auto bRet = wtsapi::MyWTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, wtsapi::MyWTSGetActiveConsoleSessionId(), WTSUserName, &lpUser, &dwUserLength);
			if (bRet)
			{
				// if (WTSActive == si.State)

				username = std::wstring(lpUser);
				wtsapi::MyWTSFreeMemory(lpUser);
			}

			return username;
		}

		HANDLE getTokenOfActiveUser(int* _error)
		{
			HANDLE hToken = 0;
			auto sessionId = wtsapi::MyWTSGetActiveConsoleSessionId();
			if (sessionId == 0xFFFFFFFF)
			{
				//_log.print(std::string(__FUNCTION__) + " sessionId: \'" + std::to_string(sessionId) + "\'");
				if (_error)
				{
					*_error = GetLastError();
				}
				return 0;
			}
			auto ret = wtsapi::MyWTSQueryUserToken(sessionId, &hToken);
			if (!ret)
			{
				//_log.print(std::string(__FUNCTION__) + " MyWTSQueryUserToken(): \'" + std::to_string(ret) + "\' hToken: \'" + std::to_string(reinterpret_cast<int64_t>(hToken)) + "\'");
				if (_error)
				{
					*_error = GetLastError();
				}
				return 0;
			}

			HANDLE hPrimaryToken = 0;
			if (!DuplicateTokenEx(hToken,
				MAXIMUM_ALLOWED,
				NULL,
				SecurityImpersonation,
				TokenPrimary,
				&hPrimaryToken))
			{
				//_log.print(std::string(__FUNCTION__) + " hPrimaryToken: \'" + std::to_string(reinterpret_cast<int64_t>(hPrimaryToken)) + "\'");
				if (_error)
				{
					*_error = GetLastError();
				}
				hPrimaryToken = 0;
			}

			CloseHandle(hToken);
			return hPrimaryToken;
		}

		int createProcessForUser(HANDLE _hToken,
			const std::wstring& _application,
			LPWSTR _lpCmdLine,
			logfile& _log)
		{
			int error = 0;
			DWORD dwSize = 0;
			LPVOID lpvEnv = 0;
			PROCESS_INFORMATION pi = { 0 };
			STARTUPINFOW si = { 0 };
			WCHAR szUserProfile[256] = L"";

			ZeroMemory(szUserProfile, sizeof szUserProfile);
			ZeroMemory(&si, sizeof(STARTUPINFOW));

			si.cb = sizeof(STARTUPINFOW);
			// 		si.lpDesktop = L"WinSta0\\Default";
			// 		si.dwFlags = STARTF_USESHOWWINDOW;
			// 		si.wShowWindow = SW_SHOW;

			if (!CreateEnvironmentBlock(&lpvEnv, _hToken, FALSE))
			{
				error = GetLastError();
				_log.printLastErrorCode("CreateEnvironmentBlock failed.");
				goto CLEAN_UP;
			}

			dwSize = sizeof(szUserProfile) / sizeof(WCHAR);
			if (!GetUserProfileDirectoryW(_hToken, szUserProfile, &dwSize))
			{
				error = GetLastError();
				_log.printLastErrorCode("GetUserProfileDirectoryW failed.");
				goto CLEAN_UP;
			}

			auto created = CreateProcessAsUserW(_hToken,
				_application.c_str(),
				_lpCmdLine,
				NULL,
				NULL,
				FALSE,
				CREATE_UNICODE_ENVIRONMENT,
				lpvEnv,
				szUserProfile,
				&si,
				&pi);

			error = GetLastError();

			if (created)
			{
				if (pi.hProcess)
				{
					CloseHandle(pi.hProcess);
				}

				if (pi.hThread)
				{
					CloseHandle(pi.hThread);
				}
			}

		CLEAN_UP:
			if (lpvEnv)
			{
				DestroyEnvironmentBlock(lpvEnv);
			}
			return error;
		}

		bool logoutActiveUser()
		{
			// последним параметр bWait [in]
			// FALSE - функция возвращается сразу же (ассинхронно)
			// TRUE  - функция ждет пока сессия будет отлогинена (синхронно)
			return wtsapi::MyWTSLogoffSession(WTS_CURRENT_SERVER_HANDLE, wtsapi::MyWTSGetActiveConsoleSessionId(), FALSE) == TRUE;
		}


		//
		// SID: S-1-5-18
		// Название: Локальная система
		// Описание: Учетная запись службы, используемая операционной системой.
		//
		// SID: S-1-5-19
		// Название: Администратор NT
		// Описание: Локальная служба
		//
		// SID: S-1-5-20
		// Название: Администратор NT
		// Описание: Сетевая служба
		//

		std::vector<std::wstring> getReservedSystemAccountNames()
		{
			std::vector<std::wstring> names;
			std::array<const wchar_t*, 3> built_in_sids = { L"S-1-5-18", L"S-1-5-19", L"S-1-5-20" };

			for (const auto &sid : built_in_sids)
			{
				PSID psid = NULL;
				if (ConvertStringSidToSidW(sid, &psid) != 0)
				{
					DWORD cchAccountName = 512;
					wchar_t accountName[512] = { 0 };
					SID_NAME_USE sidnameuse;
					wchar_t referencedDomainName[512] = { 0 };
					DWORD cchReferencedDomainName = 512;

					BOOL success = LookupAccountSidW(NULL, psid, accountName, &cchAccountName, referencedDomainName, &cchReferencedDomainName, &sidnameuse);
					if (success == TRUE)
					{
						names.push_back(accountName);
					}

					LocalFree(psid);
				}
			}

			return names;
		}

		bool lockUser(logfile& _log, std::wstring& _error, DWORD& flags, const std::wstring _name, LPCWSTR _servername /*= nullptr*/)
		{
			USER_INFO_1008 info_8 = { 0 };
			DWORD dwError = 0;

			info_8.usri1008_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD | UF_ACCOUNTDISABLE;

			NET_API_STATUS status = NetUserSetInfo(_servername, _name.c_str(), 1008, (LPBYTE)&info_8, &dwError);
			if (status != NERR_Success)
			{
				_error = L"NetUserSetInfo() fail; last error: " + std::to_wstring(GetLastError()) + L"; status: " + std::to_wstring(status) + L"; dwError: " + std::to_wstring(dwError);
				return false;
			}

			flags = info_8.usri1008_flags;

			return true;
		}

		bool unLockUser(logfile& _log, std::wstring& _error, DWORD& flags, const std::wstring _name, LPCWSTR _servername /*= nullptr*/)
		{
			USER_INFO_1008 info_8 = { 0 };
			DWORD dwError = 0;

			info_8.usri1008_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;

			NET_API_STATUS status = NetUserSetInfo(_servername, _name.c_str(), 1008, (LPBYTE)&info_8, &dwError);
			if (status != NERR_Success)
			{
				_error = L"NetUserSetInfo() fail; last error: " + std::to_wstring(GetLastError()) + L"; status: " + std::to_wstring(status) + L"; dwError: " + std::to_wstring(dwError);
				return false;
			}

			flags = info_8.usri1008_flags;

			return true;
		}
	}
}
