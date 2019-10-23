//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "AsyncRequestsKeeper.h"


namespace service
{
	namespace thrift_impl
	{
		AsyncRequestsKeeper::AsyncRequestsKeeper() :m_counter(0)
		{
			// ...
		}

		bool AsyncRequestsKeeper::isPresent(const dgi::AsyncId _requestId) const
		{
			return m_operationStates.Present(_requestId);
		}

		dgi::AsyncId AsyncRequestsKeeper::createNew(dgi::DgiStatus::type requestState /*= ics_service::IcsStatusCode::Ics_NotFinishedYet*/)
		{
			std::unique_lock<std::mutex> mtxlocker(m_counterLock);

			auto result = ++m_counter;
			m_operationStates.Add(result, requestState);
			return result;
		}

		bool AsyncRequestsKeeper::getState(const dgi::AsyncId _requestId, dgi::DgiStatus::type& _outRequestState)
		{
			return m_operationStates.getValueIfPresent(_requestId, _outRequestState);
		}

		void AsyncRequestsKeeper::setState(const dgi::AsyncId _requestId, const dgi::DgiStatus::type _newRequestState)
		{
			m_operationStates.setValueIfPresent(_requestId, _newRequestState);
		}

		void AsyncRequestsKeeper::clear()
		{
			m_operationStates.Clear();
		}

		bool AsyncRequestsKeeper::isFinished(dgi::AsyncId _requestId)
		{
			dgi::DgiStatus::type state;

			if (getState(_requestId, state))
			{
				return state != dgi::DgiStatus::InProcess;
			}

			// If we have no info about the request.
			//
			return false;
		}
	}
}
