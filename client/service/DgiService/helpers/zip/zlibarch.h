// 
// #pragma once
// 
// #include <cstdint>
// 
// #define _WINSOCKAPI_
// #include <windows.h>
// #undef _WINSOCKAPI_
// 
// #include <string>
// #include <vector>
// #include <map>
// //#include <bitset>
// #include "../internal/log.h"
// 
// // #include <zip.h>
// // #include <unzip.h>
// // #include <zlib.h>
// 
// #define ONE				1
// #define MAX_FILENAME	512
// #define READ_SIZE		8192
// #define WRITEBUFFERSIZE	16384
// 
// 
// namespace zlib
// {
// 	static const int ZLIB_ATTRIBUTE_FILE = 32;
// 	static const int ZLIB_ATTRIBUTE_DIR = 16;
// 
// 	struct FileInfo
// 	{
// 		std::string name;
// 		unsigned long length;
// 		unsigned long compressedLength;
// 		int filetype;
// 	};
// 
// 
// 	// Extracts all files from '.zip' including sub-directories.
// 	// Has problems with no-latin names of extracted objects.
// 	bool extractArchive(std::string _srcZipFilePath, std::string _destination);
// 
// 	// Creates new zip-folder from directory.
// 	// Works in current thread and does not use parallels algorithms for packing.
// 	// Not effective and dangerous for big files.
// 	bool makeArchive(std::string _srcFolder, std::string _destArchiveName);
// 
// 	std::string getLastZlibError(int _errorNum);
// 
// 	// Key - where get file, value - the name of file in archive.
// 	//
// 	// c:\szi\service\ics.exe : .files/file_name_in_archive
// 	typedef std::map<std::string, std::string> FilesList;
// 
// 	//
// 	// _files - key-value map, where file is on a local disk and its path
// 	// which it should be in the archive.
// 	//
// 	// _destPath - the place, where to create .zip archive (full path with .zip extension).
// 	//
// 	bool makeArchive(const FilesList& _files, std::string _destPath);
// 
// 	// Prints additional information in log.
// 	bool makeArchive(const FilesList& _files, std::string _destPath, logfile& _dbglog);
// 
// 	//
// 	// Returns list of files and directories which are in archive.
// 	//
// 	std::pair<bool, std::vector<::zlib::FileInfo> > getFiles(const std::string& _zipArchive);
// 
// 	//
// 	// Extracts only one file with name '_srcFileName' which should completely match.
// 	//
// 	bool extractFile(const std::string& _zipArch, const std::string& _srcFileName, const std::string& _destFilePath);
// 
// 	// 
// 	bool isCorrectFormat(const std::string& _zipArchive);
// 
// 	//
// 	// Verifies an existence of necessary file in a list of files.
// 	//
// 	bool present(std::vector<::zlib::FileInfo>& _files, std::string _fileName);
// }
