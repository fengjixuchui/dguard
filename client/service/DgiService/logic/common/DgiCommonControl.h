//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <string>
#include <boost/noncopyable.hpp>
#include "DgiCommon.h"
#include "master-password.h"

namespace logic
{
	namespace common
	{
		//
		//	Common interface which should be implemented and supported by all internal Data Guard subsystems:
		//	folder lock, secure erasing, bank cards keeper, encryption and etc.
		//

		class DgiCommonControl
		{
		public:
			virtual bool ctrInit() = 0;
			virtual bool ctrlLateInit() = 0;
			virtual bool ctrlIsRunning() = 0;
			virtual bool ctrlShutdown(bool _canWait) = 0;

			//
			//	Returns subsystem name and that name can not be an empty string.
			//

			virtual std::string ctrlGetName() = 0;
			
			//
			//  This is a handler for event which is caused after user authenticates with right master-password.
            //  Method should return result as fast as possible.
			//

			virtual bool ctrlSetPassword(::logic::common::MasterPassword _password) = 0;

			//
			//	Method to keep data in a secure state and used by external modules.
            //  Method should return result as fast as possible!
			//
			//	For example:
			//		1. User changed master-password;
			//		2. Data Guard main module (::logic::common::DgiEngine) is responsible for call to this method.
			//

			virtual bool ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword) = 0;

		};
	}
}
