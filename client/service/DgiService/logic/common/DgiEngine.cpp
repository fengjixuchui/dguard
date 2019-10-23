
//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiEngine.h"

namespace logic
{
	namespace common
	{
		//
		// Static members declared here.
		//

		std::mutex							DgiEngine::g_lock;
		DgiSystems							DgiEngine::m_systems;
		::logic::common::DgiConf*			DgiEngine::g_conf;
		::logic::common::MPassKeeper*		DgiEngine::g_masterPasswordKeeper;
		banking::manager::Wallet*			DgiEngine::g_wallet;
		secure_erase::manager::Shredder*	DgiEngine::g_shredder;
		folderlock::manager::FileGuard*		DgiEngine::g_flock;
		DgiCrypt							DgiEngine::g_cryptor;
		DgiSync*							DgiEngine::g_sync;

		const std::string dbgPassword = "0123456789";
		

		logic::common::DgiConf& DgiEngine::getConf()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_conf == nullptr)
			{
				auto imagedir = windir::getImageDir();

				g_conf = new ::logic::common::DgiConf(imagedir);
			}

			return *g_conf;
		}

		logic::common::MPassKeeper& DgiEngine::getPassword()
		{
			auto& conf = getConf();
			logfile& logCommon = conf.getLog(LOG_COMMON);

			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_masterPasswordKeeper == nullptr)
			{
				g_masterPasswordKeeper = new MPassKeeper(conf.getSettingsKeeper().add(CONF_MPR_HASH), logCommon);
			}

			return *g_masterPasswordKeeper;
		}

		::logic::banking::manager::Wallet& DgiEngine::getWallet()
		{
			auto& conf = getConf();
			auto log = conf.getLogFile(LOG_WALLET);

			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_wallet == nullptr)
			{
				g_wallet = new ::logic::banking::manager::Wallet(log);
				m_systems[SYSTEM_WALLET] = g_wallet;
			}

			return *g_wallet;
		}

		logic::secure_erase::manager::Shredder& DgiEngine::getShredder()
		{
			auto& conf = getConf();
			auto log = conf.getLogFile(LOG_SHREDDER);

			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_shredder == nullptr)
			{
				g_shredder = new ::logic::secure_erase::manager::Shredder(log);
				m_systems[SYSTEM_SHREDDER] = g_shredder;
			}

			return *g_shredder;
		}

		logic::folderlock::manager::FileGuard& DgiEngine::getFlock()
		{
			auto& conf = getConf();
			auto log = conf.getLogFile(LOG_FLOCK);

			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_flock == nullptr)
			{
				g_flock = new ::logic::folderlock::manager::FileGuard(log);
				m_systems[SYSTEM_FLOCK] = g_flock;
			}

			return *g_flock;
		}
		
		void DgiEngine::destroyWallet()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			delete g_wallet;
			g_wallet = nullptr;
		}

		void DgiEngine::destroyShredder()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			delete g_shredder;
			g_shredder = nullptr;
		}

		void DgiEngine::destroyFlock()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			delete g_flock;
			g_flock = nullptr;
		}

		bool DgiEngine::changePassword(std::wstring& _currPassword, std::wstring& _newPassword)
		{
			auto& conf = getConf();
			auto& log = conf.getLog(LOG_COMMON);

			if (_newPassword.empty())
			{
				log.print(std::string(__FUNCTION__) + ": error - can't change the password because new password is not set." );
				return false;
			}

			if (_currPassword.empty())
			{
				log.print(std::string(__FUNCTION__) + ": error - can't change the password because no info about current password.");
				return false;
			}

			MasterPassword newMasterPassword(_newPassword), currentPassword(_currPassword);

			::logic::common::MPassKeeper& dgiMasterPassword = getPassword();

			//
			//	Verify with current master-password.
			//

			if (!dgiMasterPassword.isThePassword(currentPassword))
			{
				log.print(std::string(__FUNCTION__) + ": error - can't change master-password because you passed wrong current password.");
				return false;
			}

			::logic::banking::manager::Wallet& wallet = getWallet();
			logic::folderlock::manager::FileGuard& flock = getFlock();

			//
			//	That lock all requests on access for DgiEngine Core.
			//
			// std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (!wallet.ctrlEventPasswordChanged(newMasterPassword, currentPassword))
			{
				//
				//	It's too bad and I hope we never see that in reality.
				//
				log.print(std::string(__FUNCTION__) + ": critical error - can't re-encrypt wallet's data with new master-password.");
				return false;
			}

			if (!flock.ctrlEventPasswordChanged(newMasterPassword, currentPassword))
			{
				//
				//	Roll back wallet's data.
				//	It's too bad and I hope we never see that in reality.
				//

				wallet.ctrlEventPasswordChanged(currentPassword, currentPassword);

				log.print(std::string(__FUNCTION__) + ": critical error - can't re-encrypt flocks with new master-password.");
				return false;
			}

			//
			//	Change master password in hash storage.
			//

			if (!dgiMasterPassword.setPassword(newMasterPassword))
			{
				log.print(std::string(__FUNCTION__) + ": critical error - can't save new password info in storage.");
				return false;
			}

			return true;
		}

		bool DgiEngine::initMasterPassword(::logic::common::MasterPassword _password)
		{
			auto& conf = getConf();
			auto& log = conf.getLog(LOG_COMMON);

			::logic::banking::manager::Wallet& wallet = getWallet();
			logic::folderlock::manager::FileGuard& flock = getFlock();

			if (!_password.isSet())
			{
				log.print(std::string(__FUNCTION__) + ": error - can't init master password because new password is not set.");
				return false;
			}

			auto& masterPasswordKeeper = getPassword();

			if (!masterPasswordKeeper.isStorageLoaded())
			{
				log.print(std::string(__FUNCTION__) + ": error - mpr storage is not loaded yet.");
			}

			//	Is that right current master-password?
			if (!masterPasswordKeeper.isThePassword(_password))
			{
				log.print(std::string(__FUNCTION__) + ": error - can't authenticate because was passed wrong password. Stop initialization process.");
				return false;
			}

			if(!masterPasswordKeeper.setPassword(_password))
			{
				log.print(std::string(__FUNCTION__) + ": critical error - can't set master-password in keeper.");
				return false;
			}
            
            //
            //  Notify each our internal subsystem about 'changed master password'.
            //
			for (auto system : m_systems)
			{
				if (!system.second->ctrlSetPassword(_password))
				{
					log.print(std::string(__FUNCTION__) + ": error - could not set mpr for (" + system.first + ") system.");
					return false;
				}
			}

			return true;
		}

		bool DgiEngine::shutdown()
		{
			bool res = true;
			auto& conf = getConf();
			auto& log = conf.getLog(LOG_COMMON);

            //
            //  Notify each internal subsystem about 'system shutdown'.
            //  All systems will be notified but if even only one subsystem fails then final result of the function will be false.
            //  All subsystems should handle .ctrlShutdown() event fast and return result immediately.
            //

			for (auto system : m_systems)
			{
				log.print(std::string(__FUNCTION__) + ": shutdown " + system.first + " subsystem.");

				if (!system.second->ctrlShutdown(false))
				{
					res = false;
					log.print(std::string(__FUNCTION__) + ": error - " + system.first + " was stopped.");
				}
			}

			return res;
		}

		DgiCrypt& DgiEngine::getCryptor()
		{
			return g_cryptor;
		}

		logic::common::DgiSync& DgiEngine::getSync()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_sync == nullptr)
			{
				g_sync = new DgiSync();
			}

			return *g_sync;
		}

	}
}
