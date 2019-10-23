// 
// 
// 
// //#define _CRT_SECURE_NO_WARNINGS
// 
// #include <zip.h>
// #include <unzip.h>
// #include <zlib.h>
// 
// #include <iostream>
// #include <fstream>
// 
// #include "../../stdafx.h"
// #include "zlibarch.h"
// #include "../internal/helpers.h"
// #include "../containers/file/mapper.h"
// #include <boost/algorithm/string.hpp>
// 
// 
// #pragma comment(lib, "zlibstat.lib")
// 
// namespace zlib
// {
// 	// Could has problem with extracting not Latin-symbols. ru_RU.
// 	bool extractArchive(std::string _srcZipFilePath, std::string _destination)
// 	{
// 		windir::removeLastSeparator(_destination);
// 		unzFile hZipFile = unzOpen(_srcZipFilePath.c_str());
// 		if (hZipFile == NULL)
// 		{
// 			return false;
// 		}
// 
// 		unz_global_info globalInfo = { 0 };
// 		auto errorZlib = unzGetGlobalInfo(hZipFile, &globalInfo);
// 		if (errorZlib != UNZ_OK)
// 		{
// 			unzClose(hZipFile);
// 			return false;
// 		}
// 
// 		for (uint32_t i = 0; i < globalInfo.number_entry; ++i)
// 		{
// 			unz_file_info zipFileInfo;
// 			char filename[MAX_FILENAME] = { 0 };
// 
// 			errorZlib = unzGetCurrentFileInfo(hZipFile, &zipFileInfo, filename, MAX_FILENAME, NULL, 0, NULL, 0);
// 			if (errorZlib != UNZ_OK)
// 			{
// 				unzClose(hZipFile);
// 				return false;
// 			}
// 
// 			std::string currentFileName(filename);
// 			//boost::to_lower(currentFileName, std::locale("")); // ru_RU
// 			boost::trim_if(currentFileName, boost::is_any_of("\\//"));
// 			boost::replace_all(currentFileName, "/", "\\");
// 			auto path = _destination + '\\' + currentFileName;
// 
// 			if (zipFileInfo.external_fa == 16) // 16 - is a flag for directory.
// 			{
// 				if(!windir::create(strings::s_ws(path)))
// 				{
// 					unzClose(hZipFile);
// 					return false;
// 				}
// 			}
// 			else
// 			{
// 				errorZlib = unzOpenCurrentFile(hZipFile);
// 				if (errorZlib != UNZ_OK)
// 				{
// 					unzClose(hZipFile);
// 					return false;
// 				}
// 
// 				FILE* outputFile = fopen(path.c_str(), "wb");
// 				if (outputFile == NULL)
// 				{
// 					unzCloseCurrentFile(hZipFile);
// 					unzClose(hZipFile);
// 					return false;
// 				}
// 
// 				do
// 				{
// 					char readBuffer[READ_SIZE];
// 					errorZlib = unzReadCurrentFile(hZipFile, readBuffer, READ_SIZE);
// 					if (errorZlib < 0) // means error.
// 					{
// 						unzCloseCurrentFile(hZipFile);
// 						unzClose(hZipFile);
// 						fclose(outputFile);
// 						return false;
// 					}
// 
// 					if (errorZlib > 0) // Has data to write.
// 					{
// 						size_t resultValue = fwrite(readBuffer, errorZlib, ONE, outputFile); // You should check return of fwrite...
// 						if (resultValue != ONE)
// 						{
// 							unzCloseCurrentFile(hZipFile);
// 							unzClose(hZipFile);
// 							fclose(outputFile);
// 							return false;
// 						}
// 					}
// 				} while (errorZlib > 0);
// 
// 				fclose(outputFile);
// 				unzCloseCurrentFile(hZipFile);
// 			}
// 
// 			unzGoToNextFile(hZipFile);
// 		}
// 
// 		unzClose(hZipFile);
// 		return true;
// 	}
// 
// 	bool makeArchive(std::string _srcFolder, std::string _destArchiveName)
// 	{
// 		bool result = true;
// 		std::map<std::wstring, bool> hierarchy;
// 		if (!windir::getDirHierarchy(strings::s_ws(_srcFolder), hierarchy))
// 		{
// 			return false;
// 		}
// 
// 		DeleteFileA(_destArchiveName.c_str());
// 		zipFile zipArchive = zipOpen(_destArchiveName.c_str(), APPEND_STATUS_CREATE);
// 		if (zipArchive == NULL)
// 		{
// 			return false;
// 		}
// 
// 		for (auto fileToAdd : hierarchy)
// 		{
// 			zip_fileinfo zfi = { 0 };
// 			zfi.external_fa = fileToAdd.second ? 32 /* file */ : 16 /* dir */;
// 
// 			std::string fileName = strings::ws_s(fileToAdd.first.substr(_srcFolder.size() + 1));
// 
// 			if (fileToAdd.second) // File.
// 			{
// 				std::fstream file(fileToAdd.first.c_str(), std::ios::binary | std::ios::in);
// 
// 				if (file.is_open())
// 				{
// 					//
// 					// Read file size - set file pointer to end and interpret offset as a file size.
// 					//
// 					file.seekg(0, std::ios::end);
// 					long long size_ = file.tellg();
// 
// 					if (size_ < 0)
// 					{
// 						result = false;
// 						file.close();
// 						break;
// 					}
// 
// 					//
// 					// Return file pointer back to the begin of file.
// 					//
// 					file.seekg(0, std::ios::beg);
// 
// 					long size = (long)size_;
// 
// 					//
// 					// Reserve memory for the hole file body. It's bad!!!
// 					//
// 					std::vector<char> buffer(size);
// 
// 					if ((size == 0) || file.read(&buffer[0], size))
// 					{
// 						if (S_OK == zipOpenNewFileInZip(zipArchive, fileName.c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
// 						{
// 							zipWriteInFileInZip(zipArchive, (size == 0) ? "" : &buffer[0], size);
// 							zipCloseFileInZip(zipArchive);
// 						}
// 						else
// 						{
// 							result = false;
// 							file.close();
// 							break;
// 						}
// 					}
// 
// 					file.close();
// 				}
// 				else
// 				{
// 					result = false; break;
// 				}
// 			} else // Directory.
// 			{
// 				if (S_OK == zipOpenNewFileInZip(zipArchive, fileName.c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) )
// 				{
// 					zipCloseFileInZip(zipArchive);
// 				}
// 				else
// 				{
// 					result = false; break;
// 				}
// 			}
// 		}
// 
// 		zipClose(zipArchive, NULL);
// 		return result;
// 	}
// 
// 	std::string getLastZlibError(int _errorNum)
// 	{
// 		return std::string("Library error(") + std::to_string((long long)_errorNum) + std::string("): ") + std::string(zError(_errorNum));
// 	}
// 
// 	bool makeArchive(const FilesList& _files, std::string _destPath)
// 	{
// 		bool result = true;
// 		
// 		DeleteFileA(_destPath.c_str());
// 
// 		zipFile zipArchive = zipOpen(_destPath.c_str(), APPEND_STATUS_CREATE);
// 		if (zipArchive == NULL)
// 		{
// 			return false;
// 		}
// 
// 		for (auto file : _files)
// 		{
// 			zip_fileinfo zfi = { 0 };
// 			zfi.external_fa = ZLIB_ATTRIBUTE_FILE;
// 
// 			whlp::Mapper srcFile(file.first, true);
// 			srcFile.load();
// 			if (srcFile.isLoaded())
// 			{
// 				if (S_OK == zipOpenNewFileInZip(zipArchive, file.second.c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
// 				{
// 					zipWriteInFileInZip(zipArchive, (srcFile.fileSize() == 0) ? "" : srcFile.getMappedData(), srcFile.fileSize());
// 					zipCloseFileInZip(zipArchive);
// 				}
// 				else
// 				{
// 					result = false;
// 					break;
// 				}
// 
// 				srcFile.unload();
// 			}
// 			else
// 			{
// 				result = false;
// 				break;
// 			}
// 		}
// 
// 		zipClose(zipArchive, NULL);
// 		return result;
// 	}
// 
// 
// // #define UNZ_OK                          (0)
// // #define UNZ_END_OF_LIST_OF_FILE         (-100)
// // #define UNZ_ERRNO                       (Z_ERRNO)
// // #define UNZ_EOF                         (0)
// // #define UNZ_PARAMERROR                  (-102)
// // #define UNZ_BADZIPFILE                  (-103)
// // #define UNZ_INTERNALERROR               (-104)
// // #define UNZ_CRCERROR                    (-105)
// 
// 	bool makeArchive(const FilesList& _files, std::string _destPath, logfile& _dbglog)
// 	{
// 		bool result = true;
// 
// 		DeleteFileA(_destPath.c_str());
// 
// 		zipFile zipArchive = zipOpen(_destPath.c_str(), APPEND_STATUS_CREATE);
// 		if (zipArchive == NULL)
// 		{
// 			_dbglog.printEx("%s: error - zipOpen for %s", __FUNCTION__, _destPath.c_str());
// 			return false;
// 		}
// 
// 		for (auto file : _files)
// 		{
// 			zip_fileinfo zfi = { 0 };
// 			zfi.external_fa = ZLIB_ATTRIBUTE_FILE;
// 
// 			whlp::Mapper srcFile(file.first, true);
// 			srcFile.load();
// 			if (srcFile.isLoaded())
// 			{
// 				if (UNZ_OK == zipOpenNewFileInZip(zipArchive, file.second.c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
// 				{
// 					zipWriteInFileInZip(zipArchive, (srcFile.fileSize() == 0) ? "" : srcFile.getMappedData(), srcFile.fileSize());
// 					zipCloseFileInZip(zipArchive);
// 				}
// 				else
// 				{
// 					_dbglog.printEx("%s: error - zipOpenNewFileInZip for %s", __FUNCTION__, file.second.c_str() );
// 
// 					result = false;
// 					break;
// 				}
// 
// 				srcFile.unload();
// 			}
// 			else
// 			{
// 				_dbglog.printEx("%s: error - srcFile.isLoaded( %s )", __FUNCTION__, file.first.c_str());
// 
// 				result = false;
// 				break;
// 			}
// 		}
// 
// 		zipClose(zipArchive, NULL);
// 		return result;
// 	}
// 
// 	std::pair<bool, std::vector<::zlib::FileInfo> > getFiles(const std::string& _zipArchive)
// 	{
// 		std::pair<bool, std::vector<::zlib::FileInfo> > result;
// 		result.first = false;
// 		std::vector<::zlib::FileInfo> files;
// 
// 		unzFile hZipFile = unzOpen(_zipArchive.c_str());
// 		if (hZipFile == NULL)
// 		{
// 			return result;
// 		}
// 
// 		unz_global_info globalInfo = { 0 };
// 		auto errorZlib = unzGetGlobalInfo(hZipFile, &globalInfo);
// 		if (errorZlib != UNZ_OK)
// 		{
// 			unzClose(hZipFile);
// 			return result;
// 		}
// 
// 		for (uint32_t i = 0; i < globalInfo.number_entry; ++i)
// 		{
// 			unz_file_info zipFileInfo;
// 			char filename[MAX_FILENAME] = { 0 };
// 
// 			errorZlib = unzGetCurrentFileInfo(hZipFile, &zipFileInfo, filename, MAX_FILENAME, NULL, 0, NULL, 0);
// 			if (errorZlib != UNZ_OK)
// 			{
// 				unzClose(hZipFile);
// 				return result;
// 			}
// 
// 			::zlib::FileInfo fi;
// 			fi.name = filename;
// 			fi.filetype = zipFileInfo.external_fa;
// 			fi.length = zipFileInfo.uncompressed_size;
// 			fi.compressedLength = zipFileInfo.compressed_size;
// 			files.push_back(fi);
// 
// 			unzGoToNextFile(hZipFile);
// 		}
// 
// 		unzClose(hZipFile);
// 		
// 		result.first = true;
// 		result.second.swap(files);
// 		return result;
// 	}
// 
// 	bool extractFile(const std::string& _zipArch, const std::string& _srcFileName, const std::string& _destFilePath)
// 	{
// 		unzFile hZipFile = unzOpen(_zipArch.c_str());
// 		if (hZipFile == NULL)
// 		{
// 			return false;
// 		}
// 
// 		unz_global_info globalInfo = { 0 };
// 		auto errorZlib = unzGetGlobalInfo(hZipFile, &globalInfo);
// 		if (errorZlib != UNZ_OK)
// 		{
// 			unzClose(hZipFile);
// 			return false;
// 		}
// 
// 		for (uint32_t i = 0; i < globalInfo.number_entry; ++i)
// 		{
// 			unz_file_info zipFileInfo;
// 			char filename[MAX_FILENAME] = { 0 };
// 
// 			errorZlib = unzGetCurrentFileInfo(hZipFile, &zipFileInfo, filename, MAX_FILENAME, NULL, 0, NULL, 0);
// 			if (errorZlib != UNZ_OK)
// 			{
// 				unzClose(hZipFile);
// 				return false;
// 			}
// 
// 			if (strings::equalStrings(_srcFileName, filename))
// 			{
// 				if (zipFileInfo.external_fa == ZLIB_ATTRIBUTE_FILE)
// 				{
// 					errorZlib = unzOpenCurrentFile(hZipFile);
// 					if (errorZlib != UNZ_OK)
// 					{
// 						unzClose(hZipFile);
// 						return false;
// 					}
// 
// 					FILE* outputFile = fopen(_destFilePath.c_str(), "wb");
// 					if (outputFile == NULL)
// 					{
// 						unzCloseCurrentFile(hZipFile);
// 						unzClose(hZipFile);
// 						return false;
// 					}
// 
// 					do
// 					{
// 						char readBuffer[READ_SIZE];
// 						errorZlib = unzReadCurrentFile(hZipFile, readBuffer, READ_SIZE);
// 						if (errorZlib < 0) // means error.
// 						{
// 							unzCloseCurrentFile(hZipFile);
// 							unzClose(hZipFile);
// 							fclose(outputFile);
// 							DeleteFileA(_destFilePath.c_str()); // Remove file if it was not read completely.
// 							return false;
// 						}
// 
// 						if (errorZlib > 0) // Has data to write.
// 						{
// 							size_t resultValue = fwrite(readBuffer, errorZlib, ONE, outputFile); // You should check return of fwrite...
// 							if (resultValue != ONE)
// 							{
// 								unzCloseCurrentFile(hZipFile);
// 								unzClose(hZipFile);
// 								fclose(outputFile);
// 								DeleteFileA(_destFilePath.c_str()); // Remove file if at was not read completely.
// 								return false;
// 							}
// 						}
// 					} while (errorZlib > 0);
// 
// 					fclose(outputFile);
// 					unzCloseCurrentFile(hZipFile);
// 				}
// 			}
// 
// 			unzGoToNextFile(hZipFile);
// 		}
// 
// 		unzClose(hZipFile);
// 		return true;
// 	}
// 
// 	bool isCorrectFormat(const std::string& _zipArchive)
// 	{
// 		return false;
// 	}
// 
// 	bool present(std::vector<::zlib::FileInfo>& _files, std::string _fileName)
// 	{
// 		for (auto f : _files)
// 		{
// 			if (strings::equalStrings(_fileName, f.name))
// 			{
// 				return true;
// 			}
// 		}
// 
// 		return false;
// 	}
// }
