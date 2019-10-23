//
// Burlutsky Stanislav
// burluckij@gmail.com
//

#pragma once

#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

#include <mutex>
#include <string>

#define TEMP_BUFFER_SIZE 1024

#define __and__ ,
#define LOGPRINT(_log, _formatstr, _values) \
	wsprintfA(_log.acquireBuffer(), _formatstr, _values); _log.leaveBuffer();

#define LOGPRINTW(_log, _formatstr, _values) \
	wsprintfW(_log.acquireBuffer(), _formatstr, _values); _log.leaveBuffer();


namespace app_log
{
	//
	// Default limit is 20 MB.
	//
	const unsigned long FileSizeLimit = 1024 * 1024 * 20;

	enum FileSizePolicy
	{
		OVERWRITE,
		ACCEPT_INCREASE
	};
}

class logfile
{
public:
	explicit logfile(std::string file,
	                 bool use_automatical_closing = true,
	                 ULONG temp_buffer_size = TEMP_BUFFER_SIZE,
	                 ULONG log_file_size_limit = app_log::FileSizeLimit,
	                 app_log::FileSizePolicy size_policy = app_log::ACCEPT_INCREASE) :
		m_hFile(INVALID_HANDLE_VALUE),
		m_logFilePath(file),
		m_tempBufferSize(temp_buffer_size),
		m_tempBuffer(NULL),
		m_isBufferLocked(false),
		m_logSizePolicy(size_policy),
		m_logFileSizeLimit(log_file_size_limit),
		m_isCloseFileEachTime(use_automatical_closing)
	{
		if (!m_isCloseFileEachTime)
		{
			HANDLE hfile = open(file.c_str());
			if (hfile != INVALID_HANDLE_VALUE)
			{
				m_hFile = hfile;
			}
		}

		InitializeCriticalSection(&m_lockBuffer);
		InitializeCriticalSection(&m_lockFile);

		if (!temp_buffer_size)
		{
			m_tempBufferSize = TEMP_BUFFER_SIZE;
		}

		m_tempBuffer = new char[m_tempBufferSize];
	}

// 	logfile(std::wstring _file,
// 		bool use_automatical_closing = true,
// 		ULONG temp_buffer_size = TEMP_BUFFER_SIZE,
// 		ULONG log_file_size_limit = app_log::FileSizeLimit,
// 		app_log::FileSizePolicy size_policy = app_log::ACCEPT_INCREASE) :
// 		logfile(strings::ws_s(_file), use_automatical_closing, temp_buffer_size, log_file_size_limit, size_policy) { }

	~logfile() throw()
	{
		if (m_hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFile);
		}

		DeleteCriticalSection(&m_lockBuffer);
		DeleteCriticalSection(&m_lockFile);
		delete[] m_tempBuffer;
	}

	bool isLogFileAvailable() const throw()
	{
		return (m_hFile != INVALID_HANDLE_VALUE) && (m_hFile != nullptr);
	}

	//
	// Returns buffer state, locked - true, unlocked - false.
	//
	bool isBufferLocked() const throw()
	{
		return m_isBufferLocked;
	}

	//
	// Acquires lock and returns pointer to temporary buffer.
	//
	char* acquireBuffer() throw()
	{
		EnterCriticalSection(&m_lockBuffer);
		m_isBufferLocked = true;
		ZeroMemory(m_tempBuffer, m_tempBufferSize);
		return m_tempBuffer;
	}

	//
	// Leaves critical section which protects temporary buffer.
	//
	void leaveBuffer(bool to_flush_buf = true) throw()
	{
		if (to_flush_buf)
		{
			flushBuffer();
		}
		LeaveCriticalSection(&m_lockBuffer);
		m_isBufferLocked = false;
	}

	ULONG getBufferSize() const throw()
	{
		return m_tempBufferSize;
	}

	bool resetBuffer(ULONG new_size) throw()
	{
		char* p = new(std::nothrow) char[new_size];
		if (p)
		{
			acquireBuffer();
			delete[] m_tempBuffer;

			m_tempBuffer = p;
			m_tempBufferSize = new_size;
			leaveBuffer();
		}
		return p != NULL;
	}

	//
	// Writes in log all data from temporary buffer.
	// You should use this function only between two calls acquireBuffer() and leaveBuffer().
	// This function doesn't try to enter in critical section.
	//
	bool flushBuffer() throw()
	{
		bool flushed = false;
		if (m_isBufferLocked)
		{
			flushed = print(m_tempBuffer);
		}
		return flushed;
	}

	std::string getLogFilePath() const throw()
	{
		return m_logFilePath;
	}

	ULONG getFileSizeLimit() const throw()
	{
		return m_logFileSizeLimit;
	}

	//
	// Adds to log last error code from WinAPI call.
	//
	bool printLastErrorCode(std::string inf_msg) throw()
	{
		char number[32];
		wsprintfA(number, "%d", GetLastError());
		std::string s = std::string("Last error code: ") + std::string(number) + std::string("  ") + inf_msg;
		return this->print(s);
	}

	//
	// Flushes buffer and closes descriptor.
	//
	void close()
	{
		EnterCriticalSection(&m_lockFile);
		if (m_hFile != INVALID_HANDLE_VALUE)
		{
			FlushFileBuffers(m_hFile);
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
		LeaveCriticalSection(&m_lockFile);
	}
	
	template<typename... T>
	inline bool printEx(const char* Format, T ... Arg) {
		char lBuffer[1024] = {0};
		sprintf_s(lBuffer, sizeof(lBuffer), Format, std::forward<T>(Arg)...);
		return print(lBuffer, true);
	}

	bool WINAPI print(std::string msg, bool write_console = true) throw()
	{
		return print(msg.c_str(), write_console);
	}

    bool printError(std::string msg, bool write_console = true)
    {
        return print("Error: " + msg, write_console);
    }

    bool printInfo(std::string msg, bool write_console = true)
    {
        return print("Info: " + msg, write_console);
    }

    bool printWarning(std::string msg, bool write_console = true)
    {
        return print("Warning: " + msg, write_console);
    }

	bool WINAPI print(const char* writeData, bool _writeConsole = true) throw()
	{
		SYSTEMTIME systime = {0};
		BOOL result = FALSE;
		DWORD written = 0;
		char systemTime[128] = {0};

		if (!writeData)
		{
			return false;
		}

		GetLocalTime(&systime);
		wsprintfA(systemTime, "%02d.%02d.%d : %02d:%02d:%02d:%03d ms (pid %d tid %d)  ",
			systime.wDay, systime.wMonth, systime.wYear,
			systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds,
			GetCurrentProcessId(), GetCurrentThreadId());

		std::string outputLine = std::string(systemTime) + writeData + "\r\n";

		EnterCriticalSection(&m_lockFile);
		{
			if (m_hFile == INVALID_HANDLE_VALUE)
			{
				//
				// Try to open a log file.
				//
				m_hFile = open(m_logFilePath.c_str());
				if (m_hFile == INVALID_HANDLE_VALUE)
				{
					LeaveCriticalSection(&m_lockFile);
					return false;
				}
			}

			// Overwrite data if it need.
			if ((ULONG)GetFileSize(m_hFile, 0) >= m_logFileSizeLimit)
			{
				if (!reset())
				{
					LeaveCriticalSection(&m_lockFile);
					return false;
				}
			}

			//
			// Logging.
			//
			SetFilePointer(m_hFile, 0, 0, FILE_END);

			result = WriteFile(m_hFile, outputLine.c_str(), (DWORD)outputLine.size(), &written, nullptr);

			FlushFileBuffers(m_hFile);
			//
			// End logging.
			//

			if (m_isCloseFileEachTime)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}
		}
		LeaveCriticalSection(&m_lockFile);

		if (_writeConsole)
		{
			printf("%s : %s\n", systemTime, writeData);
		}

		return result == TRUE;
	}

private:
	volatile HANDLE m_hFile;
	std::string m_logFilePath;

	CRITICAL_SECTION m_lockBuffer;
	CRITICAL_SECTION m_lockFile;

	ULONG m_logFileSizeLimit;
	app_log::FileSizePolicy m_logSizePolicy;

	ULONG m_tempBufferSize;
	char* m_tempBuffer;
	volatile bool m_isBufferLocked;
	volatile bool m_isCloseFileEachTime;

	//
	// Opens already created log file otherwise creates a new log file.
	//
	HANDLE WINAPI open(const char* szFilePath) throw()
	{
		HANDLE hFile = INVALID_HANDLE_VALUE;
		DWORD crflags = CREATE_NEW;

	OPEN_EXISTING_FILE:
		hFile = CreateFileA(szFilePath,
		                    GENERIC_READ | GENERIC_WRITE,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE,
		                    NULL,
		                    crflags,
		                    0,
		                    NULL);

		if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_FILE_EXISTS))
		{
			crflags = OPEN_EXISTING;
			goto OPEN_EXISTING_FILE;
		}

		return hFile;
	}

	bool reset() throw()
	{
		bool result = false;
		switch (m_logSizePolicy)
		{
		case app_log::OVERWRITE:
			if (m_hFile && (m_hFile != INVALID_HANDLE_VALUE))
			{
				CloseHandle(m_hFile);
			}

			if (DeleteFileA(m_logFilePath.c_str()))
			{
				m_hFile = open(m_logFilePath.c_str());
				result = m_hFile != INVALID_HANDLE_VALUE;
			}
			break;

		case app_log::ACCEPT_INCREASE:
			result = true;
			break;

		default:
			break;
		}

		return result;
	}

	//
	// Forget about copy-constructors.
	//
	logfile(const logfile& rhs);
	logfile& operator=(const logfile& rhs);
};
