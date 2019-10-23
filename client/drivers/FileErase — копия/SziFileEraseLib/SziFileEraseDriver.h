#pragma once

#if defined(_MSC_VER)
#include <Windows.h>
#elif
typedef HANDLE int;
#endif

#include <string>
#include <map>
#include <algorithm>

namespace szi
{
	namespace drv
	{
		class FileEraseClient 
		{
			typedef std::map<std::wstring, HANDLE> EraseEvents;
		public:
			FileEraseClient();
			virtual ~FileEraseClient();
			bool Open();
			void Close();
			bool EraseFile(const std::wstring& fileName);
			bool waitEraseComplete(const std::wstring& fileName);
			bool IsAutoErase() const;
			bool EnableAutoErase();
			bool DisableAutoErase();
			bool SetMask(unsigned char);
			bool SetCountCircle(unsigned int);
			bool GetPercents(const std::wstring& fileName, unsigned & percents) const;
			long GetNStatusComplete() const;
			std::wstring GetStatusComplete() const;
			std::wstring GetStatusComplete(long ntStatus) const;
			HANDLE GetEventComplete(const std::wstring& fileName) const;
			std::wstring GetLastError() const;
			bool SetAutoErase(bool);

		public:
			HANDLE			m_driver;
	mutable std::wstring	m_lastError;
			EraseEvents		m_events;
		};
	}
}

