//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <string>


namespace sys
{
	namespace privileges
	{
		bool getPrivilege(const std::string& privName);
		void getMaxPrivileges(void);
	}
}
