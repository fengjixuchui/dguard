//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "AsyncDispatcher.h"

namespace service
{
	namespace thrift_impl
	{
		AsyncDispatcher::AsyncDispatcher(AsyncRequestsKeeper& _requestsKeeper, logfile& _log) :
			m_activeThreadLimit(10),
			m_log(_log),
			m_requestsKeeper(_requestsKeeper),
			m_ignoreNewRequest(false)
		{
			m_log.printEx("%s", __FUNCTION__);

			m_activeHandlers = 0;
		}

		AsyncDispatcher::~AsyncDispatcher()
		{
			m_log.printEx("%s", __FUNCTION__);

			ignoreNewRequests();

			//
			// Wait for completion of all requests.
			//
			// ...

			// What until all threads completed.
			//for (; getCountOfActiveHandlers() != 0;){
			//	Sleep(10);
			//}
		}

		service::thrift_impl::AsyncRequestsKeeper& AsyncDispatcher::requestsKeeper() const
		{
			return m_requestsKeeper;
		}

		logfile& AsyncDispatcher::log() const
		{
			return m_log;
		}

		unsigned long AsyncDispatcher::getCountOfActiveHandlers()
		{
			return m_activeHandlers.load();
		}

		void AsyncDispatcher::handlerStarted()
		{
			m_activeHandlers.operator++();
		}

		void AsyncDispatcher::handlerCompleted()
		{
			m_activeHandlers.operator--();
		}

		bool AsyncDispatcher::setActiveThreadsLimit(unsigned long _limit)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (m_activeThreadLimit == 0){
				return false;
			}

			m_activeThreadLimit = _limit;
			return true;
		}

		unsigned long AsyncDispatcher::getActiveThreadsLimit()
		{
			std::unique_lock<std::mutex> lock(m_lock);
			return m_activeThreadLimit;
		}

		bool AsyncDispatcher::limitAchieved()
		{
			if (getCountOfActiveHandlers() <= getActiveThreadsLimit())
			{
				return true;
			}

			return false;
		}

		dgi::AsyncResponse AsyncDispatcher::createRequest()
		{
			dgi::AsyncResponse result;
			result.asyncId = m_requestsKeeper.createNew(dgi::DgiStatus::InProcess);
			result.result.status = dgi::DgiStatus::Success;
			return result;
		}

		bool AsyncDispatcher::completeRequest(dgi::AsyncId _requestId, dgi::DgiStatus::type _completionStatus /*= dgi::DgiStatus::Completed*/)
		{
			if (m_requestsKeeper.isPresent(_requestId))
			{
				m_requestsKeeper.setState(_requestId, _completionStatus);
				return true;
			}

			return false;
		}

		bool AsyncDispatcher::canPerformNewRequest() const
		{
			std::unique_lock<std::mutex> lock(m_lock);
			return m_ignoreNewRequest == false;
		}

		void AsyncDispatcher::ignoreNewRequests()
		{
			std::unique_lock<std::mutex> lock(m_lock);
			m_ignoreNewRequest = true;
		}

	}
}
