//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <boost/noncopyable.hpp>
#include <mutex>
#include <memory>


namespace whlp
{
	//
	// Keeps information about files in one directory.
	//
	class FilesKeeper
	{
	public:

		FilesKeeper(std::wstring _rootDir);

		// Returns true if root directory is accessible.
		//
		bool isAccessibleRootDir() const;

		std::wstring getRootDir() const;

		std::set<std::wstring /* file name */ > getFiles() const;

		// Returns true if the file is present.
		//
		bool isFilePresent(std::wstring _filename) const;

		bool getSize(std::wstring _filepath, size_t& _filesize) const;

		// Returns file path in format - getRootDir() + \\ + _filename.
		//
		std::wstring add(std::wstring _filename) const;

		// Deletes all files in root directory.
		//
		bool erase();

	private:
		std::wstring m_rootDir;
		bool m_isAccessible;
	};
}
