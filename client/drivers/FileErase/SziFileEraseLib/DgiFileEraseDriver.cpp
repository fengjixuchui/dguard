#include "DgiFileEraseDriver.h"
#include "DriverFileErase.h"
#include "DgiDecodeError.h"
#include "DgiRegistry.h"

#include <WinIoCtl.h>

#define REGAUTOERASEKEY					L"SYSTEM\\CurrentControlSet\\services\\DgiFileErase"
#define REGAUTOERASEVALUENAME			L"AutoErase"

namespace szi
{
	namespace drv
	{
		FileEraseClient::FileEraseClient()
			:m_driver(INVALID_HANDLE_VALUE)
		{
		}

		FileEraseClient::~FileEraseClient()
		{
			Close();
		}

		bool FileEraseClient::Open()
		{
			if (m_driver == INVALID_HANDLE_VALUE)
			{
				m_driver = CreateFile(DEVICE_NAME_USER, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (m_driver == INVALID_HANDLE_VALUE)
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"");
					return false;
				}
			}
			return true;
		}

		void FileEraseClient::Close()
		{
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_driver);
				m_driver = INVALID_HANDLE_VALUE;
			}
			std::for_each(m_events.begin(), m_events.end(), [](EraseEvents::value_type& pair)
			              {
				              CloseHandle(pair.second);
			              });
			m_events.clear();
		}

		std::wstring FileEraseClient::GetLastError() const
		{
			return m_lastError;
		}
		
		bool FileEraseClient::EraseFile(const std::wstring& fileName)
		{
			bool result = false;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				USER_ERASEFILE_CONTEXT context;
				context.FileName = fileName.c_str();
				context.Length = fileName.length();
				context.Event = CreateEvent(NULL, FALSE, FALSE, NULL);
				if (context.Event == INVALID_HANDLE_VALUE)
				{
					m_lastError = L"error - can't create an event.";
				}
				else
				{
					DWORD lByte = 0;
					if (!DeviceIoControl(m_driver, IOCTL_ERASE_FILE, &context, sizeof(context), NULL, 0, &lByte, NULL))
					{
						CloseHandle(context.Event);
						m_lastError = szi::SziDecodeError::DecodeError(::GetLastError(), L"");
					}
					else
					{
						m_events.insert(std::make_pair(fileName, context.Event));
						m_lastError.clear();
						result = true;
					}
				}
			}
			return result;
		}

		bool FileEraseClient::waitEraseComplete(const std::wstring& fileName)
		{
			HANDLE hEvent = GetEventComplete(fileName);

			if (hEvent != INVALID_HANDLE_VALUE)
			{
				return WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
			}

			return false;
		}

		HANDLE FileEraseClient::GetEventComplete(const std::wstring& fileName) const
		{
			auto itEvent = m_events.find(fileName);
			return itEvent != m_events.end() ? itEvent->second : INVALID_HANDLE_VALUE;
		}

		bool FileEraseClient::GetPercents(const std::wstring& fileName, unsigned& percents) const
		{
			return false;
		}

		bool FileEraseClient::IsAutoErase() const
		{
			bool result = false;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				DWORD value = 0, retLength = 0;
				BOOL retCtrl = DeviceIoControl(m_driver, IOCTL_ERASE_ISAUTOERASE, NULL, NULL, &value, sizeof(value), &retLength, NULL);
				if (!retCtrl)
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"NTDLL.DLL");
					result = false;
				}
				else
					result = value > 0 ? true : false;
			}
			return result;
		}

		bool FileEraseClient::EnableAutoErase()
		{
			bool result = false;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				DWORD value = 1, retLength = 0;
				result = TRUE == DeviceIoControl(m_driver, IOCTL_ERASE_AUTOERASE, &value, sizeof(value), NULL, NULL, &retLength, NULL);
				if (!result)
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"NTDLL.DLL");
				}
			}
			return result;
		}

		bool FileEraseClient::DisableAutoErase()
		{
			bool result = false;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				DWORD value = 0, retLength = 0;
				result = TRUE == DeviceIoControl(m_driver, IOCTL_ERASE_AUTOERASE, &value, sizeof(value), NULL, NULL, &retLength, NULL);
				if (!result)
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"NTDLL.DLL");
				}
			}
			return result;
		}

		bool FileEraseClient::SetMask(unsigned char value)
		{
			bool result = false;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				DWORD retLength = 0;
				BOOL retCtrl = DeviceIoControl(m_driver, IOCTL_ERASE_MASK, &value, sizeof(value), NULL, NULL, &retLength, NULL);
				if (!retCtrl)
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"NTDLL.DLL");
					result = false;
				}
				else
					result = true;
			}
			return result;
		}

		bool FileEraseClient::SetCountCircle(unsigned int count)
		{
			bool result = false;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				DWORD retLength = 0;
				BOOL retCtrl = DeviceIoControl(m_driver, IOCTL_ERASE_COUNT, &count, sizeof(count), NULL, NULL, &retLength, NULL);
				if (!retCtrl)
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"NTDLL.DLL");
					result = false;
				}
				else
					result = true;
			}
			return result;
		}

		long FileEraseClient::GetNStatusComplete() const
		{
			long value = 0;
			if (m_driver != INVALID_HANDLE_VALUE)
			{
				DWORD retLength = 0;
				if (!DeviceIoControl(m_driver, IOCTL_ERASE_LASTSTATUS, NULL, NULL, &value, sizeof(value), &retLength, NULL))
				{
					m_lastError = szi::SziDecodeError().DecodeError(::GetLastError(), L"NTDLL.DLL");
					value = -1;
				}
			} else
			{
				value = -1;
			}
			return value;
		}

		std::wstring FileEraseClient::GetStatusComplete() const
		{
			long status = GetNStatusComplete();
			std::wstring message;
			if (status == -1)
			{
				message = GetLastError();
			}
			else if (status != 0)
			{
				message = szi::SziDecodeError().DecodeError(status, L"NTDLL.DLL");
			}
			return message;
		}

		std::wstring FileEraseClient::GetStatusComplete(long ntStatus) const
		{
			std::wstring message;
			if(ntStatus == -1) 
			{
				message = GetLastError();
			} else if(ntStatus != 0) 
			{
				message = szi::SziDecodeError().DecodeError(ntStatus, L"NTDLL.DLL");
			}
			return message;
		}

		bool FileEraseClient::SetAutoErase(bool value)
		{
			DWORD lIsAutoErase = value ? 1 : 0;
			return szi::SziRegistry::SetValue<DWORD>(szi::SziRegistry::Hklm, REGAUTOERASEKEY, REGAUTOERASEVALUENAME, lIsAutoErase);
		}
	} //drv
}//sudis