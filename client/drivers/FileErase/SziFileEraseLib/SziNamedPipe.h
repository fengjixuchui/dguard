#pragma once

#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <windows.h>
#else
typedef HANDLE int
#endif

namespace szi
{
	class SziNamedPipe 
	{
	public:
		SziNamedPipe();
		~SziNamedPipe();

		bool Create(const std::wstring& name);
		bool Open(const std::wstring& name, unsigned int timeout);
		void Close();
		bool Write(const std::wstring& message);
		bool Read(std::wstring& messages, unsigned int timeout);
		std::wstring GetLastError() const;
		unsigned int GetCodeLastError() const;
	private:
		bool IsBase64(unsigned char c) const;
		std::string Base64Encode(const std::wstring& toEncode) const;
		std::wstring Base64Decode(const std::string& toDecode) const;
	private:
		HANDLE			pipe_;
		unsigned int	error_;
	};
}
