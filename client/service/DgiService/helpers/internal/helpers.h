//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//
//

#pragma once

#include <new>
#include <string>
#include <vector>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#include <set>
#include <map>
#include <ctime>
#include <stdint.h>
#include "log.h"
#include <lm.h>

#define NETWORK_ERROR		-1

#define COUNT_OF_CHARS(char_buffer) (sizeof(char_buffer) / sizeof(char_buffer[0]))
#define SET_LAST_ZERO_CHAR(char_buffer) (char_buffer[COUNT_OF_CHARS(char_buffer) - 1] = 0)

// Copies N wide-chars to new array and sets to zero last symbol in '_dest_array'. 
#define fill_wchars(_dest_array, _src_array)	\
	ZeroMemory(_dest_array, sizeof(_dest_array)); \
	wcsncpy(_dest_array, _src_array, COUNT_OF_CHARS(_dest_array) - 1);
	//wcsncpy(_dest_array, _src_array, COUNT_OF_CHARS(_dest_array));

//	wcscpy_s(_dest_array, COUNT_OF_CHARS(_dest_array) - 1, _src_array); \

// Copies N chars to new array and sets to zero last symbol in '_dest_array'. 
#define fill_chars(_dest_array, _src_array)	\
	ZeroMemory(_dest_array, sizeof(_dest_array)); \
	strncpy(_dest_array, _src_array, COUNT_OF_CHARS(_dest_array) - 1);
	// strncpy(_dest_array, _src_array, COUNT_OF_CHARS(_dest_array));

//strcpy_s(_dest_array, COUNT_OF_CHARS(_dest_array) - 1, _src_array);
	//_dest_array[COUNT_OF_CHARS(_dest_array) - 1] = 0;
	//memset(&_dest_array[COUNT_OF_CHARS(_dest_array) - 1], 0, sizeof(_dest_array[0]));

namespace process
{
	std::string getFilePath(DWORD _pid);

	bool find(std::string _name,
		bool _fullMatch,
		std::string& _procImageFilePath,
		DWORD& _pid);
}

namespace timefn
{
	int getTimeoutValue(const SYSTEMTIME& _scheduledTime, unsigned int _waitPeriod);
	std::string getCurrentTime();
	std::string getTime(const SYSTEMTIME& _time);

	// get timestamp in milliseconds since unix epoch
	int64_t getTimestamp();

	//std::string toDateStr(int64_t _timestamp);
}

namespace network
{
	int sendAll(int s, const char* buf, int len, int flags);
	int recvAll(int s, char* buf, int len, int flags);

	// Returns only two types values:
	// 0 - when connection closed.
	// len - count of read bytes.
	//int recvAll(int _hSocket, std::string& _outBuf, int len);

	// This method succeeds only when all data was read.
	bool read(int _hSocket, std::string& _outBuf, int len);

	//
	// Returns list of local IP addresses.
	// First entry is a main local IP.
	//
	std::vector<std::string> GetLocalIpAddrs();
	
	std::vector<std::string> GetIpList1();
	std::string GetCurrentIp();
	std::vector<std::pair<std::string, std::string>> GetMacs();
	std::string GetMac();
	std::string GetMacByIP(std::string ip);

	// Creates socket for local server.
	int createListenSocket(int _port);
}

namespace memory
{
	// Allocates bytes array with C++ operator - new [].
	// Returns NULL ptr if couldn't allocate memory.
	PVOID getmem(size_t x) throw();

	// Free memory function.
	// x - could be a NULL pointer.
	void freemem(PVOID x);
}

namespace winerror
{
	// DWORD _dwErrCode = GetLastError(); shold be called before
	std::string getLastErrorCodeMessage(DWORD _dwErrCode);
}

namespace windir
{
	struct DirEntry
	{
		std::wstring currPath;
		std::vector<std::wstring> files;
		std::map<std::wstring /* directory name */, DirEntry /* directory content */> sub_dirs;
	};

	//
	// Builds directory hierarchy recursively.
	//
	void getDirStructRec(std::wstring _path, DirEntry& _outStructure);

	//
	// Removes windows directory.
	//
	bool remove(std::wstring _dirpath, DWORD& _win32ErrorCode);

	// Copies files and creates all sub-directories if it is.
	// Basic implementation. 
	bool copy(std::wstring _from, std::wstring _to);

	// Sets current directory for the process, directory where executable file is.
	// That's important for windows-services because system automatically sets '%system32%' as a current directory.
	bool setImageDirAsCurrent();

	void removeLastSeparator(__inout std::string& _s);
	void removeLastSeparator(__inout std::wstring& _s);

	bool deleteFile(std::wstring _filepath);

	enum SearchMask
	{
		NO_SEARCH = 0,
		SEARCH_ONLY_DIRS, // 1
		SEARCH_ONLY_FILES, // 2
		SEARCH_FILES_AND_DIRS // 3
	};

	long getNamesOfObjInDir(std::set<std::wstring>& _vecFiles,
	                        const std::wstring& _rootDirectory,
	                        const std::wstring& _filesExtension,
	                        bool _bCountSubdirectories,
	                        bool _bSearchSubdirectories,
							logfile& _log,
							const int _nObjTypes = SEARCH_FILES_AND_DIRS);

	// sz > 0 - if size of a file is 0 or bigger // sz = 0 - error, or file size is 0
	bool getFileByteSize(std::string _filePath, size_t& sz);
	bool getFileByteSize(std::wstring _filePath, size_t& sz);
	bool isFilePresent(std::string _filePath);
	bool isFilePresent(std::wstring _filePath);

	// Returns true for root disk drives - "d:\", "c:".
	//
	bool isDiskRootPath(std::wstring _path);

	enum FsTypes
	{
		FsFile = 0,
		FsDirectory = 1,
		FsVolume = 2
	};


	bool getFsSize(const std::wstring& _fsObject, std::size_t& _fsSize, std::string& _error);

	bool resizeFile(const std::wstring& _fsObject, const std::size_t _newSize);

	// Получение последнего вермени модефикации
	bool getFsObjectLastModifiedTime(const std::string& _fsObject, std::time_t& _lastModifiedTime);

	//! Get all files, directories or other fs objects, giving root fs object
	bool getChildFsObjects(const std::wstring& _parent,
						 std::vector<std::pair<FsTypes, std::wstring>>& _out,
						 int& _numFiles,
						 int& _numDirs,
						 int& _numVolumes,
						 const std::string& _log);

	//! Get type of filesystem object, return false if '_object' is invalid
	bool getFsObjectType(const std::wstring& _object, windir::FsTypes& _outType);

	//! Get all roots of filesystem trees a.k.a disks, devices, storages, etc
	bool getRootFsObjects(std::vector<std::wstring>& _out);

	// Возвращает путь к текущей директории.
	std::wstring getCurrentDir();

	// Возвращает путь к временному каталогу.
	std::wstring getTempDir();

	// Возвращает путь к каталогу в катором находится исполняемый файл.
	std::wstring getImageDir();

	// Возвращает путь к исполняемому файлу на диске.
	std::wstring getImageFilePath();

	// Создает новый каталог. Если указан не существующий путь, создается серия вложенных каталогов.
	bool create(std::wstring _path);

	//
	// Создаёт путь каталогов для некоторого файла.
	//
	// Пример. Путь - x:\files\folder1\subfolder2\app.exe
	// Выстроит следующую систему каталогов: - x:\files\folder1\subfolder2
	//
	bool createDirForFile(std::wstring _filePath);

	//
	// Возвращает каталог в котором находится некоторый файл.
	//
	// Пример: "x:\docs\levels\" = getDirForFile("x:\docs\levels\my_pass.xml")
	//
	std::wstring getDirForFile(std::wstring _filePath);

	// Returns hierarchy of some file directory.
	//
	// In following format: [path, filetype]
	// path = x:\something.txt
	// filetype is true when it's file and false for directory.
	//
	bool getDirHierarchy(std::wstring _path, std::map<std::wstring, bool>& _hierarchy);
	
	// Returns content only for one directory.
	//
	bool getOneDirContent(std::wstring _path, std::map<std::wstring, bool>& _hierarchy);

	std::wstring getWindowsDir();

	//
	// Returns file name by full file path;
	// For c:\files\my\calc.dgt returns calc.dgt.
	//
	std::wstring getFileName(std::wstring _filePath);
	std::string getFileName(std::string _filePath);

	// Свободное место на диске
	bool getCurrentDiskFreeSpace(unsigned long long& freeSpace);

	bool setFilePointerToBegin(HANDLE _hFile);
}

namespace integers
{
	template<typename ArgType>
	void SerializeNumeric(std::ostringstream& _os, const ArgType& _inVar)
	{
		_os.write(reinterpret_cast<const char*>(&_inVar), sizeof(_inVar));
	}

	template<typename ArgType>
	void DeserializeNumeric(std::istringstream& _is, ArgType& _outVar)
	{
		_is.read(reinterpret_cast<char*>(&_outVar), sizeof(_outVar));
	}

	template<typename ArgType>
	void BufferToNumeric(const std::vector<char>& _inBuf, ArgType& _outNum)
	{
		std::istringstream istrm(std::string(_inBuf.begin(), _inBuf.end()));
		DeserializeNumeric(istrm, _outNum);
	}
}

namespace strings
{
	std::string ws_s(const std::wstring& s, UINT cp = CP_ACP);
	std::wstring s_ws(const std::string& s, UINT cp = CP_ACP);

	void toUpper(std::string& _s);
	void toLower(std::string& _s);

	void toUpper(std::wstring& _s);
	void toLower(std::wstring& _s);

	// Compares strings without case match.
	bool equalStrings(std::string _s1, std::string _s2);
	bool equalStrings(std::wstring, std::wstring);

	void addBytes(std::string& _str, const char* _p, std::size_t _length);

	std::string generateUuid();

	// split string into substrings by delimiter, empty strings ignoring unlike boost::split
	void splitString(const std::wstring& _in, const std::wstring& _delimiter, std::vector<std::wstring>& _out);


	void SerializeString(std::ostringstream& _os, const std::string& _inStr);

	void DeserializeString(std::istringstream& _is, std::string& _outStr);

	void SerializeWString(std::ostringstream& _os, const std::wstring& _inStr);

	void DeserializeWString(std::istringstream& _is, std::wstring& _outStr);

	void WStringToBuffer(const std::wstring& _inStr, std::vector<char>& _outBuf);

	std::wstring BufferToWString(const std::vector<char>& _inBuf);
}

namespace cpu
{
	bool IsCPUx64();
}

namespace system_power
{
	bool reboot(std::wstring _message, unsigned long _timeoutSeconds, bool _closeAllImmediately);
}

namespace registry
{
	enum RegistryTypes
	{
		RegKey = 0,
		RegValue = 1
	};

	bool getRootKeys(std::vector<std::wstring>& _out);
	bool getChildKeys(const std::wstring& _parentKey, 
					  std::vector<std::pair<RegistryTypes, std::wstring>>& _out,
					  int& _numKeys,
					  int& _numValues);

	bool SaveBinDataInRegistry(HKEY _hKey, std::string _valueName, std::string& _data);
	bool ReadBinDataFromRegistry(HKEY _hKey, std::string _valueName, std::string& _data);

	std::wstring GetKeyPathWithoutRootDir(const std::wstring& _key, HKEY& _hRootDir);
	int OpenKey(const std::wstring& _key, HKEY& _result, DWORD _flag, DWORD _accessRights);
	int TryToOpenKey(std::wstring _keyPath);
	long CreateKey(const std::wstring& _key, HKEY& _result, DWORD _flag, DWORD _accessRights);

	// Позволяет узнать существование ключа.
	bool isKeyExist(const std::wstring& _keypath);

	bool removeValue(std::wstring _keyPath, std::wstring _value, DWORD _flag = 0);

	bool setValue(const std::wstring& _keyPath, // path to parameter
				  const std::wstring& _valueName,  // name of parameter
				  DWORD _flag,                     // KEY_WOW64_32KEY or KEY_WOW64_64KEY for example
				  const std::vector<BYTE>& _data,  // new data
				  const DWORD _type);              // new type of parameter

	// Включает и выключает корзину Windows.
	bool enableWindowsRecycle(bool _toEnable);
}

namespace users
{
	bool getSidByUsername(std::wstring _domain, std::wstring _username, std::wstring& _outStrSid, SID& _outSid);

	bool getUsernameBySid(const std::wstring& _systemName, const std::wstring& _inSid, std::wstring& _outUsername);
}
