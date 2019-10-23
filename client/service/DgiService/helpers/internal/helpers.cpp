//
//	Authors: 
//			burluckij@gmail.com
//			p.obrosov
//

#define _WIN32_WINNT 0x0600

#include "helpers.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#undef _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <algorithm>
#include <TlHelp32.h>
#include <algorithm>
#include <Psapi.h>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <ostream>
#include <unordered_map>
#include <iphlpapi.h>
#include <Sddl.h>
#include <chrono>
#include <ctime>
#include <iomanip>

// #include "user manager/userManagerHelpers.h"

#pragma comment(lib, "psapi")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

namespace process
{
	std::string getFilePath(DWORD _pid)
	{
		std::string path;
		char process_image[1024] = { 0 };

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ /*PROCESS_ALL_ACCESS*/, TRUE, _pid);
		if (hProcess != NULL)
		{
			size_t buffer_lenght = GetModuleFileNameExA(hProcess, NULL, process_image, sizeof(process_image));
			if (GetLastError() == ERROR_SUCCESS)
			{
				path = process_image;
			}
			else
			{
				ZeroMemory(process_image, sizeof(process_image));
				buffer_lenght = GetProcessImageFileNameA(hProcess, process_image, sizeof process_image);
				if (buffer_lenght != 0 /*GetLastError() == ERROR_SUCCESS*/)
				{
					path = process_image;
				}
			}
			CloseHandle(hProcess);
		}

		return path;
	}

//#ifndef UNICODE

	bool find(std::string _name,
		bool _fullMatch,
		std::string& _procImageFilePath,
		DWORD& _pid)
	{
		DWORD pid = 0;
		bool result = false;
		HANDLE hProcessSnap;
		PROCESSENTRY32 pe32;

		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (!Process32First(hProcessSnap, &pe32))
		{
			CloseHandle(hProcessSnap);
			return false;
		}

		// use only same-case.
		strings::toLower(_name);
		std::wstring name = strings::s_ws(_name);

		do
		{
			std::wstring procname = pe32.szExeFile;
			strings::toLower(procname);

			if (_fullMatch)
			{
				if (strings::equalStrings(name, procname))
				{
					result = true;
				}
			}
			else if (procname.find(name) != std::string::npos)
			{
				result = true;
			}

			if (result)
			{
				_pid = pe32.th32ProcessID;
				_procImageFilePath = getFilePath(_pid);
				// 				if (_procName.empty())
				// 				{
				// 					_procName = procname;
				// 				}
				result = true;
				break;
			}
		} while (Process32Next(hProcessSnap, &pe32));
		CloseHandle(hProcessSnap);
		return result;
	}

//#endif
}

namespace timefn
{
	int getTimeoutValue(const SYSTEMTIME& _scheduledTime, unsigned int _waitPeriod)
	{
		SYSTEMTIME current;
		GetLocalTime(&current);

		int n = (_scheduledTime.wHour * 60 + _scheduledTime.wMinute) - (current.wHour * 60 + current.wMinute);

		// Time has passed, We have to calculate new time interval 
		if (n < 0)
		{
			// Time is come to do something..
			if (_waitPeriod == 0)
			{
				return 30;
			}
			if ((abs(n) % _waitPeriod) == 0)
			{
				return 0;
			}

			int timePast = abs(n) % _waitPeriod;
			int timeout = _waitPeriod - timePast;
			return timeout;
		}
		else if (n > 0)
		{
			// Time isn't come. Wait all the time.
			return n;
		}

		return 0;
	}
	
	std::string getCurrentTime()
	{
		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		return getTime(st);
	}

	std::string getTime(const SYSTEMTIME& _time)
	{
		char time[256] = { 0 };
		wsprintfA(time, "%d:%d:%d %d.%d.%d",
			_time.wHour, _time.wMinute, _time.wSecond,
			_time.wDay, _time.wMonth, _time.wYear);

		return time;
	}

	// get timestamp in milliseconds since unix epoch
	int64_t getTimestamp()
	{
		auto now = std::chrono::system_clock::now();
		auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
		auto epoch = now_ms.time_since_epoch();
		auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

		return value.count();														// timestamp with milliseconds
	}

}

namespace network
{
	int sendAll(int s, const char* buf, int len, int flags)
	{
		int total = 0, n = 0;

		while (total < len)
		{
			n = send(s, buf + total, len - total, flags);
			if (n == SOCKET_ERROR)
			{
				break;
			}
			total += n;
		}

		return (n == SOCKET_ERROR ? NETWORK_ERROR : total);
	}

	int recvAll(int s, char* buf, int len, int flags)
	{
		int total = 0, n = -1;

		while (total < len)
		{
			n = recv(s, buf + total, len - total, flags);
			if (n == 0) // connection closed.
			{
				break;
			}
			else if (n < 0)
			{
				// ignore all errors.
				break;
			}
			total += n;
		}

		return (n == -1 ? -1 : total);
	}

	// This method succeeds only when all data was read.
	bool read(int _hSocket, std::string& _outBuf, int length)
	{
		char buf[4096];
		std::string readData;

		while (readData.size() != length) // Read until all data is collected.
		{
			// If need read more than 4k bytes data, devide it on a several steps. 
			int cbAvailableToRead = (length - readData.size()) >= sizeof(buf) ? sizeof(buf) : (length - readData.size()) /*length*/;

			if (cbAvailableToRead)
			{
				int rv = recv(_hSocket, buf, cbAvailableToRead, 0);
				if (rv == 0)
				{ // Connection closed.
					return false; // Stops only when connection is closed.
				}
				else if (rv < 0)
				{
					return false; // Error happened.
				}

				// Append just read bytes to result string.
				std::string tmp;
				tmp.assign(buf, rv);
				readData += tmp;
			}
		}

		_outBuf.swap(readData);
		return true;
	}
}

namespace memory
{
	PVOID getmem(size_t x)
	{
		return (PVOID)((char*)new(std::nothrow) char[x]);
	}

	void freemem(PVOID x)
	{
		if (x)
		{
			char* p = (char*)x;
			delete[] p;
		}
	}
}

namespace winerror
{
	// DWORD errDw = GetLastError(); shold be called before
	std::string getLastErrorCodeMessage(DWORD _dwErrCode)
	{
		char* messageBuffer = nullptr;

		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			//FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			_dwErrCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPSTR)&messageBuffer,
			0,
			NULL);

		char szErrorCode[32] = { 0 };
		//_itoa(_dwErrCode, szErrorCode, 10);
		wsprintfA(szErrorCode, "%d", _dwErrCode);
		std::string resMess(messageBuffer);
		LocalFree(messageBuffer); 

		if (size = 0)
		{
			resMess = "No description for the error code.";
		}
		return "(" + std::string(szErrorCode) + "): " + resMess;
	}
}

namespace windir
{
	void getDirStructRec(std::wstring _path, DirEntry& _outStructure)
	{
		removeLastSeparator(_path);

		// Save name of the current directory
		//
		_outStructure.currPath = _path;

		std::map<std::wstring, bool> dirContent;

		//getDirHierarchy(_path, dirContent);
		getOneDirContent(_path, dirContent);

		for (auto dirObject : dirContent)
		{
			if (dirObject.second == true)
			{
				//
				// Add file to the file list.
				//
				_outStructure.files.push_back(dirObject.first);
			}
			else
			{
				//
				// Add new directory.
				//
				std::pair<std::map<std::wstring, windir::DirEntry>::iterator, bool> i = _outStructure.sub_dirs.insert(std::pair<std::wstring, windir::DirEntry>(dirObject.first, DirEntry()));

				if (i.first != _outStructure.sub_dirs.end())
				{
					DirEntry& de = i.first->second;

					//
					// Build hierarchy for sub folder.
					//
					getDirStructRec(dirObject.first, de);
				}
			}
		}
	}

	bool remove(std::wstring _dirpath, DWORD& _win32ErrorCode)
	{
		windir::removeLastSeparator(_dirpath);

		// "\\?\" + _dirpath - can extend limit to the default file path length.
		//

		BOOL success = RemoveDirectoryW(_dirpath.c_str());
		
		if (!success)
		{
			_win32ErrorCode = GetLastError();
		}

		return success == TRUE;
	}

	bool setImageDirAsCurrent()
	{
		bool success = false;
		char path[1024] = { 0 };
		if (GetModuleFileNameA(NULL, path, sizeof(path)))
		{
			std::string fullFilePath(path);
			auto pos = fullFilePath.find_last_of('\\');
			if (pos != std::string::npos)
			{
				auto newDir = fullFilePath.substr(0, pos);
				if (!newDir.empty())
				{
					success = SetCurrentDirectoryA(newDir.c_str()) == TRUE;
				}
			}
		}
		return success;
	}

	void removeLastSeparator(__inout std::string& _s)
	{
		return boost::trim_right_if(_s, boost::is_any_of("\\"));
	}

	void removeLastSeparator(__inout std::wstring& _s)
	{
		return boost::trim_right_if(_s, boost::is_any_of(L"\\"));
	}

	bool deleteFile(std::wstring _filepath)
	{
		if (!_filepath.empty())
		{
			return DeleteFileW(_filepath.c_str()) == TRUE;
		}

		return false;
	}

	long getNamesOfObjInDir(
		std::set<std::wstring>& _files,
		const std::wstring& _rootDirectory,
		const std::wstring& _filesExtension,
		bool _bCountSubdirectories,
		bool _bSearchSubdirectories,
		logfile& _log,
		const int _nObjTypes
	)
	{
		std::wstring strPattern; // Pattern
		std::wstring strFilePath; // Filepath
		std::wstring strExtension; // Extension
		HANDLE hFile; // Handle to file
		WIN32_FIND_DATAW fileInfo; // File information

		strPattern = _rootDirectory;
		if (_rootDirectory.rfind(L'\\') != _rootDirectory.size())
		{
			//strPattern += '\\';
			strPattern.append(L"\\");
		}

		strPattern.append(L"*.*");

		hFile = ::FindFirstFileW(strPattern.c_str(), &fileInfo);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			_log.print(std::string(__FUNCTION__) + " error occured while FindFirstFileW(); strPattern: \'" + strings::ws_s(strPattern) + "\'");
			return -1;
		}
		else
		{
			do
			{
				if (fileInfo.cFileName[0] != L'.')
				{
					strFilePath = _rootDirectory;
					if (_rootDirectory.rfind(L'\\') != _rootDirectory.size())
					{
						strFilePath.append(L"\\");
					}

					strFilePath.append(fileInfo.cFileName);

					if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						if (((_nObjTypes == SEARCH_ONLY_DIRS) || (_nObjTypes == SEARCH_FILES_AND_DIRS)) && _bCountSubdirectories)
						{
							// Save Directory
							std::wstring strStarFilePath(L'*' + strFilePath); // Filepath with a STAR-symbol "*" at the beginning
							strings::toUpper(strStarFilePath); // !!! only for ICS
							_files.insert(strStarFilePath);
						}

						if (_bSearchSubdirectories)
						{
							// Search subdirectory
							long iRC = getNamesOfObjInDir(
								_files,
								strFilePath,
								_filesExtension,
								_bCountSubdirectories,
								_bSearchSubdirectories,
								_log,
								_nObjTypes
							);

							if (iRC)
							{
								_log.print(std::string(__FUNCTION__) + " error - can't get subobjects from: strFilePath: \'" + strings::ws_s(strFilePath) + "\', \'" +
									" filesExtension: \'" + strings::ws_s(_filesExtension) + "\'");
								continue;
							}
						}
					}
					else
					{
						if ((_nObjTypes == SEARCH_ONLY_FILES) || (_nObjTypes == SEARCH_FILES_AND_DIRS))
						{
							// Check extension ".exe" or any other
							if (_filesExtension.empty())
							{
								// Save Filename
								strings::toUpper(strFilePath); // !!! only for ICS
								_files.insert(strFilePath);
							}
							else
							{
								strExtension = fileInfo.cFileName;
								strExtension = strExtension.substr(strExtension.rfind(L".") + 1);

								if (strExtension == _filesExtension)
								{
									// Save Filename with specific extension
									strings::toUpper(strFilePath); // !!! only for ICS
									_files.insert(strFilePath);
								}
							}
						}
					}

					strFilePath.erase();
				}
			}
			while (::FindNextFileW(hFile, &fileInfo));

			DWORD dwError = ::GetLastError();
			if (dwError != ERROR_NO_MORE_FILES)
			{
				std::wcout << L"FindNextFile died for some reason; path = " << strFilePath << std::endl;
				return dwError;
			}

			FindClose(hFile);
		}

		return 0;
	} 

	bool isFilePresent(std::string _filePath)
	{
		return isFilePresent(strings::s_ws(_filePath));
	}

	bool isFilePresent(std::wstring _filePath)
	{
		WIN32_FIND_DATAW fileInfo;
		HANDLE hFile = ::FindFirstFileW(_filePath.c_str(), &fileInfo);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFile);
			return true;
		}
		return false;
	}

	bool isDiskRootPath(std::wstring _path)
	{
		removeLastSeparator(_path);

		if (!_path.empty())
		{
			// "D:" - thats what we wait to see.
			// "NetworkDrive:"
			// 
			// Last symbol should be - ':'
			//

			if (_path.at(_path.length() - 1) == ':')
			{
				return true;
			}
		}

		return false;
	}

	bool getFileByteSize(std::string _filePath, size_t& sz)
	{
		return getFileByteSize(strings::s_ws(_filePath), sz);
	}

	bool getFileByteSize(std::wstring _filePath, size_t& sz)
	{
		bool result = false;
		HANDLE hFile = CreateFileW(_filePath.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER file_size = { 0 };
			result = (TRUE == GetFileSizeEx(hFile, &file_size));
			
			if (result)
			{
				sz = static_cast<size_t>(file_size.QuadPart);
			}

			CloseHandle(hFile);
		}

		return result;
	}

	bool getRootFsObjects(std::vector<std::wstring>& _out)
	{
		_out.clear();
		auto bufLen = GetLogicalDriveStringsW(0, nullptr);
		if (bufLen < 1)
			return false;
		std::vector<wchar_t> outData(bufLen, L'\0');
		bufLen = GetLogicalDriveStringsW(bufLen, outData.data());
		strings::splitString(std::wstring(outData.begin(), outData.end()), std::wstring(1, L'\0'), _out);
		return true;
	}

	std::wstring getCurrentDir()
	{
		wchar_t currentDir[1024] = {0};
		GetCurrentDirectoryW(sizeof currentDir, currentDir);
		std::wstring currentDirectory(currentDir);
		removeLastSeparator(currentDirectory);
		return currentDirectory;
	}

	std::wstring getTempDir()
	{
		const int string_size = 1024;
		wchar_t tmp[string_size]{};
		GetTempPathW(string_size, tmp);

		std::wstring strTmpDir(tmp);
		removeLastSeparator(strTmpDir);
		return strTmpDir;
	}

	std::wstring getImageDir()
	{
		std::wstring path;
		wchar_t szpath[1024] = { 0 };

		if (GetModuleFileNameW(NULL, szpath, sizeof(szpath)))
		{
			path = szpath;
			auto pos = path.find_last_of(L'\\');
			if (pos != std::string::npos)
			{
				path = path.substr(0, pos);
				removeLastSeparator(path);
			}
		}

		return path;
	}

	std::wstring getImageFilePath()
	{
		wchar_t szpath[1024] = { 0 };

		if (GetModuleFileNameW(NULL, szpath, sizeof(szpath)))
		{
			return szpath;
		}

		return L"";
	}

	bool create(std::wstring _path)
	{
		std::vector<std::wstring > dirs;
		auto pos = _path.find_first_of(L'\\'); // x:\folder\sub\dir
		if (pos != std::string::npos)
		{
			auto rootPath = _path.substr(0, pos); // x: 
			auto pathWithoutRoot = _path.substr(pos); // folder\sub\dir

			split(dirs, pathWithoutRoot, boost::is_any_of(L"\\")); // dirs contains: folder, sub, dir.

			// Выстроить весь путь - x:\folder, x:\folder\sub, x:\folder\sub\dir.
			for (auto dirEntry : dirs)
			{
				if (!dirEntry.empty())
				{
					rootPath = rootPath + L'\\' + dirEntry;
					if(!CreateDirectoryW((LPCWSTR)rootPath.c_str(), NULL))
					{
						if (GetLastError() != ERROR_ALREADY_EXISTS)
						{
							return false;
						}
					}
				}
			}
		} else
		{
			return false;
		}

		return true;
	}

	bool createDirForFile(std::wstring _filePath)
	{
		bool result = false;

		auto pos = _filePath.find_last_of(L'\\'); // x:\folder\sub\dir
		if ( !_filePath.empty() &&  (pos != std::string::npos) )
		{
			auto path = _filePath.substr(0, pos);
			result = create(path);
		}

		return result;
	}

	std::wstring getDirForFile(std::wstring _filePath)
	{
		auto pos = _filePath.find_last_of(L'\\'); // x:\folder\sub\dir

		if (!_filePath.empty() && (pos != std::string::npos))
		{
			return _filePath.substr(0, pos);
		}

		return L"";
	}

	bool getDirHierarchy(std::wstring _path, std::map<std::wstring, bool>& _hierarchy)
	{
		std::set<std::wstring> res;
		WIN32_FIND_DATAW fileInfo;

		removeLastSeparator(_path);
		_path += L"\\";

		auto searchPattern = _path + L"*.*";
		HANDLE hFile = FindFirstFileW(searchPattern.c_str(), &fileInfo);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fileInfo.cFileName[0] != L'.')
				{
					if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						_hierarchy[_path + fileInfo.cFileName] = false;
						getDirHierarchy(_path + fileInfo.cFileName, _hierarchy);
					}
					else
					{
						_hierarchy[_path + fileInfo.cFileName] = true;
					}
				}

			} while (FindNextFileW(hFile, &fileInfo) == TRUE);

			FindClose(hFile);
		} else
		{
			return false;
		}

		return true;
	}

	bool getOneDirContent(std::wstring _path, std::map<std::wstring, bool>& _hierarchy)
	{
		std::set<std::wstring> res;
		WIN32_FIND_DATAW fileInfo;

		removeLastSeparator(_path);
		_path += L"\\";

		auto searchPattern = _path + L"*.*";
		HANDLE hFile = FindFirstFileW(searchPattern.c_str(), &fileInfo);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (fileInfo.cFileName[0] != L'.')
				{
					if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						_hierarchy[_path + fileInfo.cFileName] = false;
						//getDirHierarchy(_path + fileInfo.cFileName, _hierarchy);
					}
					else
					{
						_hierarchy[_path + fileInfo.cFileName] = true;
					}
				}

			} while (FindNextFileW(hFile, &fileInfo) == TRUE);

			FindClose(hFile);
		}
		else
		{
			return false;
		}

		return true;
	}

	std::wstring getWindowsDir()
	{
		wchar_t lBuffer[MAX_PATH] = { 0 };
		if(0 == GetWindowsDirectoryW(lBuffer, MAX_PATH))
		{
			return L"";
		}

		std::wstring wdir = lBuffer;
		windir::removeLastSeparator(wdir);
		return wdir;
	}

	std::wstring getFileName(std::wstring _filePath)
	{
		std::wstring name;

		boost::replace_all(_filePath, "/", "\\");

		auto pos = _filePath.find_last_of(L"\\/");

		if (pos != std::wstring::npos)
		{
			if (pos != _filePath.size())
			{
				return _filePath.substr(pos + 1);
			}
		}

		return L"";
	}

	std::string getFileName(std::string _filePath)
	{
		auto s = strings::s_ws(_filePath);
		return strings::ws_s(getFileName(s));
	}

	std::wstring performPath(std::wstring _newRoot, std::wstring _oldRoot, std::wstring _path)
	{
		strings::toLower(_oldRoot);
		strings::toLower(_path);

		boost::replace_first(_path, _oldRoot, _newRoot);

		return _path;
	}

	bool copy(std::wstring _from, std::wstring _to)
	{
		windir::removeLastSeparator(_to);
		windir::removeLastSeparator(_from);
		strings::toLower(_to);
		strings::toLower(_from);

		std::map< std::wstring, bool > entries;
		if(!::windir::getDirHierarchy(_from, entries))
		{
			return false;
		}

		// At first iteration - create all necessary directories.
		for (auto fsObject : entries)
		{
			if (fsObject.second == false) // means directory.
			{
				auto dir = fsObject.first;
				boost::replace_first(dir, _from, _to);
				windir::create(dir);
			}
		}

		// At the second iteration - copy all need files.
		for (auto fsObject : entries)
		{
			if (fsObject.second == true) // file.
			{
				auto newFileDest = fsObject.first;
				boost::replace_first(newFileDest, _from, _to);

				if (!CopyFileW(fsObject.first.c_str(), newFileDest.c_str(), FALSE))
				{
					return false;
				}
			}
		}

		return true;
	}

	bool getCurrentDiskFreeSpace(unsigned long long& freeSpace)
	{
		ULARGE_INTEGER lpFreeBytesAvailableToCaller;
		ULARGE_INTEGER lpTotalNumberOfBytes;
		ULARGE_INTEGER lpTotalNumberOfFreeBytes;
		
		const auto result = GetDiskFreeSpaceExW(
			nullptr,
			&lpFreeBytesAvailableToCaller,
			&lpTotalNumberOfBytes,
			&lpTotalNumberOfFreeBytes
		);

		freeSpace = lpTotalNumberOfFreeBytes.QuadPart;

		return (result == TRUE);
	}

	bool setFilePointerToBegin(HANDLE _hFile)
	{
		LARGE_INTEGER liPos, liNewPos;

		liPos.LowPart = 0;
		liPos.HighPart = 0;

		return SetFilePointerEx(_hFile, liPos, &liNewPos, FILE_BEGIN) == TRUE;
	}

}

namespace strings
{
	std::string ws_s(const std::wstring& s, UINT cp)
	{
		int slength = (int)s.length(); /* + 1*/;
		int len = WideCharToMultiByte(cp, 0, s.c_str(), slength, 0, 0, 0, 0);
		std::string r(len, '\0');
		WideCharToMultiByte(cp, 0, s.c_str(), slength, &r[0], len, 0, 0);
		return r;
	}

	std::wstring s_ws(const std::string& s, UINT cp)
	{
		int slength = (int)s.length(); /* + 1*/;
		int len = MultiByteToWideChar(cp, 0, s.c_str(), slength, 0, 0);
		std::wstring r(len, L'\0');
		MultiByteToWideChar(cp, 0, s.c_str(), slength, &r[0], len);
		return r;
	}

	void toUpper(std::string& _s)
	{
		//std::transform(_s.begin(), _s.end(), _s.begin(), toupper);
		boost::to_upper(_s, std::locale(""));
	}

	void toLower(std::string& _s)
	{
		//std::transform(_s.begin(), _s.end(), _s.begin(), tolower);
		boost::to_lower(_s, std::locale(""));
	}

	void toUpper(std::wstring& _s)
	{
		//std::transform(_s.begin(), _s.end(), _s.begin(), toupper);
		boost::to_upper(_s, std::locale(""));
	}

	void toLower(std::wstring& _s)
	{
		//std::transform(_s.begin(), _s.end(), _s.begin(), tolower);
		boost::to_lower (_s, std::locale(""));
	}

	// Compares strings without case match.
	bool equalStrings(std::string _s1, std::string _s2)
	{
		toLower(_s1);
		toLower(_s2);
		return _s1.compare(_s2) == 0;
	}

	bool equalStrings(std::wstring _s1, std::wstring _s2)
	{
		toLower(_s1);
		toLower(_s2);
		return _s1.compare(_s2) == 0;
	}

	// Добавляет _length байт к строке _str, читает из _p.
	void addBytes(__inout std::string& _str, const char* _p, std::size_t _length)
	{
		_str += std::string(_p, _length);
	}

	std::string generateUuid()
	{
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		return boost::lexical_cast<std::string>(uuid);
	}

	void splitString(const std::wstring& _in, const std::wstring& _delimiter, std::vector<std::wstring>& _out)
	{
        if (_delimiter.empty())
            return;
		_out.clear();
		//boost::split(_out, _in, boost::is_any_of(_delimiter)); // Works incorrect - adds empty string to vector if several L'\0' exists in a row
		size_t prev_pos = 0;
		size_t pos = 0;
		std::wstring substring;

		while (pos != std::wstring::npos)
		{
			pos = _in.find(_delimiter, prev_pos);
			substring.assign(_in.substr(prev_pos, pos - prev_pos));
			if (!substring.empty() && substring != std::wstring(1, L'\0')) // This verification distinguish this implementation from boost version
			{
				_out.push_back(substring + std::wstring(1, L'\0'));
			}
			prev_pos = pos + 1;
		}
	}


	void SerializeString(std::ostringstream& _os, const std::string& _inStr)
	{
		const size_t strSize = _inStr.size();
		_os.write(reinterpret_cast<const char*>(&strSize), sizeof(strSize));
		if (strSize == 0)
		{
			return;
		}
		_os.write(reinterpret_cast<const char*>(_inStr.data()), strSize);
	}

	void DeserializeString(std::istringstream& _is, std::string& _outStr)
	{
		size_t strSize;
		_is.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
		std::vector<char> vData(strSize);
		if (strSize > 0)
		{
			_is.read(vData.data(), strSize);
		}
		_outStr.assign(vData.begin(), vData.end());
	}

	void SerializeWString(std::ostringstream& _os, const std::wstring& _inStr)
	{
		auto inStr = strings::ws_s(_inStr, CP_UTF8);
		SerializeString(_os, inStr);
	}

	void DeserializeWString(std::istringstream& _is, std::wstring& _outStr)
	{
		std::string outStr;
		DeserializeString(_is, outStr);
		_outStr.assign(strings::s_ws(outStr, CP_UTF8));
	}

	void WStringToBuffer(const std::wstring& _inStr, std::vector<char>& _outBuf)
	{
		auto inStr = strings::ws_s(_inStr, CP_UTF8);
		_outBuf.assign(inStr.begin(), inStr.end());
	}

	std::wstring BufferToWString(const std::vector<char>& _inBuf)
	{
		std::wstring result;
		std::string str(_inBuf.begin(), _inBuf.end());
		result.assign(strings::s_ws(str, CP_UTF8));
		return result;
	}
}

namespace cpu
{
	bool IsCPUx64()
	{
		SYSTEM_INFO si = {0};
		GetNativeSystemInfo(&si);
		return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
	}
}

namespace system_power
{
	bool reboot(std::wstring _message, unsigned long _timeoutSeconds, bool _closeAllImmediately)
	{
		const wchar_t* ptrMsg = nullptr;

		if (!_message.empty())
		{
			ptrMsg = _message.c_str();
		}

		return InitiateSystemShutdownW(NULL, const_cast<wchar_t*>(ptrMsg), _timeoutSeconds, _closeAllImmediately, TRUE) == TRUE;
	}
}

namespace registry
{
	HKEY g_RegRootHandles[] = {
		HKEY_CLASSES_ROOT,
		HKEY_CURRENT_CONFIG,
		HKEY_CURRENT_USER,
		HKEY_LOCAL_MACHINE,
		HKEY_USERS
	};

	const char* g_RegRoot[] = {
		"HKEY_CLASSES_ROOT",
		"HKEY_CURRENT_CONFIG",
		"HKEY_CURRENT_USER",
		"HKEY_LOCAL_MACHINE",
		"HKEY_USERS"
	};

	const wchar_t* g_RegRootW[] = {
		L"HKEY_CLASSES_ROOT",
		L"HKEY_CURRENT_CONFIG",
		L"HKEY_CURRENT_USER",
		L"HKEY_LOCAL_MACHINE",
		L"HKEY_USERS"
	};

	std::string GetKeyPathWithoutRootDir(const std::string& _key, HKEY& _hRootDir)
	{
		std::wstring keypath, key = strings::s_ws(_key);
		keypath = GetKeyPathWithoutRootDir(key, _hRootDir);
		return strings::ws_s(keypath);
	}

	std::wstring GetKeyPathWithoutRootDir(const std::wstring& _key, HKEY& _hRootDir)
	{
		std::wstring result;
		std::size_t pos = std::wstring::npos;
		int i = 0;
		for (i = 0; i < sizeof(g_RegRootW) / sizeof(const wchar_t*); ++i)
		{
			pos = _key.find(g_RegRootW[i]);
			if (pos != std::wstring::npos)
			{
				break;
			}
		}

		if (pos != std::wstring::npos)
		{
			if (std::wstring(g_RegRootW[i]).length() + 1 < _key.length())
			{
				result = _key.substr(std::wstring(g_RegRootW[i]).length() + 1 /* \ */);
			}
			else
			{
				result = L"";
			}

			_hRootDir = g_RegRootHandles[i];
		}

		return result;
	}

	bool getRootKeys(std::vector<std::wstring>& _out)
	{
		_out.clear();

		for (auto i = 0; i < sizeof(g_RegRootW) / sizeof(const wchar_t*); ++i)
			_out.push_back(g_RegRootW[i]);

		return true;
	}

	bool getChildKeys(const std::wstring& _parentKey, 
					  std::vector<std::pair<RegistryTypes, std::wstring>>& _out,
					  int& _numKeys,
					  int& _numValues)
	{
		_out.clear();

		HKEY openedHKey;
		auto err = OpenKey(_parentKey, openedHKey, 0, KEY_READ | KEY_ENUMERATE_SUB_KEYS);
		if (ERROR_SUCCESS != err)
		{
			return false;
		}

		// Get information about number of keys, values, their max size names, etc
		DWORD lpcSubKeys;				// number of subkeys
		DWORD lpcMaxSubKeyLen;			// size of the key's subkey with the longest name, in Unicode characters
		DWORD lpcValues;				// number of values
		DWORD lpcMaxValueNameLen;		// size of the key's longest value name, in Unicode characters
		DWORD lpcMaxValueLen;           // size of the longest data component among the key's values (What is this??!)
		DWORD lpcbSecurityDescriptor;
		err = RegQueryInfoKeyW(openedHKey, 
							   nullptr, 
							   nullptr, 
							   nullptr, 
							   &lpcSubKeys, 
							   &lpcMaxSubKeyLen, 
							   nullptr, 
							   &lpcValues, 
							   &lpcMaxValueNameLen, 
							   &lpcMaxValueLen, 
							   &lpcbSecurityDescriptor, 
							   nullptr);

		if (ERROR_SUCCESS != err)
		{
			RegCloseKey(openedHKey);
			return false;
		}

		_numKeys = lpcSubKeys;
		_numValues = lpcValues;

		// reserving buffers
		std::vector<wchar_t> subkeyName(lpcMaxSubKeyLen + 1, '\0');     // lpcMaxSubKeyLen not including null terminating character, so we add it
		std::vector<wchar_t> valueName(lpcMaxValueNameLen + 1, '\0');

		// collect all subkeys
		for (decltype(lpcSubKeys) i = 0; i < lpcSubKeys; ++i)
		{
			DWORD lpcName = subkeyName.size();     // size of the buffer in characters (in|out)
			err = RegEnumKeyExW(openedHKey, i, subkeyName.data(), &lpcName, nullptr, nullptr, nullptr, nullptr);
			if (err != ERROR_SUCCESS)
			{
				RegCloseKey(openedHKey);
				return false;
			}
			std::wstring keyName(subkeyName.begin(), subkeyName.begin() + lpcName);
			_out.push_back(std::make_pair(RegistryTypes::RegKey, _parentKey + L"\\" + keyName));
		}

		// enumerates by values - collect all values
		for (decltype(lpcValues) i = 0; i < lpcValues; ++i)
		{
			DWORD lpcchValueName = valueName.size();
			err = RegEnumValueW(openedHKey, i, valueName.data(), &lpcchValueName, nullptr, nullptr, nullptr, nullptr);
			if (err != ERROR_SUCCESS)
			{
				RegCloseKey(openedHKey);
				return false;
			}
			std::wstring keyName(valueName.begin(), valueName.begin() + lpcchValueName);
			_out.push_back(std::make_pair(RegistryTypes::RegValue, _parentKey + L"\\" + keyName));
		}
		
		RegCloseKey(openedHKey);
		return true;
	}

	bool SaveBinDataInRegistry(HKEY _hKey, std::string _valueName, std::string& _data)
	{
		LONG status = RegSetValueExA(_hKey, _valueName.c_str(), 0, REG_BINARY, (LPBYTE)_data.data(), _data.size());
		return status == ERROR_SUCCESS;
	}

	bool ReadBinDataFromRegistry(HKEY _hKey, std::string _valueName, std::string& _data)
	{
		bool success = false;
		DWORD value_type = 0;
		std::string value;
		BYTE valueBuf[64] = { 0 };
		DWORD valueSize = 0;

		LSTATUS status = RegQueryValueExA(_hKey, _valueName.c_str(), 0, 0, (LPBYTE)valueBuf, &valueSize);
		if (status == ERROR_MORE_DATA)
		{
			char* p = new(std::nothrow) char[valueSize + 1];
			if (p)
			{
				status = RegQueryValueExA(_hKey, _valueName.c_str(), 0, &value_type, (LPBYTE)p, &valueSize);
				if (success = (status == ERROR_SUCCESS))
				{
					_data = std::string((char*)p, valueSize);
				}

				delete[] p;
			}
		}

		return success;
	}

	int OpenKey(const std::wstring& _key, HKEY& _result, DWORD _flag, DWORD _accessRights)
	{
		HKEY rootKey = 0;
		std::wstring keyPath = GetKeyPathWithoutRootDir(_key, rootKey);
		int errorCode = RegOpenKeyExW(rootKey, keyPath.c_str(), 0, _accessRights | _flag, &_result);
		if (errorCode != ERROR_SUCCESS)
		{
			return errorCode;
		}

		return ERROR_SUCCESS;
	}

	int TryToOpenKey(std::wstring _keyPath)
	{
		HKEY h;
		int res = OpenKey(_keyPath, h, cpu::IsCPUx64() ? KEY_WOW64_64KEY : 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_READ);
		if (res == ERROR_SUCCESS)
		{
			RegCloseKey(h);
		}
		return res;
	}

	long CreateKey(const std::wstring& _key, HKEY& _result, DWORD _flag, DWORD _accessRights)
	{
		HKEY rootKey = 0;
		DWORD disposition = 0;
		std::wstring keyPath = GetKeyPathWithoutRootDir(_key, rootKey);
		LONG result = RegCreateKeyExW(rootKey, keyPath.c_str(), 0, nullptr, 0/*REG_OPTION_NON_VOLATILE*/, _accessRights | _flag, nullptr, &_result, &disposition);
		return result;
	}

	bool isKeyExist(const std::wstring& _keypath)
	{
		HKEY h;
		bool result = false;
		int res = OpenKey(_keypath, h, cpu::IsCPUx64() ? KEY_WOW64_64KEY : 0, KEY_QUERY_VALUE | KEY_READ);
		if ( result = ( res == ERROR_SUCCESS) )
		{
			RegCloseKey(h);
		}

		return result;
	}

	bool removeValue(std::wstring _keyPath, std::wstring _value, DWORD _flag /*= 0*/)
	{
		HKEY h;
		bool result = false;
		int res = OpenKey(_keyPath, h, _flag, KEY_SET_VALUE);
		if (result = (res == ERROR_SUCCESS))
		{
			auto removedState = RegDeleteValueW(h, _value.c_str());
			result = (removedState == ERROR_SUCCESS);
			RegCloseKey(h);
		}

		return result;
	}

	bool setValue(const std::wstring& _keyPath, const std::wstring& _valueName, DWORD _flag, const std::vector<BYTE>& _data, const DWORD _type)
	{
		HKEY h;
		if (OpenKey(_keyPath, h, _flag, KEY_SET_VALUE) == ERROR_SUCCESS)
		{
			if (ERROR_SUCCESS == RegSetValueExW(h, _valueName.c_str(), 0, _type, _data.data(), _data.size()))
			{
				RegCloseKey(h);
				return true;
			}
			RegCloseKey(h);
		}
		return false;
	}

	bool enableWindowsRecycle(bool _toEnable)
	{
		HKEY h;
		int res = registry::OpenKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
			h, cpu::IsCPUx64() ? KEY_WOW64_64KEY : 0, KEY_SET_VALUE | KEY_READ);

		if (res == ERROR_SUCCESS)
		{
			DWORD dwValue = _toEnable ? 0 : 1;
			auto changed = RegSetValueExW(h, L"NoRecycleFiles", 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));
			RegCloseKey(h);

			return changed == ERROR_SUCCESS;
		}

		return false;
	}
}

namespace network
{
	//
	// Returns list of local IP addresses.
	// First entry is a main local IP.
	//
	std::vector<std::string> GetLocalIpAddrs()
	{
		std::vector<std::string> ips;
		char ac[128] = { 0 };
		if (gethostname(ac, sizeof(ac)) != SOCKET_ERROR)
		{
			// cout << "Host name is " << ac << "." << endl;
			struct hostent* phe = gethostbyname(ac);
			if (phe != 0)
			{
				for (int i = 0; phe->h_addr_list[i] != 0; ++i)
				{
					struct in_addr addr;
					memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
					ips.push_back(inet_ntoa(addr));
				}
			}
		}
		return ips;
	}

	std::vector<std::string> GetIpList1()
	{
		std::vector<std::string> ips;
		DWORD n = 1024 * 4;
		PIP_ADAPTER_INFO pIai = (PIP_ADAPTER_INFO)malloc(n);
		if (pIai == NULL)
		{
			return ips;
		}

		volatile DWORD result = GetAdaptersInfo(pIai, &n);
		if (result == ERROR_BUFFER_OVERFLOW)
		{
			free(pIai);
			pIai = (PIP_ADAPTER_INFO)malloc(n);
			if (!pIai)
			{
				return ips;
			}
		}

		result = GetAdaptersInfo(pIai, &n);
		if (result == ERROR_SUCCESS)
		{
			for (PIP_ADAPTER_INFO entry = pIai; entry; entry = entry->Next)
			{
				for (IP_ADDR_STRING* pIp = &entry->IpAddressList; pIp; pIp = pIp->Next)
				{
					IP_ADDRESS_STRING ip_v4 = pIp->IpAddress;
					std::string str_ipv4(ip_v4.String);
					if (str_ipv4 != "0.0.0.0")
					{
						ips.push_back(str_ipv4);
					}
				}
			}
		}

		free(pIai);
		return ips;
	}

	std::string GetCurrentIp()
	{
		std::vector<std::string> ips = GetLocalIpAddrs();
		if (!ips.empty())
		{
			return ips.at(0);
		}
		else
		{
			std::vector<std::string> _list = GetIpList1();
			if (!_list.empty())
			{
				return _list[0];
			}
		}
		return std::string();
	}

	std::vector<std::pair<std::string, std::string>> GetMacs()
	{
		std::vector<std::pair<std::string, std::string>> macs;
		IP_ADAPTER_INFO *info = NULL, *pos;
		DWORD size = 0;

		GetAdaptersInfo(info, &size);
		info = (IP_ADAPTER_INFO *)malloc(size);

		if (info)
		{
			GetAdaptersInfo(info, &size);
			for (pos = info; pos != NULL; pos = pos->Next)
			{
				char szmac[64] = { 0 };
				std::string macAddr;
				wsprintfA(szmac, "%2.2x", pos->Address[0]);
				macAddr = szmac;

				for (UINT i = 1; i < pos->AddressLength; i++)
				{
					ZeroMemory(szmac, sizeof szmac);
					wsprintfA(szmac, ":%2.2x", pos->Address[i]);
					macAddr += szmac;
				}

				macs.push_back(std::pair<std::string, std::string>(pos->Description, macAddr));
			}

			free(info);
		}
		return macs;
	}

	std::string GetMac()
	{
		return GetMacByIP(GetCurrentIp());
	}

	std::string GetMacByIP(std::string ip)
	{
		std::string mac;
		DWORD n = 1024 * 4;
		PIP_ADAPTER_INFO pIai = (PIP_ADAPTER_INFO)malloc(n);
		if (pIai == NULL)
		{
			return mac;
		}

		volatile DWORD result = GetAdaptersInfo(pIai, &n);
		if (result == ERROR_BUFFER_OVERFLOW)
		{
			free(pIai);
			pIai = (PIP_ADAPTER_INFO)malloc(n);
			if (!pIai)
			{
				return mac;
			}
		}

		result = GetAdaptersInfo(pIai, &n);
		if (result == ERROR_SUCCESS)
		{
			for (PIP_ADAPTER_INFO entry = pIai; entry; entry = entry->Next)
			{
				for (IP_ADDR_STRING* pIp = &entry->IpAddressList; pIp; pIp = pIp->Next)
				{
					IP_ADDRESS_STRING ip_v4 = pIp->IpAddress;
					std::string str_ipv4(ip_v4.String);

					if (str_ipv4 == ip)
					{
						char szmac[64] = { 0 };
						wsprintfA(szmac, "%2.2x", entry->Address[0]);
						mac = szmac;

						for (UINT i = 1; i < entry->AddressLength; i++)
						{
							ZeroMemory(szmac, sizeof szmac);
							wsprintfA(szmac, ":%2.2x", entry->Address[i]);
							mac += szmac;
						}

						break;
					}
				}
			}
		}

		free(pIai);
		return mac;
	}

	int createListenSocket(int _port)
	{
		int s;
		struct sockaddr_in addr;

		// AF_UNIX would be better for us.
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0)
		{
			return -1;
		}

		addr.sin_family = AF_INET;
		addr.sin_port = htons(_port);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			closesocket(s);
			return -1;
		}

		return s;
	}
}

namespace users
{
	bool getSidByUsername(std::wstring _domain, std::wstring _username, std::wstring& _outStrSid, SID& _outSid)
	{
		bool result = false;
		std::wstring accountName;

		//////////////////////////////////////////////////////////////////////////
		DWORD			cbSid = SECURITY_MAX_SID_SIZE;
		PSID			sid = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbSid);
		SID_NAME_USE	peUse = SidTypeUser;
		WCHAR			domainName[MAX_PATH * sizeof(wchar_t)] = L"\0";
		DWORD			cchReferencedDomainName = MAX_PATH;
		//////////////////////////////////////////////////////////////////////////

		if (_domain.empty())
		{
			accountName = L"\\" + _username;
		}
		else
		{
			accountName = _domain + L"\\" + _username;
		}

		auto success = LookupAccountNameW(NULL,accountName.c_str(),sid,&cbSid,domainName,&cchReferencedDomainName,&peUse);
		if (success == TRUE)
		{
			LPWSTR strSid = NULL;
			if (result = (ConvertSidToStringSidW(sid, &strSid) == TRUE))
			{
				_outSid = *((SID*)sid);
				_outStrSid = std::wstring(strSid);
				LocalFree(strSid);
				
			}
		}

		LocalFree(sid);
		return result;
	}

	bool getUsernameBySid(const std::wstring& _systemName, const std::wstring& _inSid, std::wstring& _outUsername)
	{
		wchar_t			username[MAX_PATH] = {0};
		DWORD			cbUsername = sizeof(username);
		wchar_t			referencedDomainName[MAX_PATH] = {0};
		DWORD			cchReferencedDomainName = sizeof(referencedDomainName);
		SID_NAME_USE	sidType;

		PSID userSid;
		if (!ConvertStringSidToSidW(_inSid.c_str(), &userSid))
		{
			return false;
		}

		const WCHAR* pSysName = _systemName.empty() ? NULL: _systemName.c_str();

		BOOL found = LookupAccountSidW(pSysName, userSid, username, &cbUsername, referencedDomainName, &cchReferencedDomainName, &sidType);
		if (found)
		{
			_outUsername = username;
		}

		LocalFree(userSid);
		return found == TRUE;
	}

}
