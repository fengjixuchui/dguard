//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once


#include "ManagerSecureErase.h"
#include "../../../helpers/internal/helpers.h"

namespace logic
{
	namespace secure_erase
	{
		namespace manager
		{
			Shredder::Shredder(std::wstring _logpath /*= L"shredder.log"*/) :
				m_log(strings::ws_s(_logpath)),
				m_driverConfigured(false)
			{
				m_driverConfigured = configureDriver();
			}

			Shredder::~Shredder()
			{
				m_log.print(std::string(__FUNCTION__));
			}

			logic::common::InternalStatus Shredder::eraseList(const std::vector<EraseObject>& _toEraseList, std::vector<EraseObjectResult>& _outResultList)
			{
				logic::common::InternalStatus status = Int_UnknownError;

				//
				// Verify connection with driver.
				//
				// if (...)
				// {
				//		return logic::common::InternalStatus::AnError;
				// }
				//

				for (auto toErase : _toEraseList)
				{
					//
					// Erase file data and file itself.
					//
					status = this->eraseOneObject(toErase, _outResultList);

					if (status != InternalStatus::Int_Success)
					{
						//
						//	Save information about error if it was.
						//

						EraseObjectResult eor;
						eor.object = toErase;
						eor.result = status;

						_outResultList.push_back(eor);
					}
				}

				//
				// If connection with driver was established returns success.
				//
				return logic::common::InternalStatus::Int_Success;
			}

			logic::common::InternalStatus Shredder::eraseOneObject(common::EraseObject _eraseObject, std::vector<EraseObjectResult>& _outResultList)
			{
				std::string fnname = std::string(__FUNCTION__);

				logic::common::InternalStatus result = Int_UnknownError;
				windir::DirEntry dirHierarchy;

				//
				// Verify connection with driver.
				//
				// if (...)
				// {
				//		return logic::common::InternalStatus::AnError;
				// }
				//

				if (_eraseObject.objectType == EOT_File)
				{
					auto status = secureEraseFile(_eraseObject.path);
					if (status != Int_Success)
					{
						EraseObjectResult eor;
						eor.result = status;
						eor.object = _eraseObject;
						_outResultList.push_back(eor);

						m_log.print(fnname + ": error - can't remove file object " + strings::ws_s(_eraseObject.path) + " internal error code is " + std::to_string((long)status));
					}
				}
				else if (_eraseObject.objectType == EOT_Directory) // folder
				{
					if (!windir::isFilePresent(_eraseObject.path))
					{
						EraseObjectResult eor;
						eor.result = Int_NotFound;
						eor.object = _eraseObject;
						_outResultList.push_back(eor);

						m_log.print(fnname + ": error - directory is not present " + strings::ws_s(_eraseObject.path));
					}
					else
					{
						//
						//	To do first attempt.
						//

						windir::getDirStructRec(_eraseObject.path, dirHierarchy);
						eraseDirHierarchy(dirHierarchy, _outResultList);

						//
						//	Second try if we couldn't remove something at last time and may be somebody has wrote something in directory.
						//

						dirHierarchy.files.clear();
						dirHierarchy.sub_dirs.clear();

						windir::getDirStructRec(_eraseObject.path, dirHierarchy);
						if (!(dirHierarchy.files.empty() && dirHierarchy.sub_dirs.empty()))
						{
							eraseDirHierarchy(dirHierarchy, _outResultList);
						}
					}
				}
				else if (_eraseObject.objectType == EOT_Disk) // hole drive
				{
					//
					//	First attempt.
					//

					windir::getDirStructRec(_eraseObject.path, dirHierarchy);
					eraseDirHierarchy(dirHierarchy, _outResultList);

					//
					//	Second try if we couldn't remove something at last time and may be somebody has wrote something in directory.
					//

					dirHierarchy.files.clear();
					dirHierarchy.sub_dirs.clear();

					windir::getDirStructRec(_eraseObject.path, dirHierarchy);
					if (!(dirHierarchy.files.empty() && dirHierarchy.sub_dirs.empty()))
					{
						eraseDirHierarchy(dirHierarchy, _outResultList);
					}
				}
				else
				{
					m_log.print(fnname + ": error - unknown erase object type:  " + strings::ws_s(_eraseObject.path) + " : " + std::to_string((long)_eraseObject.objectType));
					return logic::common::InternalStatus::Int_UnknownType;
				}

				return logic::common::InternalStatus::Int_Success;
			}

			logic::common::InternalStatus Shredder::secureEraseFile(std::wstring _filePath)
			{
				std::string fn = std::string(__FUNCTION__);

				//
				//	Initialize driver if it was not.
				//

				configureDriver();

				logic::common::InternalStatus eraseResult = logic::common::InternalStatus::Int_Success;
				::szi::drv::FileEraseClient eraseDriver;
				
				//
				//	To connect with driver.
				//

				if (eraseDriver.Open())
				{
					if (eraseDriver.EraseFile(_filePath))
					{
						if (eraseDriver.waitEraseComplete(_filePath))
						{
							//
							//	Success file was removed!
							//

							//
							//	Find just removed file in recycle bin and remove it from there.
							//

							// ... not implemented yet ...
						}
						else
						{
							m_log.print(fn + ": error - can't wait for file erase results.");
							eraseResult = logic::common::InternalStatus::Int_UnknownError;
						}
					}
					else
					{
						m_log.print(fn + ": error - can't erase file in driver by unknown ");
						eraseResult = logic::common::InternalStatus::Int_UnknownError;
					}

					eraseDriver.Close();
				}
				else
				{
					eraseResult = logic::common::InternalStatus::Int_DriverNotConnected;

					//////////////////////////////////////////////////////////////////////////
					DWORD dwWin32Error = 0;
					auto result = DeleteFileW(_filePath.c_str());

					if (result = FALSE)
					{
						dwWin32Error = GetLastError();

						m_log.print(std::string(__FUNCTION__) + ": error - can't remove file " + strings::ws_s(_filePath) + " win32 error code is " + std::to_string(dwWin32Error));

						return logic::common::InternalStatus::Int_UnknownError;
					}
					else
					{
						eraseResult = logic::common::InternalStatus::Int_Success;
					}
					//////////////////////////////////////////////////////////////////////////
				}

				return eraseResult;
			}

			void Shredder::eraseDirHierarchy(const windir::DirEntry& _dir, std::vector<EraseObjectResult>& _outResultList)
			{
				std::string fnname = __FUNCTION__;

				for (auto file : _dir.files)
				{
					auto result = secureEraseFile(file);

					//
					//	Save an error info.
					//

					if (result != Int_Success)
					{
						EraseObjectResult eor;
						eor.result = result;
						eor.object.path = file;
						eor.object.objectType = logic::common::EraseObjectType::EOT_File;

						m_log.print(fnname + ": error - can't remove file object " + strings::ws_s(file) + " internal error code is " + std::to_string((long)result));

						//
						//	Stop erasing mechanism even we couldn't remove a one file.
						//

						m_log.print(fnname + ": interrupt erasing process in that case.");
						return;
					}
				}

				//
				//	Go deeper to the sub dirs.
				//

				for (auto dir : _dir.sub_dirs)
				{
					eraseDirHierarchy(dir.second, _outResultList);
				}

				//
				//	If we are here - it means we are in lowest level of the hierarchy.
				//
				//	(*) All files in the directory should be removed.
				//	(*) That directory should not have any sub directories. 
				//

				DWORD dwWin32Error = 0;

				//
				//	We can't remove hard drive itself.
				//

				if (!windir::isDiskRootPath(_dir.currPath))
				{
					if (!windir::remove(_dir.currPath, dwWin32Error))
					{
						m_log.print(fnname + ": error - can't remove directory " + strings::ws_s(_dir.currPath) + " win32 last error is " + std::to_string(dwWin32Error));

						logic::common::InternalStatus finalStatus = Int_UnknownError;
						if (!windir::isFilePresent(_dir.currPath))
						{
							finalStatus = Int_NotFound;
						}

						EraseObjectResult eor;
						eor.result = finalStatus;
						eor.object.path = _dir.currPath;
						eor.object.objectType = logic::common::EraseObjectType::EOT_Directory;
					}
				}
				else
				{
					m_log.print(fnname + ": do not remove disk drive - " + strings::ws_s(_dir.currPath));
				}
			}

			logic::common::InternalStatus Shredder::eraseFile(std::wstring _filePath)
			{
				if (!windir::isFilePresent(_filePath))
				{
					return logic::common::InternalStatus::Int_NotFound;
				}

				return secureEraseFile(_filePath);
			}

			bool Shredder::ctrInit()
			{
				return true;
			}

			bool Shredder::ctrlLateInit()
			{
				return true;
			}

			bool Shredder::ctrlIsRunning()
			{
				::szi::drv::FileEraseClient m_eraseDriver;
				bool drvAvailable = m_eraseDriver.Open();

				if (drvAvailable)
				{
					m_eraseDriver.Close();
				}

				return drvAvailable;
			}

			bool Shredder::ctrlShutdown(bool _canWait)
			{
				return true;
			}

			std::string Shredder::ctrlGetName()
			{
				return SYSTEM_SHREDDER;
			}

			bool Shredder::ctrlSetPassword(::logic::common::MasterPassword _password)
			{
				return true;
			}

			bool Shredder::ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword)
			{
				return true;
			}

			bool Shredder::configureDriver()
			{
				std::unique_lock<std::mutex> lock(m_lock);

				//	Nothing to do if driver configured.
				if (m_driverConfigured)
				{
					return true;
				}

				bool configured = false;

				::szi::drv::FileEraseClient m_eraseDriver;

				if (m_eraseDriver.Open())
				{
					configured = m_eraseDriver.SetAutoErase(false) &&
						m_eraseDriver.DisableAutoErase() &&
						m_eraseDriver.SetCountCircle(1) &&
						m_eraseDriver.SetMask(0x00);

					m_eraseDriver.Close();
				}
				else
				{
					std::string lastErr = strings::ws_s(m_eraseDriver.GetLastError());

					m_log.print(std::string(__FUNCTION__) + ": error - failed to configure, last error is " + lastErr);
				}

				//	Save result for future. It helps to avoid double driver configuration.
				m_driverConfigured = configured;

				return configured;
			}
		}
	}
}
