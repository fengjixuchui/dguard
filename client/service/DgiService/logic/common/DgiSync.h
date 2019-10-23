//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <mutex>
#include <map>
#include <set>
#include <string>
#include <windows.h>
#include <boost/noncopyable.hpp>

namespace logic
{
	namespace common
	{
		class DgiSync : public ::boost::noncopyable
		{
		public:

			DgiSync();
			~DgiSync();

			//
			// These functions are for custom (manual) work with synchronization objects.
			//

			bool present(const std::string& _name);
			bool waitFor(const std::string& _name);
			bool verify(const std::string& _name);
			bool close(const std::string& _name);

			bool createEvent(const std::string& _name, bool _autoReset, bool _initState);
			bool createMutex(const std::string& _name, bool _initState);

			void closeAll();
			
			//
			//	Some hard-coded functions and Data Guard specific synchronization objects.
			//

			bool waitForProcessTermination(DWORD _timeout = INFINITE);
			bool verifyProcessTermination();
			bool setProcessTermination();
			bool resetProcessTermination();

			//	This event is set when all internal services said that we can terminate hole process.
			bool waitForReadyToComplete();
		
			//	This event is signaled when operation system wants to shutdown or reboot the computer.
			//	It means we can not waste or time on expensive and long operations.
			//	We should end our application as soon as possible.

			bool waitForShutdownEvent();
			bool setShutdown();

			bool waitForServiceReadyToComplete();
			bool setServiceReadyToComplete();
		
		protected:

			HANDLE getHandle(const std::string& _name);

		private:
			std::mutex m_lock;
			std::map<std::string, HANDLE> m_handles;
		};
	}
}

