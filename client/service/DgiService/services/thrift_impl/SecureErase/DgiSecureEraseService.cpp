//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiSecureEraseService.h"
#include "../../../logic/session/DgiSession.h"
#include "../../../logic/common/DgiEngine.h"
#include "../type-convertors/TDgiThriftTypesConverter.h"

namespace service
{

	namespace thrift_impl
	{

		DgiSecureEraseService::DgiSecureEraseService(std::string _logfile) :
			m_log(_logfile),
			m_asyncDispatcher(logic::common::DgiEngine::getShredder(), m_requestsKeeper, m_log)
		{

		}

		void DgiSecureEraseService::eraseFiles(::dgi::AsyncResponse& _return, const ::dgi::EraseList& _toErase)
		{
			m_log.printEx("%s", __FUNCTION__);

			//
			// Verify only presence of the operation.
			//
// 			if (logic::session::Storage::get().present(_sid) == false)
// 			{
// 				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
// 				_return.result.status = ::dgi::DgiStatus::AccessDenied;
// 				return;
// 			}

			//
			// Perform operation in a background.
			//
			_return = m_asyncDispatcher.perform_eraseFiles(_toErase);
		}

		void DgiSecureEraseService::getState(::dgi::DgiResult& _return, const ::dgi::AsyncId _asyncId)
		{
			m_log.printEx("%s", __FUNCTION__);

			dgi::DgiStatus::type state;
			if (m_requestsKeeper.getState(_asyncId, state))
			{
				_return.status = state;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + ": error - async id not registered " + std::to_string(_asyncId));
				_return.status = ::dgi::DgiStatus::NotFound;
			}
		}

		void DgiSecureEraseService::getEraseState(::dgi::EraseResponse& _return, const ::dgi::AsyncId _asyncId)
		{
			m_log.printEx("%s", __FUNCTION__);

			dgi::DgiStatus::type state;
			if (m_requestsKeeper.getState(_asyncId, state))
			{
				if (state != dgi::DgiStatus::InProcess)
				{
					// Ok, request handled and we can receive a response
					//
					if (m_asyncDispatcher.getResponse_eraseFiles(_asyncId, _return))
					{
						// Remove response data. Second call of this method returns nothing.
						//
						m_asyncDispatcher.deleteResponseData(_asyncId);
					}
					else
					{
						// Request is handled but response is absent.
						//
						_return.result.status = ::dgi::DgiStatus::HaveNoResponse;

						m_log.print(std::string(__FUNCTION__) + ": error - request handled, but response is absent, async id " + std::to_string(_asyncId));
					}
				}
				else
				{
					// Still busy.
					//
					_return.result.status = state;
				}
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + ": error - async id not registered " + std::to_string(_asyncId));

				_return.result.status = ::dgi::DgiStatus::NotFound;
			}
		}

		void DgiSecureEraseService::eraseFile(::dgi::DgiResult& _return, const std::string& _filepath)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			std::wstring filepath = strings::s_ws(_filepath, CP_UTF8);

			::logic::secure_erase::manager::Shredder& shredder = ::logic::common::DgiEngine::getShredder();

			auto ires = shredder.eraseFile(filepath);

			if (!IntSuccess(ires))
			{
				m_log.print(fn + ": error - " + " internal status code is " + std::to_string((int)ires)  +  ", can't delete file " + _filepath + "(utf8)" );
			}

			_return.status = ::service::thrift_impl::toThrift(ires);
		}

	}
}
