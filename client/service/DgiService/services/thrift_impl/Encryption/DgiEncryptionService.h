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

#include "../async/AsyncRequestsKeeper.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCommon.h"

#include "../../../../../../thrift/cpp/DgiEncryption.h"
#include "AsyncEncryptionHandler.h"


namespace service
{
	namespace thrift_impl
	{
		using namespace ::logic::common;

		using namespace ::apache::thrift;
		using namespace ::apache::thrift::protocol;
		using namespace ::apache::thrift::transport;
		using namespace ::apache::thrift::server;


		//
		// Apache::thrift interface implementation is here.
		//

		class DgiEncryptionService : virtual public ::dgi::DgiEncryptionIf
		{

		public:
			DgiEncryptionService(std::string _logfile);
			~DgiEncryptionService();

			virtual void isFileEncoded(::dgi::BoolResponse& _return, const std::string& _filepath) override;
			virtual void getFileInfo(::dgi::ResponseFileInfo& _return, const std::string& _filepath) override;
			virtual void encryptFileAsync(::dgi::AsyncResponse& _return, const  ::dgi::RequestEncryptFile& _file) override;
			virtual void encryptFile(::dgi::DgiResult& _return, const  ::dgi::RequestEncryptFile& _file) override;
			virtual void getState(::dgi::DgiResult& _return, const  ::dgi::AsyncId _asyncId) override;
			virtual void decodeFile(::dgi::ResponseDecodeFile& _return, const  ::dgi::RequestDecodeFile& _file) override;
			virtual void decodeFileAsync(::dgi::AsyncResponse& _return, const  ::dgi::RequestDecodeFile& _file) override;

		protected:

		private:

			logfile m_log;
			AsyncRequestsKeeper m_requestsKeeper; // all id's are registered here.
			::logic::encryption::FileEncoder m_fileEncoder;
			AsyncEncryptionDispatcher m_asyncDispatcher; // manager for handling operations in a background mode.

		};
	}
}
