//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <memory>
#include <thread>
#include "AsyncEraseHandler.h"
#include "../type-convertors/TDgiEraseConvertor.h"

namespace service
{
	namespace thrift_impl
	{
		AsyncEraseDispatcher::AsyncEraseDispatcher(logic::secure_erase::manager::Shredder& _eraseManager, AsyncRequestsKeeper& _requestsKeeper, logfile& _log) :
			AsyncDispatcher(_requestsKeeper, _log),
			m_eraseManager(_eraseManager)
		{
			m_log.printEx("%s", __FUNCTION__);

			this->setActiveThreadsLimit(10);
		}

		AsyncEraseDispatcher::~AsyncEraseDispatcher()
		{
			m_log.printEx("%s", __FUNCTION__);

			ignoreNewRequests();

			// What until all threads completed.
			for (; getCountOfActiveHandlers() != 0;)
			{
				Sleep(10);
			}

			// I think it's not necessary but I prefer to do that.
			m_responseLock.lock();
			m_eraseResponses.clear();
			m_responseLock.unlock();
		}

		void AsyncEraseDispatcher::deleteResponseData()
		{
			std::unique_lock<std::mutex> lock(m_responseLock);

			m_eraseResponses.clear();
		}


		bool AsyncEraseDispatcher::deleteResponseData(::dgi::AsyncId _requestId)
		{
			std::unique_lock<std::mutex> lock(m_responseLock);

			if (m_eraseResponses.count(_requestId) != 0)
			{
				m_eraseResponses.erase(_requestId);
				return true;
			}

			return false;
		}

		::dgi::AsyncResponse AsyncEraseDispatcher::perform_eraseFiles(::dgi::EraseList _eraseList)
		{
			m_log.print(std::string(__FUNCTION__));

			::dgi::AsyncResponse error;

			if (limitAchieved() && (!canPerformNewRequest()))
			{
				m_log.print(std::string(__FUNCTION__) + ": error - thread limit achieved or new requests marked as ignored.");
				error.result.status = dgi::DgiStatus::LimitAchieved;
				return error;
			}

			// Register the fact of handling request.
			handlerStarted();

			try
			{
				::dgi::AsyncResponse result = createRequest();
				std::thread(AsyncEraseDispatcher::handler_eraseFiles, result.asyncId, std::ref(*this), _eraseList).detach();
				return result;
			}
			catch (...)
			{
				handlerCompleted();

				m_log.print(std::string(__FUNCTION__) + ": error - can't create thread, no resources.");
				error.result.status = dgi::DgiStatus::LimitAchieved;
				return error;
			}
		}


		bool AsyncEraseDispatcher::getResponse_eraseFiles(::dgi::AsyncId _requestId, ::dgi::EraseResponse& _response)
		{
			std::unique_lock<std::mutex> lock(m_responseLock);

			if (m_eraseResponses.count(_requestId) != 0)
			{
				try
				{
					// Return lin
					_response = m_eraseResponses.at(_requestId);

					// ... erasing skipped ... 
				}
				catch (...)
				{
					return false;
				}
			}

			return false;
		}

		bool AsyncEraseDispatcher::getResponseOnce_eraseFiles(::dgi::AsyncId _requestId, ::dgi::EraseResponse* _ptrCopyTo)
		{
			std::unique_lock<std::mutex> lock(m_responseLock);

			if (_ptrCopyTo == nullptr)
			{
				return false;
			}

			if (m_eraseResponses.count(_requestId) != 0)
			{
				try
				{
					*_ptrCopyTo = m_eraseResponses.at(_requestId);

					// Erase data.
					m_eraseResponses.erase(_requestId);
				}
				catch (...)
				{
					return false;
				}
			}

			return false;
		}

		//
		// Dispatchers do all necessary job.
		//

		void AsyncEraseDispatcher::handler_eraseFiles(dgi::AsyncId _requestId, AsyncEraseDispatcher& _dispatcherBkward,
			/* all additional parameters are here ... */
			::dgi::EraseList _eraseList
			)
		{
			_dispatcherBkward.log().printEx("%s", __FUNCTION__);

			//
			// That result should be in 'response queue' for future returning to the client who initiated request.
			//
			dgi::EraseResponse result;
			std::vector<::logic::common::EraseObject> intEraseList;
			std::vector<::logic::common::EraseObjectResult> intErrorsList;

			if (!fromThrift(_eraseList, intEraseList))
			{
				result.result.status = ::dgi::DgiStatus::InvalidFormat;

				_dispatcherBkward.log().printEx("%s: error - can't convert input parameters, because of invalid parameters type.", __FUNCTION__);
			}
			else
			{
				auto intStatus = _dispatcherBkward.eraseManager().eraseList(intEraseList, intErrorsList);

				if (!IntSuccess(intStatus))
				{
					result.result.status = ::dgi::DgiStatus::UnknownError;

					_dispatcherBkward.log().printEx("%s: error - operation failed with internal error code %d", __FUNCTION__, intStatus);
				}
				else
				{
					if (!intEraseList.empty())
					{
						_dispatcherBkward.log().printEx("%s: warning - not all files were removed", __FUNCTION__);
					}

					if (toThrift(intErrorsList, result.notErased))
					{
						//
						// Yep! Success.
						//
						result.result.status = ::dgi::DgiStatus::Success;
					}
					else
					{
						result.result.status = ::dgi::DgiStatus::InvalidFormat;
						_dispatcherBkward.log().printEx("%s: error - can't convert internal data types to thrift types.", __FUNCTION__);
					}
				}
			}

			// Save results.
			//
			std::unique_lock<std::mutex> lock(_dispatcherBkward.m_responseLock);
			_dispatcherBkward.m_eraseResponses[_requestId] = result;

			// Mark as finished
			_dispatcherBkward.completeRequest(_requestId);
		}

		//
		// Next dispatcher..
		//

	}
}
