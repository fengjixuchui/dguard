//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <atomic>
#include <memory>
#include <boost/noncopyable.hpp>
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCommon.h"
#include "../async/AsyncDispatcher.h"
#include "../../../logic/erasing/manager/ManagerSecureErase.h"
#include "../../../../../../thrift/cpp/dgiCommonTypes_types.h"
#include "../../../../../../thrift/cpp/dgiSecureEraseTypes_types.h"

namespace service
{
	namespace thrift_impl
	{
		class AsyncEraseDispatcher : public AsyncDispatcher
		{
		public:

			AsyncEraseDispatcher(logic::secure_erase::manager::Shredder& _eraseManager, AsyncRequestsKeeper& _requestsKeeper, logfile& _log);
			~AsyncEraseDispatcher();

			// Performs erasing in a background thread.
			::dgi::AsyncResponse perform_eraseFiles(::dgi::EraseList _eraseList);

			// Returns true if response is ready.
			bool getResponse_eraseFiles(::dgi::AsyncId _requestId, ::dgi::EraseResponse& _response);

			// Returns true if response is ready and deletes response data from the internal storage.
			bool getResponseOnce_eraseFiles(::dgi::AsyncId _requestId, ::dgi::EraseResponse* _ptrCopyTo);


			// Deletes response data.
			// (*) Should be called after getResponse_eraseFiles(...)
			//
			bool deleteResponseData(::dgi::AsyncId _requestId);

			// Deletes all data prepared to send back all responses.
			//
			void deleteResponseData();

		private:

			// Basically handler async request handler should receive following parameters:
			// 1 - new generated async operation id
			// 2 - backward link to current object (AsyncRequestsDispatcher)
			// 3 - all need thrift-types for implementation of request-operation

			// Erases files and saves results in internal response storage-container for future returning data back to the requestor.
			//
			static void handler_eraseFiles(dgi::AsyncId _requestId, AsyncEraseDispatcher& _dispatcherBkward, ::dgi::EraseList _eraseList );


			logic::secure_erase::manager::Shredder& eraseManager() const
			{
				return m_eraseManager;
			};

		private:
			std::mutex m_responseLock;
			logic::secure_erase::manager::Shredder& m_eraseManager;

			// Requests results.
			std::map<dgi::AsyncId, dgi::EraseResponse> m_eraseResponses;
		};
	}
}
