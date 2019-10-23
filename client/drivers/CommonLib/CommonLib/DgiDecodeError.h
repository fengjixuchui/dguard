#pragma once

#include <string>

namespace szi 
{
	class  SziDecodeError 
	{
	public:
		SziDecodeError();
		~SziDecodeError();

		static std::wstring DecodeError(unsigned long NTStatusMessage, const std::wstring& module);
		static std::wstring DecodeLastError();
	};
}
