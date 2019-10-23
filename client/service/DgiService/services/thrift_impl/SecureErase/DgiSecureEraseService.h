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

#include "../../../../../../thrift/cpp/DgiSecureErase.h"
#include "../async/AsyncRequestsKeeper.h"
#include "AsyncEraseHandler.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCommon.h"

namespace service
{
	namespace thrift_impl
	{
		using namespace ::logic::common;

		using namespace ::apache::thrift;
		using namespace ::apache::thrift::protocol;
		using namespace ::apache::thrift::transport;
		using namespace ::apache::thrift::server;


		class DgiSecureEraseService : virtual public ::dgi::DgiSecureEraseIf
		{

		public:
			DgiSecureEraseService(std::string _logfile);

			//
			// Apache::thrift interface implementation is here.
			//

			virtual void eraseFiles(::dgi::AsyncResponse& _return, const ::dgi::EraseList& _toErase) override;
			virtual void getState(::dgi::DgiResult& _return, const ::dgi::AsyncId _asyncId) override;

			//
			// Gets all results of early initiated operation.
			//
			virtual void getEraseState(::dgi::EraseResponse& _return, const ::dgi::AsyncId _asyncId) override;

			//
			// Secure delete file ind its data.
			//
			virtual void eraseFile(::dgi::DgiResult& _return, const std::string& _filepath) override;

		protected:

		private:

			logfile m_log;
			AsyncRequestsKeeper m_requestsKeeper; // all id's are registered here.
			AsyncEraseDispatcher m_asyncDispatcher; // manager for handling operations in a background mode.

		};
	}
}
