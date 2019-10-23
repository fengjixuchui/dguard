//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiSession.h"
#include "../../helpers/internal/helpers.h"

namespace logic
{
	namespace session
	{
		const wchar_t InternalSessionName[] = L"@internal-0";

		//
		// Storage.
		//

		Manager* Storage::g_sziUserSessions = nullptr;
		std::mutex Storage::g_lock;

		Manager& Storage::get()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			if (g_sziUserSessions == nullptr)
			{
				g_sziUserSessions = new Manager();
			}

			return *g_sziUserSessions;
		}

		void Storage::destroy()
		{
			std::unique_lock<std::mutex> mtxlocker(g_lock);

			delete g_sziUserSessions;
			g_sziUserSessions = nullptr;
		}

		//
		// Manager.
		//

		SessionId Manager::getInternalSession()
		{
			SessionId sid;
			if (getForHolder(InternalSessionName, sid))
			{
				return sid;
			}

			Info si;
			si.holder = InternalSessionName; 
			si.creationTime = timefn::getCurrentTime();
			si.sid = strings::generateUuid();

			m_sessions[si.sid] = si;

			return si.sid;
		}

		logic::session::SessionId Manager::create()
		{
			logic::session::SessionId ssid = strings::generateUuid();

			Info si;
			si.holder = L"user";
			si.creationTime = timefn::getCurrentTime();
			si.sid = ssid;

			this->saveSession(ssid, si);

			return ssid;
		}

		void Manager::saveSession(SessionId _id, Info _info)
		{
			m_sessions[_id] = _info;
		}

		void Manager::close(SessionId _id)
		{
			m_sessions.Remove(_id);
		}

		void Manager::closeAll()
		{
			m_sessions.Clear();
		}

		bool Manager::present(SessionId _session)
		{
			return true;

#ifdef _DEBUG
			return true;
#endif
			return m_sessions.Present(_session);
		}

		bool Manager::getSessionInfo(SessionId _sid, Info& _info)
		{
			return m_sessions.getValueIfPresent(_sid, _info);
		}

		bool Manager::getForHolder(const std::wstring& _holder, SessionId& _sid)
		{
			m_sessions.Lock();
			auto sessions = m_sessions.GetStlMap();
			m_sessions.Unlock();

			for (auto si : sessions)
			{
				if (si.second.holder == _holder)
				{
					_sid = si.second.sid;
					return true;
				}
			}

			return false;
		}

		logic::session::Manager::SessionTable::ThsMapType Manager::getSessions()
		{
			m_sessions.Lock();
			auto res = m_sessions.GetStlMap();
			m_sessions.Unlock();

			//
			// All open sessions.
			//
			return res;
		}

	}
}
