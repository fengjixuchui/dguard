
#include "DgiString.h"
#include <stdio.h>
#include <stdarg.h>

szi::SziUtilsString::SziUtilsString() {
}


szi::SziUtilsString::~SziUtilsString() {
}

szi::SziUtilsString::SziSplitStrings szi::SziUtilsString::Split(const std::wstring& inString, const std::wstring& delim)
{
	SziSplitStrings			lReturn;
	std::wstring::size_type lPosLast = 0;
	while (true)
	{
		std::wstring::size_type lPos = inString.find(delim, lPosLast);
		std::wstring			lStringRes(inString.begin() + lPosLast, lPos != std::wstring::npos ? inString.begin() + lPos : inString.end());
		if (!lStringRes.empty())
		{
			lReturn.emplace_back(lStringRes);
		}
		
		if (lPos == std::wstring::npos) break;
		lPosLast = lPos + 1;
	}
	return lReturn;
}

std::wstring szi::SziUtilsString::Format(const wchar_t * FormatStr, ...)
{
	wchar_t lBuffer[512] = L"\0";
	va_list args;
	va_start(args, FormatStr);
	vswprintf(lBuffer, sizeof(lBuffer) / sizeof(wchar_t), FormatStr, args);
	va_end(args);
	return lBuffer;
}