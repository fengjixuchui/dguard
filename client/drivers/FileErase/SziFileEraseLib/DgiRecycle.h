#pragma once

#include <string>

namespace szi
{
	//Откючает\Вкючает корзину для пользователя
	class SziBitBucket 
	{
	public:
		SziBitBucket();
		~SziBitBucket();

		//sidUser - user sid "S-1-5-21-281876179-808521044-535982850-4111"
		//enable - true\false
		bool SetBySid(const std::wstring& sidUser, const bool enable);
		bool SetLocal(const bool enable);
	};
}
