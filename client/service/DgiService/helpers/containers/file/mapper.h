//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <string>
#include <stdexcept>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

namespace whlp
{
	class Mapper
	{
	public:
		Mapper(std::string _file, bool _readOnly = false) :
			m_readOnly(_readOnly),
			m_file(_file),
		    m_pMappedData(nullptr),
		    m_hFile(INVALID_HANDLE_VALUE),
			m_hMap(NULL),
			m_mapSize(0)
		{
		}

		~Mapper()
		{
			unload();
		}

		HANDLE CreateOrOpen(std::string _filePath)
		{
			HANDLE hFile = INVALID_HANDLE_VALUE;
			DWORD flags = CREATE_NEW;

		OPEN_EXISTING_FILE:
			hFile = CreateFileA(_filePath.c_str(),
				m_readOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
			    0,
			    NULL,
				flags,
				0,
				NULL);

			if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_FILE_EXISTS))
			{
				flags = OPEN_EXISTING;
				goto OPEN_EXISTING_FILE;
			}

			return hFile;
		}

		bool load(DWORD _toMapBytes = 0)
		{
			DWORD fileSize = 0;
			HANDLE hFile = CreateOrOpen(m_file.c_str());
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			// Empty file.
			// 			if ((GetFileSize(hFile, 0) == 0) && (_toMapBytes == 0))
			// 			{
			// 				fileSize = sizeof DWORD;
			// 			}
			// 			else
			// 			{
			// 				fileSize = _toMapBytes == 0 ? GetFileSize(hFile, 0) : _toMapBytes;
			// 			}

			fileSize = _toMapBytes == 0 ? GetFileSize(hFile, 0) : _toMapBytes;

			HANDLE hMap = CreateFileMappingA(hFile, NULL, m_readOnly ? PAGE_READONLY : PAGE_READWRITE, 0, fileSize /* + additional size */, NULL);
			if (!hMap)
			{
				CloseHandle(hFile);
				return false;
			}

			PVOID pMappedTo = MapViewOfFile(hMap, m_readOnly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE), 0, 0, 0);
			if (!pMappedTo)
			{
				CloseHandle(hMap);
				CloseHandle(hFile);
				return false;
			}

			m_hMap = hMap;
			m_hFile = hFile;
			m_mapSize = fileSize;
			m_pMappedData = pMappedTo;
			return true;
		}

		void unload() throw()
		{
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}

			if (m_pMappedData)
			{
				UnmapViewOfFile(m_pMappedData);
				m_pMappedData = nullptr;
			}

			if (m_hMap)
			{
				CloseHandle(m_hMap);
				m_hMap = NULL;
			}
		}

		bool flush()
		{
			if (!m_readOnly)
			{
				if (m_hFile != INVALID_HANDLE_VALUE)
				{
					BOOL res1 = false, res2 = false;

					if (m_pMappedData)
					{
						res1 = FlushViewOfFile(m_pMappedData, 0);
						res2 = FlushFileBuffers(m_hFile);

						return res1 && res2;
					}
				}
			}

			return false;
		}

		bool isLoaded() const
		{
			return m_pMappedData != NULL;
		}

		DWORD fileSize()
		{
			DWORD fsize = 0;
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
				fsize = GetFileSize(m_hFile, NULL);
			}
			else
			{
				HANDLE hFile = CreateOrOpen(m_file);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					fsize = GetFileSize(hFile, NULL);
					CloseHandle(hFile);
				}
			}
			return fsize;
		}

		//bool increaseFileSize(std::size_t _addBytes)
		bool increaseFileSize(DWORD _addBytes)
		{
			DWORD newSize = (m_mapSize + _addBytes); // !!
			unload();
			return load(newSize);
		}

		bool cutSize(DWORD _newSize)
		{
			if (INVALID_HANDLE_VALUE == m_hFile)
			{
				return false;
			}

			if (m_pMappedData)
			{
				UnmapViewOfFile(m_pMappedData);
				CloseHandle(m_hMap);
				m_pMappedData = NULL;
				m_hMap = NULL;
			}

			DWORD fileSize = GetFileSize(m_hFile, 0);
			if (_newSize >= fileSize)
			{
				_newSize = fileSize;
			}

			SetFilePointer(m_hFile, _newSize, NULL, FILE_BEGIN);
			SetEndOfFile(m_hFile);
			unload();
			return load();
		}

		HANDLE getHandleOfFile() const
		{
			return m_hFile;
		}

		HANDLE getHandleOfMap() const
		{
			return m_hMap;
		}

		DWORD getSizeOfMap() const
		{
			return m_mapSize;
		}

		PVOID getMappedData() const
		{
			return m_pMappedData;
		}

		std::string getFilePath() const throw()
		{
			return m_file;
		}

	private:
		bool m_readOnly;

		std::string m_file;
		HANDLE m_hFile;
		HANDLE m_hMap;
		PVOID m_pMappedData;
		DWORD m_mapSize;
	};



	//
	// I had no time, and that's why it's a copy-paste.
	//

	class MapperW
	{
	public:
		MapperW(std::wstring _file, bool _readOnly = false) :
			m_readOnly(_readOnly),
			m_file(_file),
			m_pMappedData(nullptr),
			m_hFile(INVALID_HANDLE_VALUE),
			m_hMap(NULL),
			m_mapSize(0)
		{
		}

		~MapperW()
		{
			unload();
		}

		HANDLE CreateOrOpen(std::wstring _filePath)
		{
			HANDLE hFile = INVALID_HANDLE_VALUE;
			DWORD flags = CREATE_NEW;

		OPEN_EXISTING_FILE:
			hFile = CreateFileW(_filePath.c_str(),
				m_readOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
				0,
				NULL,
				flags,
				0,
				NULL);

			if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_FILE_EXISTS))
			{
				flags = OPEN_EXISTING;
				goto OPEN_EXISTING_FILE;
			}

			return hFile;
		}

		bool load(DWORD _toMapBytes = 0)
		{
			DWORD fileSize = 0;
			HANDLE hFile = CreateOrOpen(m_file.c_str());
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			// Empty file.
			// 			if ((GetFileSize(hFile, 0) == 0) && (_toMapBytes == 0))
			// 			{
			// 				fileSize = sizeof DWORD;
			// 			}
			// 			else
			// 			{
			// 				fileSize = _toMapBytes == 0 ? GetFileSize(hFile, 0) : _toMapBytes;
			// 			}

			fileSize = _toMapBytes == 0 ? GetFileSize(hFile, 0) : _toMapBytes;

			HANDLE hMap = CreateFileMappingW(hFile, NULL, m_readOnly ? PAGE_READONLY : PAGE_READWRITE, 0, fileSize /* + additional size */, NULL);
			if (!hMap)
			{
				CloseHandle(hFile);
				return false;
			}

			PVOID pMappedTo = MapViewOfFile(hMap, m_readOnly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE), 0, 0, 0);
			if (!pMappedTo)
			{
				CloseHandle(hMap);
				CloseHandle(hFile);
				return false;
			}

			m_hMap = hMap;
			m_hFile = hFile;
			m_mapSize = fileSize;
			m_pMappedData = pMappedTo;
			return true;
		}

		void unload() throw()
		{
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}

			if (m_pMappedData)
			{
				UnmapViewOfFile(m_pMappedData);
				m_pMappedData = nullptr;
			}

			if (m_hMap)
			{
				CloseHandle(m_hMap);
				m_hMap = NULL;
			}
		}

		bool flush()
		{
			if (!m_readOnly)
			{
				if (m_hFile != INVALID_HANDLE_VALUE)
				{
					BOOL res1 = false, res2 = false;

					if (m_pMappedData)
					{
						res1 = FlushViewOfFile(m_pMappedData, 0);
						res2 = FlushFileBuffers(m_hFile);

						return res1 && res2;
					}
				}
			}

			return false;
		}

		bool isLoaded() const
		{
			return m_pMappedData != NULL;
		}

		DWORD fileSize()
		{
			DWORD fsize = 0;
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
				fsize = GetFileSize(m_hFile, NULL);
			}
			else
			{
				HANDLE hFile = CreateOrOpen(m_file);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					fsize = GetFileSize(hFile, NULL);
					CloseHandle(hFile);
				}
			}
			return fsize;
		}

		bool increaseFileSize(DWORD _addBytes)
		{
			DWORD newSize = (m_mapSize + _addBytes); // !!

			unload();

			return load(newSize);
		}

		bool cutSize(DWORD _newSize)
		{
			if (INVALID_HANDLE_VALUE == m_hFile)
			{
				return false;
			}

			if (m_pMappedData)
			{
				UnmapViewOfFile(m_pMappedData);
				CloseHandle(m_hMap);
				m_pMappedData = NULL;
				m_hMap = NULL;
			}

			DWORD fileSize = GetFileSize(m_hFile, 0);
			if (_newSize >= fileSize)
			{
				_newSize = fileSize;
			}

			SetFilePointer(m_hFile, _newSize, NULL, FILE_BEGIN);
			SetEndOfFile(m_hFile);
			unload();
			return load();
		}

		HANDLE getHandleOfFile() const
		{
			return m_hFile;
		}

		HANDLE getHandleOfMap() const
		{
			return m_hMap;
		}

		DWORD getSizeOfMap() const
		{
			return m_mapSize;
		}

		PVOID getMappedData() const
		{
			return m_pMappedData;
		}

		std::wstring getFilePath() const throw()
		{
			return m_file;
		}

	private:
		bool m_readOnly;

		std::wstring m_file;
		HANDLE m_hFile;
		HANDLE m_hMap;
		PVOID m_pMappedData;
		DWORD m_mapSize;
	};
}
