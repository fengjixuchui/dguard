#include "SziFileSearch.h"
#include "Windows.h"

namespace szi
{
	FileSearch::FileSearch()
	{
	}


	FileSearch::~FileSearch()
	{
	}

	void FileSearch::Search(const std::wstring& path, const std::wstring& exPath, std::list<std::wstring>& files, bool insideDir) const
	{
		const std::wstring	lSearchMask(path + L"\\*");
		WIN32_FIND_DATA		lFindFileData;
		HANDLE				hFind = FindFirstFile(lSearchMask.c_str(), &lFindFileData);

		if (INVALID_HANDLE_VALUE != hFind)
		{
			do
			{
				if ((lFindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT)
					continue;

				if (!((lFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
				{
					if (exPath.empty())
						files.push_back(lFindFileData.cFileName);
					else
						files.push_back(exPath + L"\\" + lFindFileData.cFileName);
				}
				else
				{
					const std::wstring lFileName = lFindFileData.cFileName;
					if (lFileName != L"." && lFileName != L".." && insideDir)
					{
						Search(path + L"\\" + lFileName + L"\\", !exPath.empty() ? exPath + L"\\" + lFileName : lFileName, files);
						files.push_back(!exPath.empty() ? exPath + L"\\" + lFileName : lFileName);
					}
				}
			} while (FindNextFile(hFind, &lFindFileData));

			FindClose(hFind);
		}
	}

	void DirSearch::Search(const std::wstring& path, const std::wstring& exPath, ListDirs& files, bool insideDir /*= true*/) const
	{
		const std::wstring	lSearchMask(path + L"\\*");
		WIN32_FIND_DATA		lFindFileData;
		HANDLE				hFind = FindFirstFile(lSearchMask.c_str(), &lFindFileData);

		if (INVALID_HANDLE_VALUE != hFind)
		{
			do
			{
				if (((lFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
				{
					const std::wstring lFileName = lFindFileData.cFileName;
					if (lFileName != L"." && lFileName != L".." && insideDir)
					{
						Search(path + L"\\" + lFileName + L"\\", !exPath.empty() ? exPath + L"\\" + lFileName : lFileName, files);
						files.push_back(!exPath.empty() ? exPath + L"\\" + lFileName : lFileName);
					}
					else if (lFileName != L"." && lFileName != L"..")
					{
						if (exPath.empty())
							files.push_back(lFindFileData.cFileName);
						else
							files.push_back(exPath + L"\\" + lFindFileData.cFileName);
					}
				}
			} while (FindNextFile(hFind, &lFindFileData));

			FindClose(hFind);
		}
	}

}
