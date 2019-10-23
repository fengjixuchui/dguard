//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//
// This file contains set of wrapper-helpers functions for managing local users and groups.
//

#pragma once

#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

#include <list>
#include <map>
#include <vector>
#include <Lm.h>
#include <string>
#include "../../internal/log.h"


namespace sys
{
	namespace users
	{
		typedef struct _LOCAL_USER_ACCOUNT
		{
			WCHAR name[20];// User account names are limited to 20 characters and group names are limited to 256 characters.
			WCHAR comment[256]; // MAXCOMMENTSZ are limited to 256 chars.
			DWORD flags; // usri1_flags from USER_INFO_1
		}LOCAL_USER_ACCOUNT, *PLOCAL_USER_ACCOUNT;

		typedef struct _LOCAL_GROUP
		{
			WCHAR name[256]; // Group names are limited to 256 characters.
			WCHAR comment[256]; // MAXCOMMENTSZ are limited to 256 chars.
		} LOCAL_GROUP, *PLOCAL_GROUP;

		typedef std::list<LOCAL_GROUP>			LOCAL_GROUPS;
		typedef std::list<LOCAL_USER_ACCOUNT>	LOCAL_USERS;

		// Provides list of all local groups from '_servername'.
		// If _servername is NULL, using a local computer.
		void getLocalGroups(LOCAL_GROUPS& _list, LPCWSTR _servername = nullptr);

		// Provides list of all local users from '_servername' computer.
		// If _servername is NULL, using a local computer.
		void getLocalUsers(LOCAL_USERS& _list, LPCWSTR _servername = nullptr);

		// Verify existence of the user in OS
		bool doesUserExist(std::wstring _username);

		// Provides a list of local groups to which a specified user belongs.
		void getGroupsUserBelongsTo(LOCAL_GROUPS& _list, LPCWSTR _userName, LPCWSTR _servername = nullptr);

		// Returns full name of _user. Empty string means error.
		std::wstring getUserFullName(LPCWSTR _user, LPCWSTR _servername = nullptr);

		// Removes user account from computer with name '_servername'.
		// Returns true if account was successfully removed.
		bool deleteUserAccount(LPCWSTR _username, LPCWSTR _servername = nullptr);

		// Sets password user account.
		bool setUserPassword(LPWSTR _password, LPCWSTR _username, LPCWSTR _servername = nullptr);

		// Sets full name to the _user account.
		bool setUserFullName(LPCWSTR _user, LPWSTR _fullName, LPCWSTR _servername = nullptr);

		// Set user privilege - Guest\User\Administrator.
		bool setUserPrivilege(LPCWSTR _user, DWORD _privelege, LPCWSTR _servername = nullptr);

		// Добавляет локального пользователя в некоторую локальную группу.
		// '_domainAndName' looks like: <DomainName>\<AccountName>
		bool addUserToLocalGroup(logfile& _log, LPWSTR _domainAndName, LPCWSTR _groupname, LPCWSTR _servername = nullptr);

		// Remove user from a local group.
		// '_domainAndName' looks like: <DomainName>\<AccountName>
		bool deleteUserFromLocalGroup(LPWSTR _domainAndName, LPCWSTR _groupname, LPCWSTR _servername = nullptr);

		// Adds user to a system with password which couldn't be expired.
		// If _serververname is NULL, using local computer.
		// If password is not needed set _password to NULL.
		// If comment for user account is not needed set _comment to NULL.
		// For using default user directory set _homePath to NULL.
		bool addUser(std::wstring& _error,
			logfile& _log,
			LPWSTR _name,
			LPWSTR _fullName,
			LPWSTR _password,
			bool _giveAdminRights);

		// Creates new local group.
		bool addLocalGroup(LPWSTR _name, LPWSTR _comment, LPCWSTR _servername = nullptr);

		// Returns name of current active windows user.
		std::wstring getNameOfActiveUser();

		// Works only if code running in a 'local system' context.
		HANDLE getTokenOfActiveUser(int* _winapiLastErrorCode);

		int createProcessForUser(HANDLE _hToken,
			const std::wstring& _application,
			LPWSTR _lpCmdLine,
			logfile& _log);

		// производит logout текущей сессии пользователя
		bool logoutActiveUser();

		// Наделяет локального пользователя правами администратора.
		// Путем добавления пользователя в группу локальных администраторов.
		bool giveAdminRightsToLocalUser(logfile& _log, std::wstring _username);

		// Удаляет пользователя из админской группы, лишая таким образом полномочий администратора.
		void removeAdminRightsOfLocalUser(std::wstring _username);

		std::vector<std::wstring> getReservedSystemAccountNames();

		bool lockUser(logfile& _log, std::wstring& _error, DWORD& flags, const std::wstring _name, LPCWSTR _servername = nullptr);

		bool unLockUser(logfile& _log, std::wstring& _error, DWORD& flags, const std::wstring _name, LPCWSTR _servername = nullptr);
	}
}
