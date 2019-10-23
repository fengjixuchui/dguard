//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <map>
#include <mutex>
#include <string>
#include "DgiCrypt.h"
#include "../banking/manager/ManagerBanking.h"
#include "../erasing/manager/ManagerSecureErase.h"
#include "../folderlock/manager/DgiFolderLockManager.h"
#include "DgiConf.h"
#include "master-password.h"
#include "DgiSync.h"
#include "DgiCommonControl.h"
#include "DgiCommon.h"

//
//	Log files.
//

#define LOG_FLOCK			L"flock.log"
#define LOG_SHREDDER		L"shredder.log"
#define LOG_WALLET			L"wallet.log"
#define LOG_FCRYPT			L"fcrypt.log"
#define LOG_COMMON			L"dguard.log"
#define LOG_MAIN_SRV		LOG_COMMON


//
//	Configurations and storage files.
//

#define CONF_MPR_HASH		L"dgimpr.conf"


namespace logic
{
	namespace common
	{
		typedef std::map< std::string, DgiCommonControl*> DgiSystems;

		class DgiEngine
		{
		public:

			static ::logic::common::DgiConf& getConf();

			static ::logic::common::MPassKeeper& getPassword();
			static ::logic::common::MPassKeeper& initPassword(MasterPassword _newPassword);

			static ::logic::banking::manager::Wallet& getWallet();
			static void destroyWallet();

			static secure_erase::manager::Shredder& getShredder();
			static void destroyShredder();

			static folderlock::manager::FileGuard& getFlock();
			static void destroyFlock();

			static DgiCrypt& getCryptor();

			static DgiSync& getSync();

			//
			//	After successfully changing password - new and old passwords (which are arguments) will be erased.
			//
			//	0. Locks all operations with using master-password.
			//	1. Decodes all data with old password and encodes with new password.
			//	2. Saves hash of the new password on a disk. 
			//	3. Flushes all encoded with new password data on a disk.
			//	4. Unlocks an access on using master-password.
			//	5. ...
			//

			static bool changePassword(std::wstring& _currPassword, std::wstring& _newPassword);

			//
			//	Method is called when user logins to Data Guard.
			//
			//	It's automatically sends the master password after successfully authenticating to sub-systems.
			//

			static bool initMasterPassword(::logic::common::MasterPassword _password);

			//
			//	Stops all subsystems as soon as possible and returns control.
			//

			static bool shutdown();

		private:
			static DgiSystems	m_systems;
			static std::mutex g_lock;
			static ::logic::common::DgiConf* g_conf;
			static ::logic::common::MPassKeeper* g_masterPasswordKeeper;
			static banking::manager::Wallet* g_wallet;
			static secure_erase::manager::Shredder* g_shredder;
			static folderlock::manager::FileGuard* g_flock;
			static DgiCrypt g_cryptor;
			static DgiSync* g_sync;
		};
		
	}
}
