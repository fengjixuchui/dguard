//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "DgiCommon.h"

namespace logic
{
	namespace common
	{
		std::string getEncodedPassword(const EntryProtect& _entry)
		{
			auto length = _entry.passwordSize;

			if ((length != 0) && (length <= sizeof(_entry.passwordEncoded)))
			{
				return std::string( (const char*)_entry.passwordEncoded, length );
			}

			return "";
		}

		void setProtPassword(EntryProtect& _protection, std::string _password)
		{
			if (!_password.empty())
			{
				auto length = _password.length();

				if (length > sizeof(_protection.passwordEncoded))
				{
					// Limit size of used password.
					length = sizeof(_protection.passwordEncoded);
				}

				_protection.passwordSize = length;
				memcpy(_protection.passwordEncoded, _password.data(), length);
			}
		}

	}
}
