//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiFolderLockService.h"
#include "../../../logic/session/DgiSession.h"
#include "../../../logic/common/DgiEngine.h"
#include "../type-convertors/TDgiThriftTypesConverter.h"
#include "../type-convertors/TDgiFLockConvertor.h"


namespace service
{
	namespace thrift_impl
	{
		// Here we have implementation of thrift-defined service methods.
		// Internally everything what we do here - just call DgiEngine methods.

		DgiFolderlockService::DgiFolderlockService(std::string _logfile):
			m_log(_logfile)
		{
			m_log.printEx("%s", __FUNCTION__);
		}

		DgiFolderlockService::~DgiFolderlockService()
		{
			m_log.printEx("%s", __FUNCTION__);

			//
			// Not to shutdown flock internal manager.
			//
		}

		void DgiFolderlockService::getSubsState(::dgi::SubSystemStateResponse& _return)
		{
			::dgi::SubSystemStateResponse results;

			results.hasProblems = true;

			::dgi::DgiResult err1;
			err1.status = ::dgi::DgiStatus::CriticalError;
			err1.description = "Subsystem is not implemented yet..";

			results.report.push_back(err1);
			results.result.status = ::dgi::DgiStatus::Success;
		}

        void DgiFolderlockService::isSupportedFs(::dgi::BoolResponse& _return, const std::string& _path)
        {
            auto fn = std::string(__FUNCTION__);
            m_log.print(fn);

            bool supported = false;
            std::wstring path = strings::s_ws(_path, CP_UTF8);

            if (DgiEngine::getFlock().isSupportedFs(path, supported))
            {
                if (supported)
                {
                    m_log.print(fn + ": info - target volume fs is supported.");
                }
                else
                {
                    m_log.print(fn + ": warning - target volume fs is not supported.");
                }

                _return.bool_result = supported;
                _return.errorResult.status = ::dgi::DgiStatus::Success;
            }
            else
            {
                m_log.print(fn + ": warning - an error occurred.");

                _return.bool_result = false;
                _return.errorResult.status = ::dgi::DgiStatus::UnknownError;
            }
        }

        ::dgi::DgiStatus::type DgiFolderlockService::add(const ::dgi::DgiSid& _sid, const ::dgi::FLockInfo& _flock)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				return ::dgi::DgiStatus::AccessDenied;
			}

			logic::folderlock::storage::FLockObject newFlock;

			if (fromThrift(_flock, newFlock))
			{
				// After data converting we can add it to our storage.
				auto status = DgiEngine::getFlock().add(newFlock);

				if (IntSuccess(status))
				{
					m_log.print(std::string(__FUNCTION__) + ": success - flock was added.");
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error - can't add new flock to storage, internal status code is " + std::to_string((int)status));
				}

				return toThrift(status);
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + ": error - can't convert from thrift-type to internal data type");

				return ::dgi::DgiStatus::InvalidFormat;
			}
		}

		void DgiFolderlockService::present(::dgi::BoolResponse& _return, const ::dgi::DgiSid& _sid, const ::dgi::utf8string& _flockPath)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.errorResult.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			std::wstring path = strings::s_ws(_flockPath, CP_UTF8);

			::logic::common::InternalStatus intState = ::logic::common::DgiEngine::getFlock().present(path);

			switch (intState)
			{
			case Int_Success:
				_return.bool_result = true;
				_return.errorResult.status = ::dgi::DgiStatus::Success;
				break;

			case Int_NotFound:
				m_log.print(std::string(__FUNCTION__) + ": warning - path not found.");

				_return.bool_result = false;
				_return.errorResult.status = ::dgi::DgiStatus::Success;
				break;

			default: // All other cases means some internal errors.
				m_log.print(std::string(__FUNCTION__) + ": error - path not found, internal status code is " + std::to_string((int)intState));

				_return.bool_result = false;
				_return.errorResult.status = toThrift(intState);
				break;
			}
		}

		void DgiFolderlockService::presentById(::dgi::BoolResponse& _return, const ::dgi::DgiSid& _sid, const std::string& _flockId)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.errorResult.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			if (_flockId.length() != sizeof(::logic::folderlock::storage::FlockIdLength) )
			{
				m_log.print(std::string(__FUNCTION__) + ": error - wrong length of unique flock id = " + std::to_string(_flockId.length()));
				_return.errorResult.status = ::dgi::DgiStatus::InvalidFormat;
				return;
			}

			::logic::common::InternalStatus intState = ::logic::common::DgiEngine::getFlock().presentById(_flockId);

			if (intState == Int_Success)
			{
				_return.errorResult.status = ::dgi::DgiStatus::Success;
				_return.bool_result = true;
			}
			else if (intState == Int_NotFound)
			{
				_return.errorResult.status = ::dgi::DgiStatus::Success;
				_return.bool_result = false;
			}
			else
			{
				std::string errorMsg = std::string(__FUNCTION__) + ": error - flock was not found, internal status code is " + std::to_string((int)intState);
				m_log.print(errorMsg);

				_return.errorResult.status = toThrift(intState);
				_return.errorResult.description = errorMsg;
			}
		}

		void DgiFolderlockService::getFlocks(::dgi::FLockListResponse& _return, const ::dgi::DgiSid& _sid)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.result.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			// Hope that everything will be ok.
			//
			_return.result.status = ::dgi::DgiStatus::Success;

			std::vector<::logic::folderlock::storage::FLockObject> flocks;
			auto result = DgiEngine::getFlock().getAll(flocks);

			if (IntSuccess(result))
			{
				for (auto flock : flocks)
				{
					::dgi::FLockInfo thriftFlock;

					// Convert from internal data type to external.
					if (::service::thrift_impl::toThrift(flock, thriftFlock))
					{
						// Ok, converted successfully.
						// Save info in response.
						//
						_return.flocks.push_back(thriftFlock);
					}
					else
					{
						m_log.print(std::string(__FUNCTION__) + ": error - can't convert from internal type to thrift-defined type.");
						_return.result.status = ::dgi::DgiStatus::InvalidFormat;
					}
				}
			}
			else
			{
				_return.result.status = ::service::thrift_impl::toThrift(result);
			}
		}

        void DgiFolderlockService::setStateById(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid, const std::string& _flockId, const ::dgi::FLockState::type _newState)
        {
            std::string fn = std::string(__FUNCTION__);
            m_log.print(fn);

            if (::logic::session::Storage::get().present(_sid) == false)
            {
                m_log.print(fn + ": error - invalid session " + _sid);
                _return.status = ::dgi::DgiStatus::AccessDenied;
                return;
            }

            auto result = DgiEngine::getFlock().presentById(_flockId);

            if (!IntSuccess(result))
            {
                m_log.print(fn + ": error - could not verify flock presence in storage.");
                _return.status = toThrift(result);
                return;
            }

            auto newInternalState = fromThriftFlockState(_newState);

            result = DgiEngine::getFlock().changeStateById(_flockId, newInternalState);

            if (IntSuccess(result))
            {
                m_log.print(fn + ": info - flock's state was changed.");
                _return.status = ::dgi::DgiStatus::Success;
            }
            else
            {
                m_log.print(fn + ": error - could not change flock's state in storage, error code is " + std::to_string((int)result));
                _return.status = toThrift(result);
            }
        }

        void DgiFolderlockService::getState(::dgi::FLockStateResponse& _return, const ::dgi::DgiSid& _sid, const ::dgi::utf8string& _flockPath)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.result.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			std::wstring path = strings::s_ws(_flockPath, CP_UTF8);
			::logic::folderlock::storage::FLockObject flockInfo;

			auto result = DgiEngine::getFlock().get(path, flockInfo);

			if (IntSuccess(result))
			{
				// Need to use thrift_impl:: type convertors
				//
				if (::service::thrift_impl::toThrift(flockInfo, _return.flinf))
				{
					_return.result.status = ::dgi::DgiStatus::Success;
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error - can't convert internal flock type to external");
					_return.result.status = ::dgi::DgiStatus::InvalidFormat;
				}
			}
			else
			{
				std::string msg = std::string(__FUNCTION__) + ": error - can't read flock state, internal status is " + std::to_string((int)result);
				m_log.print(msg);

				_return.result.status = toThrift(result);
				_return.result.description = msg;
			}
		}

		void DgiFolderlockService::setState(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::utf8string& _flockPath, const  ::dgi::FLockState::type _newState)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			std::wstring path = strings::s_ws(_flockPath, CP_UTF8);
			auto result = DgiEngine::getFlock().present(path);

			if (!IntSuccess(result))
			{
				m_log.print(std::string(__FUNCTION__) + ": error - could not verify flock presence in storage.");
				_return.status = toThrift(result);
				return;
			}

			auto newInternalState = fromThriftFlockState(_newState);
			result = DgiEngine::getFlock().changeState(path, newInternalState);

			if (IntSuccess(result))
			{
				m_log.print(std::string(__FUNCTION__) + ": info - flock's state was changed.");
				_return.status = ::dgi::DgiStatus::Success;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + ": error - could not change flock's state in storage, error code is " + std::to_string((int)result));
				_return.status = toThrift(result);
			}
		}

		void DgiFolderlockService::remove(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid, const ::dgi::utf8string& _flockPath)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			std::wstring path = strings::s_ws(_flockPath, CP_UTF8);
			auto result = DgiEngine::getFlock().present(path);

			if (!IntSuccess(result))
			{
				m_log.print(std::string(__FUNCTION__) + ": error - we tried to remove unknown flock, internal error " + std::to_string((int)result));
				_return.status = toThrift(result);
			}
			else
			{
				// Remove all access locks to a specific file.
				result = DgiEngine::getFlock().remove(path);

				if (IntSuccess(result))
				{
					m_log.print(std::string(__FUNCTION__) + ": success - flock was removed.");
					_return.status = toThrift(result);

// Update information in driver.
// 					result = DgiEngine::getFlock().sendFlocksToDriver();
// 
// 					if (IntSuccess(result))
// 					{
// 						m_log.print(std::string(__FUNCTION__) + ": success - flock was removed.");
// 						_return.status = toThrift(result);
// 					}
// 					else
// 					{
// 						m_log.print(std::string(__FUNCTION__) + ": error - could not send flocks list to driver, internal error " + std::to_string((int)result));
// 						_return.status = toThrift(result);
// 					}
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error - could not remove flock, internal error " + std::to_string((int)result));
					_return.status = toThrift(result);
				}
			}
		}

		void DgiFolderlockService::removeAll(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (::logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			auto result = DgiEngine::getFlock().clear();

			if (IntSuccess(result))
			{
				m_log.print(std::string(__FUNCTION__) + ": success(!) - flocks storage is cleared.");
				_return.status = toThrift(result);
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + ": error - flocks storage was not cleared.");
				_return.status = toThrift(result);
			}
		}

		void DgiFolderlockService::getCacheInfo(::dgi::FLockCacheInfo& _return)
		{
			m_log.printEx("%s: is not implemented yet.", __FUNCTION__);
		}

	}
}
