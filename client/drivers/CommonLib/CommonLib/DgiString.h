#pragma once

#include <list>
#include <string>

namespace szi
{
	class SziUtilsString
	{
	public:
		typedef std::list<std::wstring> SziSplitStrings;
	public:
		SziUtilsString();
		~SziUtilsString();

		static SziSplitStrings	Split(const std::wstring&, const std::wstring&);
		static std::wstring	Format(const wchar_t * FormatStr, ...);
	};

}
