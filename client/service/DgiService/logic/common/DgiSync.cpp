//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiSync.h"

#define	INTERNAL_EV_PROC_TERM	"dgev-proc-term"

namespace logic
{
	namespace common
	{
		
		DgiSync::DgiSync()
		{
			HANDLE hProcTerm = CreateEventA(NULL, TRUE, FALSE, INTERNAL_EV_PROC_TERM);

			if (hProcTerm)
			{
				m_handles[INTERNAL_EV_PROC_TERM] = hProcTerm;
			}
		}

		DgiSync::~DgiSync()
		{
			closeAll();
		}

		void DgiSync::closeAll()
		{
			std::unique_lock<std::mutex> lock(m_lock);

			for (auto obj : m_handles )
			{
				CloseHandle(obj.second);
			}

			m_handles.clear();
		}

		bool DgiSync::waitForProcessTermination(DWORD _timeout)
		{
			HANDLE hobj = getHandle(INTERNAL_EV_PROC_TERM);

			if (hobj)
			{
				auto res = WaitForSingleObject(hobj, _timeout);

				if (res == WAIT_OBJECT_0)
				{
					return true;
				}

				// Timeout and other cases will lead to false result.
			}

			return false;
		}

		bool DgiSync::verifyProcessTermination()
		{
			HANDLE hobj = getHandle(INTERNAL_EV_PROC_TERM);
			
			if (hobj)
			{
				return ( WAIT_OBJECT_0 == WaitForSingleObject(hobj, 0) );
			}

			return false;
		}

		bool DgiSync::setProcessTermination()
		{
			HANDLE hobj = getHandle(INTERNAL_EV_PROC_TERM);

			if (hobj)
			{
				return SetEvent(hobj) == TRUE;
			}

			return false;
		}

		bool DgiSync::resetProcessTermination()
		{
			HANDLE hobj = getHandle(INTERNAL_EV_PROC_TERM);

			if (hobj)
			{
				return ResetEvent(hobj) == TRUE;
			}

			return false;
		}

		bool DgiSync::waitForShutdownEvent()
		{
			// Not imple,emted yet.
			return false;
		}

		bool DgiSync::setShutdown()
		{
			// Not imple,emted yet.
			return false;
		}

		bool DgiSync::waitForServiceReadyToComplete()
		{
			// Not imple,emted yet.
			return false;
		}

		bool DgiSync::setServiceReadyToComplete()
		{
			// Not imple,emted yet.
			return false;
		}

		HANDLE DgiSync::getHandle(const std::string& _name)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			auto hi = m_handles.find(_name);

			if (hi != m_handles.end())
			{
				// return handle.
				return hi->second;
			}

			return 0;
		}

	}
}

