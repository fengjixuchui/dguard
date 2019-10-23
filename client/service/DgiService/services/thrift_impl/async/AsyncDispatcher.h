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
#include "../async/AsyncRequestsKeeper.h"
#include "../../../../../../thrift/cpp/dgiCommonTypes_types.h"


namespace service
{
	namespace thrift_impl
	{
		class AsyncDispatcher : private boost::noncopyable
		{
		public:

			AsyncDispatcher(AsyncRequestsKeeper& _requestsKeeper, logfile& _log);
			virtual ~AsyncDispatcher();


			// Returns true if we have no resources to handle new async request.
			bool limitAchieved();
			bool setActiveThreadsLimit(unsigned long _limit);
			unsigned long getActiveThreadsLimit();
			unsigned long getCountOfActiveHandlers();

			bool canPerformNewRequest() const;
			void ignoreNewRequests();

		protected:


			logfile& log() const;
			AsyncRequestsKeeper& requestsKeeper() const;

			// Register start and finish of handling process.
			// It's need to know how many request are handling in a current moment of time.
			void handlerStarted();
			void handlerCompleted();

			// Creates new request with a new id and marks it as 'in process'.
			dgi::AsyncResponse createRequest();

			// Marks the request as 'completed' or which you wish
			bool completeRequest(dgi::AsyncId _requestId, dgi::DgiStatus::type _completionStatus = dgi::DgiStatus::Completed);

			//
			// Members.
			//

			logfile& m_log;

		private:
			mutable  std::mutex m_lock;
			unsigned long m_activeThreadLimit;
			AsyncRequestsKeeper& m_requestsKeeper;
			std::atomic_ulong m_activeHandlers;
			bool m_ignoreNewRequest;
		};
	}
}
