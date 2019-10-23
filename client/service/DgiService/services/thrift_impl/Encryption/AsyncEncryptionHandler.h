//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <atomic>
#include <memory>
#include <boost/noncopyable.hpp>
#include "../async/AsyncDispatcher.h"
#include "../../../logic/encryption/files/manager/FileEncoder.h"
#include "../../../../../../thrift/cpp/dgiEncryption_types.h"

namespace service
{
	namespace thrift_impl
	{
		class AsyncEncryptionDispatcher : public AsyncDispatcher
		{
		public:

			AsyncEncryptionDispatcher(logic::encryption::FileEncoder& _encoder, AsyncRequestsKeeper& _requestsKeeper, logfile& _log);
			~AsyncEncryptionDispatcher();

			::dgi::AsyncResponse perform_encryptFile(const ::dgi::RequestEncryptFile& _file);
			::dgi::AsyncResponse perform_decodeFile(const ::dgi::RequestDecodeFile& _file);

			//
			// Returns result of decoding operation - file can be:
			// decoded normally,
			// decoded but file body is not equal to original state - something was changed while file was encoded,
			// not decoded, because encryption key was wrong.
			//
			// (*) Response is removed immediately after returning from that method. The second call
			// with the same '_requestId' returns nothing.
			//bool resultDecodeFile(const ::dgi::AsyncResponse& _requestId, ::dgi::ResponseDecodeFile& _operationResult);

		private:
			
			static void handler_encodeFile(dgi::AsyncId _requestId, AsyncEncryptionDispatcher& _dispatcherBkward, ::dgi::RequestEncryptFile _request);
			static void handler_decodeFile(dgi::AsyncId _requestId, AsyncEncryptionDispatcher& _dispatcherBkward, ::dgi::RequestDecodeFile _request);

			logic::encryption::FileEncoder& getEncoder() const;

		private:

			logic::encryption::FileEncoder& m_encoder;

			// Requests results.
			//std::map<dgi::AsyncId, dgi::EraseResponse> m_eraseResponses;
		};
	}
}
