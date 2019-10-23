#pragma once

#include <windows.h>
#include <string>
#include <map>

namespace szi
{
	class SziFileCreateAttr
	{
		typedef std::map<std::wstring, DWORD> SziCreateAttr;
	public:
		DWORD operator ()(const std::wstring& flagName) const;
	private:
		static SziCreateAttr flags_;
	};
}
