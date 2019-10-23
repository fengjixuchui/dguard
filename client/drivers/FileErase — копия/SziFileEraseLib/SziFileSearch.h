#pragma once

#include <string>
#include <list>

namespace szi
{
	class FileSearch
	{
	public:
		typedef std::list<std::wstring> ListFiles;
	public:
		FileSearch();
		virtual ~FileSearch();
		void Search(const std::wstring& path, const std::wstring& exPath, ListFiles& files, bool insideDir = true) const;
	};

	class DirSearch
	{
	public:
		typedef std::list<std::wstring> ListDirs;
	public:
		void Search(const std::wstring& path, const std::wstring& exPath, ListDirs& files, bool insideDir = true) const;
	};
}
