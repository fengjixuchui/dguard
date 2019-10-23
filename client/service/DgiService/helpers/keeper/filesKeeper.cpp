//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "filesKeeper.h"
#include "../internal/helpers.h"

namespace whlp
{

	FilesKeeper::FilesKeeper(std::wstring _rootDir) :m_rootDir(_rootDir)
	{
		m_isAccessible = true;

		windir::removeLastSeparator(_rootDir);

		// Create new directory if it's not created yet.
		//
		if(!windir::isFilePresent(_rootDir))
		{
			if(!windir::create(_rootDir))
			{
				// Marks the root directory as unaccessible.
				//
				m_isAccessible = false;
			}
		}
	}

	bool FilesKeeper::isAccessibleRootDir() const
	{
		return m_isAccessible;
	}

	//

	std::wstring FilesKeeper::getRootDir() const
	{
		return m_rootDir;
	}

	std::set<std::wstring> FilesKeeper::getFiles() const
	{
		std::set<std::wstring> res;
		WIN32_FIND_DATAW fileInfo;

		auto searchPattern = m_rootDir + L"\\*.*";

		HANDLE hFile = FindFirstFileW(searchPattern.c_str(), &fileInfo);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					res.insert(fileInfo.cFileName);
				}

			} while (FindNextFileW(hFile, &fileInfo) == TRUE);

			FindClose(hFile);
		}

		return res;
	}

	bool FilesKeeper::isFilePresent(std::wstring _filename) const
	{
		auto filepath = m_rootDir + L"\\" + _filename;

		return windir::isFilePresent(filepath);
	}

	bool FilesKeeper::getSize(std::wstring _filename, size_t& _filesize) const
	{
		auto filepath = m_rootDir + L"\\" + _filename;

		if (windir::isFilePresent(filepath))
		{
			return windir::getFileByteSize(filepath, _filesize);
		}

		return false;
	}

	std::wstring FilesKeeper::add(std::wstring _filename) const
	{
		auto filepath = m_rootDir + L"\\" + _filename;

		return filepath;
	}

	bool FilesKeeper::erase()
	{
		bool removedAll = true;

		auto files = getFiles();
		auto path = m_rootDir;

		for (auto file : files)
		{
			auto filepath = path + L'\\' + file;
			auto removed = windir::deleteFile(filepath);

			if (!removed)
			{
				removedAll = false;
			}
		}

		return removedAll;
	}

}
