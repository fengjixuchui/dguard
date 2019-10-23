//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiMainService.h"
#include "../../../logic/session/DgiSession.h"
#include "../../../logic/common/DgiEngine.h"
#include "../type-convertors/TDgiThriftTypesConverter.h"


namespace service
{
	namespace thrift_impl
	{
		DgiMainService::DgiMainService(std::string _s):
			m_logRef( DgiEngine::getConf().getLog(LOG_MAIN_SRV) )
		{
			m_logRef.print(std::string(__FUNCTION__));
		}

		DgiMainService::~DgiMainService()
		{
			m_logRef.print(std::string(__FUNCTION__));
		}

		void DgiMainService::login(::dgi::AuthResponse& _return, const std::string& _masterPassword)
		{
			auto fn = std::string(__FUNCTION__);
			m_logRef.print(fn);

			if (_masterPassword.empty())
			{
				m_logRef.print(fn + ": error - password is empty.");
				_return.result.status = dgi::DgiStatus::NoMasterPassword;
				return;
			}

			::logic::common::MasterPassword mpr( strings::s_ws(_masterPassword, CP_UTF8) );

			auto& keeper = DgiEngine::getPassword();

			if (keeper.isStorageLoaded())
			{
				m_logRef.print(fn + ": ok - password storage was loaded earlier.");

				if (keeper.isThePassword(mpr))
				{
					//m_logRef.print(fn + ": ok - right password.");
					//_return.result.status = dgi::DgiStatus::Success;

					if (DgiEngine::initMasterPassword(mpr))
					{
						m_logRef.print(fn + ": ok - it is right password. Create new session.");
						_return.result.status = dgi::DgiStatus::Success;

						//
						// Create new one.
						//
						_return.sessionId = logic::session::Storage::get().create();
					}
					else
					{
						if (!DgiEngine::getPassword().isStorageLoaded())
						{
							m_logRef.print(fn + ": error - storage was not loaded.");
						}

						m_logRef.print(fn + ": error - failed attempt to login.");
						_return.result.status = dgi::DgiStatus::AccessDenied;
					}

					//
					// Add new session to list of open sessions.
					//
					//_return.sessionId = logic::session::Storage::get().create();
				}
				else
				{
					m_logRef.print(fn + ": error - wrong password.");
					_return.result.status = dgi::DgiStatus::AccessDenied;
				}
			}
			else
			{
				m_logRef.print(fn + ": it is need load storage.");

				if ( DgiEngine::initMasterPassword(mpr) )
				{
					m_logRef.print(fn + ": ok - it is right password. Create new session.");
					_return.result.status = dgi::DgiStatus::Success;

					//
					// Create new one.
					//
					_return.sessionId = logic::session::Storage::get().create();
				}
				else
				{
					if (!DgiEngine::getPassword().isStorageLoaded())
					{
						m_logRef.print(fn + ": error - storage was not loaded.");
					}

					m_logRef.print(fn + ": error - failed attempt to login.");
					_return.result.status = dgi::DgiStatus::AccessDenied;
				}
			}
		}

		void DgiMainService::logout(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid)
		{
			auto fn = std::string(__FUNCTION__);
			m_logRef.print(fn);

			if (logic::session::Storage::get().present(_sid))
			{
				logic::session::Storage::get().close(_sid);

				_return.status = dgi::DgiStatus::Success;

				m_logRef.print(fn + "ok - session was successfully closed.");
			}
			else
			{
				m_logRef.print(fn + ": error - session not found.");

				_return.status = dgi::DgiStatus::NotFound;
			}
		}

		void DgiMainService::changePassword(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid, const std::string& _currentPassword, const std::string& _newPassword)
		{
			m_logRef.print(std::string(__FUNCTION__));

			auto fn = std::string(__FUNCTION__);
			m_logRef.print(fn);

			if (_currentPassword.empty() || _newPassword.empty())
			{
				m_logRef.print(fn + ": error - one of passwords is empty.");
				_return.status = dgi::DgiStatus::NoMasterPassword;
				return;
			}

			m_logRef.print(fn + ": ready to update password.");

			std::wstring current = strings::s_ws(_currentPassword, CP_UTF8);
			std::wstring newPassword = strings::s_ws(_newPassword, CP_UTF8);

			if (DgiEngine::changePassword(current, newPassword))
			{
				m_logRef.print(fn + ": ok - password was updated.");
				_return.status = dgi::DgiStatus::Success;
			}
			else
			{
				m_logRef.print(fn + ": critical error - password was not updated.");
				_return.status = dgi::DgiStatus::UnknownError;
			}

			::logic::common::MasterPassword::secureErase(newPassword);
			::logic::common::MasterPassword::secureErase(current);
		}

		void DgiMainService::isRightPassword(::dgi::DgiResult& _return, const std::string& _masterPassword)
		{
			auto fn = std::string(__FUNCTION__);
			m_logRef.print(fn);

			if (_masterPassword.empty())
			{
				m_logRef.print(fn + ": error - password is empty.");
				_return.status = dgi::DgiStatus::NoMasterPassword;
				return;
			}

			::logic::common::MasterPassword mpr(strings::s_ws(_masterPassword, CP_UTF8));

			auto& keeper = DgiEngine::getPassword();

			if (!keeper.isStorageLoaded())
			{
				m_logRef.print(fn + ": error - can not load storage for verification.");
				_return.status = dgi::DgiStatus::UnknownError;
				return;
			}

			if (keeper.isThePassword(mpr))
			{
				m_logRef.print(fn + ": ok - right password.");
				_return.status = dgi::DgiStatus::Success;
			}
			else
			{
				m_logRef.print(fn + ": error - wrong password.");
				_return.status = dgi::DgiStatus::NoMasterPassword;
			}
		}

		void DgiMainService::isValidSession(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid)
		{
			auto fn = std::string(__FUNCTION__);
			m_logRef.print(fn);

			if (logic::session::Storage::get().present(_sid))
			{
				_return.status = dgi::DgiStatus::Success;
			}
			else
			{
				m_logRef.print(fn + ": error - not found.");

				_return.status = dgi::DgiStatus::NotFound;
			}
		}

		void DgiMainService::isPasswordSet(::dgi::BoolResponse& _return)
		{
			m_logRef.print(std::string(__FUNCTION__));

			auto& keeper = DgiEngine::getPassword();

			if (keeper.hasHash())
			{
				_return.bool_result = true;
				_return.errorResult.status = dgi::DgiStatus::Success;

				m_logRef.print(std::string(__FUNCTION__) + " master password is present.");
			}
			else
			{
				_return.bool_result = false;
				_return.errorResult.status = dgi::DgiStatus::Success;

				m_logRef.print(std::string(__FUNCTION__) + " master password is not set.");
			}
		}

		void DgiMainService::setPassword(::dgi::DgiResult& _return, const std::string& _masterPassword)
		{
			auto fn = std::string(__FUNCTION__);

			m_logRef.print(fn);

			if (_masterPassword.empty())
			{
				m_logRef.print(fn + ": error - password is empty.");
				_return.status = dgi::DgiStatus::NoMasterPassword;
				return;
			}

			auto& keeper = DgiEngine::getPassword();

			if (keeper.hasHash())
			{
				_return.status = dgi::DgiStatus::PresentAlready;

				m_logRef.print(std::string(__FUNCTION__) + " could not set master password. We have hash info about early created password.");
			}
			else
			{
				std::wstring password = strings::s_ws(_masterPassword, CP_UTF8);
				auto res = keeper.setPassword( MasterPassword(password) );

				if (res)
				{
					_return.status = dgi::DgiStatus::Success;

					m_logRef.print(std::string(__FUNCTION__) + " master password was set.");
				}
				else
				{
					_return.status = dgi::DgiStatus::UnknownError;

					m_logRef.print(std::string(__FUNCTION__) + " error - could not set master password, the reason is unknown");
				}
			}
		}

		bool DgiMainService::startInternalServices()
		{
			std::unique_lock<std::mutex> locker(m_lock);

			m_logRef.print(std::string(__FUNCTION__));

			auto& conf = DgiEngine::getConf();
			auto& log = conf.getLog(LOG_COMMON);

			using EncryptionService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiEncryptionService, ::dgi::DgiEncryptionProcessor>;
			m_srvEncryption = std::make_shared<EncryptionService>( THRIFT_ENCRYPTION_PORT, conf.getLog(LOG_FCRYPT).getLogFilePath(), 10 );

			if (!m_srvEncryption->start())
			{
				m_logRef.printEx("%s: error - couldn't start encryption service.", __FUNCTION__);
			}

			using EraseService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiSecureEraseService, ::dgi::DgiSecureEraseProcessor>;
			m_srvErasing = std::make_shared<EraseService>( THRIFT_ERASING_PORT, conf.getLog(LOG_SHREDDER).getLogFilePath(), 10 );

			if (!m_srvErasing->start())
			{
				m_logRef.printEx("%s: error - couldn't start erase service.", __FUNCTION__);
			}

			using FLockService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiFolderlockService, ::dgi::DgiFolderLockProcessor>;
			m_srvFLock = std::make_shared<FLockService>( THRIFT_FLOCK_PORT, conf.getLog(LOG_FLOCK).getLogFilePath(), 10);

			if (!m_srvFLock->start())
			{
				m_logRef.printEx("%s: error - couldn't start flock service.", __FUNCTION__);
			}

			using BankingService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiBankingService, ::dgi::DgiBankingProcessor>;
			m_srvWallet = std::make_shared<BankingService>(THRIFT_BANKING_PORT, conf.getLog(LOG_WALLET).getLogFilePath(), 10);

			if (!m_srvWallet->start())
			{
				m_logRef.printEx("%s: error - couldn't start encryption service.", __FUNCTION__);
			}

			m_logRef.printEx("%s: internal services started.\n\n", __FUNCTION__);

			return true;
		}

		bool DgiMainService::stopInternalServices()
		{
			std::unique_lock<std::mutex> locker(m_lock);

			m_logRef.print(std::string(__FUNCTION__));

			//
			// sync::SysEvents::get().raiseEventShutdownThriftServices();
			//

			m_logRef.printEx("%s: Stop encryption service.", __FUNCTION__);

			if (m_srvEncryption)
			{
				m_srvEncryption->stop();
				m_srvEncryption.reset();
			}

			m_logRef.printEx("%s: Stop erase service.", __FUNCTION__);

			if (m_srvErasing)
			{
				m_srvErasing->stop();
				m_srvErasing.reset();
			}

			m_logRef.printEx("%s: Stop file system service.", __FUNCTION__);

			if (m_srvFLock)
			{
				m_srvFLock->stop();
				m_srvFLock.reset();
			}

			m_logRef.printEx("%s: Stop banking service.", __FUNCTION__);

			if (m_srvWallet)
			{
				m_srvWallet->stop();
				m_srvWallet.reset();
			}

			m_logRef.printEx("%s: services were stopped.", __FUNCTION__);

			return true;
		}

		void DebugServices()
		{
			auto& conf = DgiEngine::getConf();
			auto& log = conf.getLog(LOG_COMMON);

			using EncryptionService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiEncryptionService, ::dgi::DgiEncryptionProcessor>;
			auto serviceEncryption = std::make_shared<EncryptionService>(
				THRIFT_ENCRYPTION_PORT,
				conf.getLog(L"service encryption.log").getLogFilePath(),
				10);

			if (!serviceEncryption->start())
			{
				log.printEx("%s: error - couldn't start encryption service.", __FUNCTION__);
			}

			using EraseService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiSecureEraseService, ::dgi::DgiSecureEraseProcessor>;
			auto serviceErase = std::make_shared<EraseService>(
				THRIFT_ERASING_PORT,
				conf.getLog(L"service erase.log").getLogFilePath(),
				10);

			if (!serviceErase->start())
			{
				log.printEx("%s: error - couldn't start erase service.", __FUNCTION__);
			}

			using FLockService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiFolderlockService, ::dgi::DgiFolderLockProcessor>;
			auto serviceFLock = std::make_shared<FLockService>(
				THRIFT_FLOCK_PORT,
				conf.getLog(L"service flock.log").getLogFilePath(),
				10);

			if (!serviceFLock->start())
			{
				log.printEx("%s: error - couldn't start flock service.", __FUNCTION__);
			}

			using BankingService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiBankingService, ::dgi::DgiBankingProcessor>;
			auto serviceBanking = std::make_shared<BankingService>(
				THRIFT_BANKING_PORT,
				conf.getLog(L"service banking.log").getLogFilePath(),
				10);

			if (!serviceBanking->start())
			{
				log.printEx("%s: error - couldn't start encryption service.", __FUNCTION__);
			}

			log.printEx("%s: internal services started.\n\n(please note!): You can not close this application normally, terminate the process please.", __FUNCTION__);

			for (;;){
				Sleep(1000);
			}
		}
	}
}
