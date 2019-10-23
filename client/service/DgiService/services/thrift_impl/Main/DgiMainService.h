//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "../../../stdafx.h"
#include "../ThriftManager/ThriftServiceManager.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCommon.h"

//
// Thats is an our service which is implemented here.
// 
#include "../../../../../../thrift/cpp/DgiServiceManager.h"

//
// These are our internal services.
//

#include "../Encryption/DgiEncryptionService.h"
#include "../Banking/DgiBankingService.h"
#include "../FolderLock/DgiFolderLockService.h"
#include "../SecureErase/DgiSecureEraseService.h"



namespace service
{
	namespace thrift_impl
	{
		using namespace ::logic::common;

		using namespace ::apache::thrift;
		using namespace ::apache::thrift::protocol;
		using namespace ::apache::thrift::transport;
		using namespace ::apache::thrift::server;


		class DgiMainService : virtual public ::dgi::DgiServiceManagerIf
		{

		public:
			DgiMainService(std::string _s);
			~DgiMainService();

			//
			//  Apache::thrift interface implementation is here.
			//

			virtual void run() {}
			virtual void stop() {}

			virtual void login(::dgi::AuthResponse& _return, const std::string& _masterPassword) override;
			virtual void logout(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid) override;
			virtual void changePassword(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const std::string& _currentPassword, const std::string& _newPassword) override;
			virtual void isRightPassword(::dgi::DgiResult& _return, const std::string& _masterPassword) override;
			virtual void isValidSession(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid) override;
			virtual void isPasswordSet(::dgi::BoolResponse& _return) override;
			virtual void setPassword(::dgi::DgiResult& _return, const std::string& _masterPassword) override;


			bool startInternalServices();
			bool stopInternalServices();

		protected:

		private:

			std::mutex m_lock;
			logfile& m_logRef;

			std::shared_ptr< ServiceManager<::service::thrift_impl::DgiEncryptionService, ::dgi::DgiEncryptionProcessor> > m_srvEncryption;
			std::shared_ptr< ServiceManager<::service::thrift_impl::DgiSecureEraseService, ::dgi::DgiSecureEraseProcessor> > m_srvErasing;
			std::shared_ptr< ServiceManager<::service::thrift_impl::DgiFolderlockService, ::dgi::DgiFolderLockProcessor> > m_srvFLock;
			std::shared_ptr< ServiceManager<::service::thrift_impl::DgiBankingService, ::dgi::DgiBankingProcessor> > m_srvWallet;

		};


		void DebugServices();
	}
}
