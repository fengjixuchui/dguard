//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <sstream>

#include "DgiFolderLockManager.h"
#include "../../../logic/common/DgiEngine.h"


namespace logic
{
	namespace folderlock
	{
		namespace manager
		{
            void generateFLockId(unsigned char* _ptr, int _size)
            {
                for (int i = 0; i < _size; ++i)
                {
                    _ptr[i] = (rand() * 100) % 255;
                }
            }

			FileGuard::FileGuard(std::wstring _logfilepath) :
				m_log(strings::ws_s(_logfilepath)),
				m_driver(m_log),
				m_storage(::logic::common::DgiEngine::getCryptor(), m_log, "flock.storage")
			{
				m_log.print(std::string(__FUNCTION__));
				
				//
				// Register current process as a main FLock service for kernel-mode driver.
				//
				m_log.print(std::string(__FUNCTION__) + ": register current process " + std::to_string(GetCurrentProcessId()) + " (PID) as a head FLock service process.");

				if (m_driver.registerAsService())
				{
					m_log.print(std::string(__FUNCTION__) + ": process id " + std::to_string(GetCurrentProcessId()) + " is registered for FLock driver.");
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error -  process " + std::to_string(GetCurrentProcessId()) + " (PID) is not registered for FLock driver.");
				}
			}

			FileGuard::~FileGuard()
			{
				m_log.print(std::string(__FUNCTION__));

				m_log.print(std::string(__FUNCTION__) + ": unregister current process " + std::to_string(GetCurrentProcessId()) + " (PID) as a head FLock service process.");

				if (m_driver.unregisterAsService())
				{
					m_log.print(std::string(__FUNCTION__) + ": successfully unregistered.");
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error - process was not unregistered.");
				}
			}

			logic::common::InternalStatus FileGuard::add(::logic::folderlock::storage::FLockObject& _flock)
			{
				m_log.printEx("%s", __FUNCTION__);

				::logic::common::InternalStatus result = logic::common::InternalStatus::Int_Success;

				std::wstring path = _flock.path;

				if (m_storage.present(path))
				{
					m_log.printEx("%s: error - already present.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_PresentAlready;
				}

				//
				//	If FLock-ID is not set explicitly then generate it randomly.
				//

				{
					unsigned char flockId[sizeof(_flock.uniqueId)] = { 0 };
					bool idIsNotSet = memcmp(flockId, _flock.uniqueId, sizeof(_flock.uniqueId)) == 0;

					if (idIsNotSet)
					{
                        generateFLockId(_flock.uniqueId, sizeof(flockId));
					}
				}

				bool idIsAlreadyUsed = m_storage.presentById(
					std::string((const char*)_flock.uniqueId,
					sizeof(_flock.uniqueId)));

				if ( idIsAlreadyUsed )
				{
					m_log.printEx("%s: error - FLock ID is already present.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_PresentAlready;
				}
				
				//
				//	0. Preparation stages.
				//
				
				std::wstring lowLevelPath = path;
				if (path.find(L"\\??\\") == std::string::npos)
				{
                    //
                    //  Add "\??\" prefix if it is missed.
                    //  Into service FLocks storage we add path without prefixes but actually we work only with prefixes, that is because
                    //  on a kernel level operation system does not understand paths like: "x:\folder", "c:\windows".
                    //  Need to use prefixes like: "\Device\HarddiskVolume1\Users\admin0\AppData\Local" or "\??\x:\dir\docs.zip".
                    //

					lowLevelPath = L"\\??\\" + path;
				}

				//
				//	It is require to do some tricks with parent directory of the file which we want to hide!
				//
                //  Updates:
                //      Earlier I marked parent directories with FLOCK_FLAG_HAS_FLOCKS only if we wanted to hide file and
                //      I ignored files which user wanted only to protected from an access. Now I decided to mark parent directories
                //      for all cases - FLock_Hidden, FLock_HiddenAndLocked, FLock_Locked.
                //      I did that because in a future user can change protection policy from Locked to Hidden, but if user also earlier had changed
                //      name of parent directory we could fail when try to mark parent dir just because we could not know new name for parent directory.
                //
                //  An example:
                //      1. User marked x:\dir1\docs\salary.docx as LOCKED.
                //      2. When some period of time has passed user decided to rename parent dir to "my_docs" and finally our protected file started to be
                //      on a completely different path - "x:\dir1\my_docs\salary.docx". And it is ok now. We continue to protect target file as protected earlier.
                //      3. User decided to change protection state on LockedAndHidden. And now we can not provide any guarantees about file hiding, because
                //      our protection system tries to mark "x:\dir1\docs" with FLOCK_FLAG_HAS_FLOCKS, but it is an invalid path - there is not directory with "docs" name,
                //      now it is "my_docs".
                //
                //      That is why I decided to mark parent dir each time with special flag for all protection cases.
                //

				// if ((_flock.state == storage::FLock_Hidden) || (_flock.state == storage::FLock_HiddenAndLocked))
				{
					//
					//	Parent directory of the protected file system object should be marked with special flag - FLOCK_FLAG_HAS_FLOCKS.
					//	That flag helps our driver to improve common system performance and not to process IRP_MJ_DIRECTORY_CONTROL requests for each directory.
					//
					//	(!): Ignore if the target file located in a root volume. Sometimes we can not write EAs into volumes root.
					//

					if (!isVolume(lowLevelPath))
					{
						if (IntSuccess(markParentDirHasFLocks(lowLevelPath)))
						{
							m_log.print(std::string(__FUNCTION__) + ": info - parent dir was marked with flock meta.");
						}
						else
						{
							//
							//	Return an error if we could not mark parent directory with FLOCK_FLAG_HAS_FLOCKS flag.
							//	Without this flag we can not guarantee protection of the target file system object.
							//

							m_log.print(std::string(__FUNCTION__) + ": error - could not mark parent dir with flock meta signature!");
							return ::logic::common::InternalStatus::Int_FLockWasNotSigned;
						}
					}
					else
					{
						m_log.print(std::string(__FUNCTION__) + ": info - ignore, parent dir is a volume.");
					}
				}
				
				//
				//	Mark target file system object with a special flock-meta signature.
				//

				::driver::FLOCK_META signature = { 0 };
				UCHAR metaSignature[] = FLOCK_META_SIGNATURE;
				memcpy(signature.signature, metaSignature, sizeof(metaSignature));
				memcpy(signature.uniqueId, _flock.uniqueId, sizeof(_flock.uniqueId));

				if (m_driver.writeFileMeta(lowLevelPath, signature))
				{
					//
					//  It is important to clear driver's cache and invalidate filter contexts after any manipulations of flock's meta information on disk,
                    //  after any changes of FLock EAs on disk.
					//

					m_log.print( std::string(__FUNCTION__) + ": file on disk was successfully signed with flock's meta.");
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": critical error - file on disk was not signed with flock meta.");
					return ::logic::common::InternalStatus::Int_FLockWasNotSigned;
				}

				//
				//	1. Add the flock into service list.
				//

				if (m_storage.add(_flock))
				{
					m_log.print(std::string(__FUNCTION__) + ": flock successfully added.");

#ifdef _DEBUG
                    std::stringstream ss;
                    for (int i = 0; i < sizeof(_flock.uniqueId); ++i)
                        ss << std::hex << (int)_flock.uniqueId[i];

                    std::string flockIdHex = ss.str();

                    m_log.print(std::string(__FUNCTION__) + ": info - flock id - " + flockIdHex);
#endif

					//
					//	2. Add to driver's list.
					//

					auto driverResponse = sendFlocksToDriver(true);

					if (IntSuccess(driverResponse))
					{
						m_log.printEx("%s: success - flocks list also was updated in driver.", __FUNCTION__);
					}
					else
					{
						m_log.printEx("%s: success - flocks list updated in driver, internal error code is %d.", __FUNCTION__, driverResponse);
					}

					return driverResponse;
				}
				else
				{
					m_log.printEx("%s: error - can't add the flock because of unknown reason.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_UnknownError;
				}
			}

			logic::common::InternalStatus FileGuard::present(const std::wstring& _path)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_storage.isLoaded())
				{
					return ::logic::common::InternalStatus::Int_UnknownError;
				}

				if (m_storage.present(_path))
				{
					return ::logic::common::InternalStatus::Int_Success;
				}
				
				return ::logic::common::InternalStatus::Int_NotFound;
			}

			logic::common::InternalStatus FileGuard::presentById(const std::string& _flockId)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_storage.isLoaded())
				{
					return ::logic::common::InternalStatus::Int_UnknownError;
				}

				if (m_storage.presentById(_flockId) )
				{
					return ::logic::common::InternalStatus::Int_Success;
				}

				return ::logic::common::InternalStatus::Int_NotFound;
			}

            logic::common::InternalStatus FileGuard::getById(const std::string& _flockId /* AF098DC3100EACB23CDE59 */, ::logic::folderlock::storage::FLockObject& _outFlock)
            {
                m_log.printEx("%s", __FUNCTION__);

                if (!m_storage.presentById(_flockId))
                {
                    m_log.printEx("%s: error - not found.", __FUNCTION__);
                    return ::logic::common::InternalStatus::Int_NotFound;
                }

                if (m_storage.getInfoById(_flockId, _outFlock))
                {
                    //
                    //	If _outFlock.state = FLock_Unlocked, then verify an existence of the file and
                    //	if it's missed or unaccessible - change state to FLock_Missed.
                    //

                    if (_outFlock.state == storage::FLock_Unlocked)
                    {
                        std::wstring path = _outFlock.path;

                        if ((_outFlock.type == storage::FLockObjectType::FLock_File) || (_outFlock.type == storage::FLockObjectType::FLock_Directory))
                        {
                            if (!windir::isFilePresent(path))
                            {
                                _outFlock.state = storage::FLock_Missed;
                            }
                        }
                        else if (_outFlock.type == storage::FLockObjectType::FLock_HardDisk)
                        {
                            //	Verify presence of the local disk.
                            //	...
                        }
                    }

                    return ::logic::common::InternalStatus::Int_Success;
                }
                else
                {
                    m_log.printEx("%s: error - can't read flock data.", __FUNCTION__);
                    return ::logic::common::InternalStatus::Int_UnknownError;
                }
            }

            logic::common::InternalStatus FileGuard::get(const std::wstring& _path /* x:\files\my.doc */, ::logic::folderlock::storage::FLockObject& _outFlock)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_storage.present(_path))
				{
					m_log.printEx("%s: error - not found.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_NotFound;
				}

				if(m_storage.getInfo(_path, _outFlock))
				{
					//
					//	If _outFlock.state = FLock_Unlocked, then verify an existence of the file and
					//	if it's missed or unaccessible - change state to FLock_Missed.
					//

					if (_outFlock.state == storage::FLock_Unlocked)
					{
						std::wstring path = _outFlock.path;

						if ((_outFlock.type == storage::FLockObjectType::FLock_File) || (_outFlock.type == storage::FLockObjectType::FLock_Directory))
						{
							if (!windir::isFilePresent(path))
							{
								_outFlock.state = storage::FLock_Missed;
							}
						}
						else if (_outFlock.type == storage::FLockObjectType::FLock_HardDisk)
						{
							//	Verify presence of the local disk.
							//	...
						}
					}

					return ::logic::common::InternalStatus::Int_Success;
				}
				else
				{
					m_log.printEx("%s: error - can't read flock data.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_UnknownError;
				}
			}

			logic::common::InternalStatus FileGuard::getAll(std::vector<::logic::folderlock::storage::FLockObject>& _flocks)
			{
				m_log.printEx("%s", __FUNCTION__);

				//	1. Get all entries only if the storage is loaded.
				if (m_storage.isLoaded())
				{
					auto flocks = m_storage.getAll();
					flocks.swap(_flocks);

					return ::logic::common::InternalStatus::Int_Success;
				}
				{
					m_log.printEx("%s: error - storage is not loaded.", __FUNCTION__);
				}

				return ::logic::common::InternalStatus::Int_UnknownError;
			}

			logic::common::InternalStatus FileGuard::remove(const ::logic::folderlock::storage::FLockObject& _flock)
			{
				m_log.printEx("%s: by FlockObject", __FUNCTION__);

				std::wstring path = _flock.path;

				return this->remove(path);
			}

			logic::common::InternalStatus FileGuard::remove(std::wstring _path /* c:\myown\rivate\files */)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_storage.present(_path))
				{
					m_log.printEx("%s: error - not found.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_NotFound;
				}

				//
				//	Remove flock from user-mode application.
				//

				if (m_storage.remove(_path))
				{	
					//
					//	Ok. It's time to remove the flock in driver's list.
					//

					auto driverResponse = sendFlocksToDriver();

					if (IntSuccess(driverResponse))
					{
						m_log.printEx("%s: success - flocks list updated in driver.", __FUNCTION__);
					}
					else
					{
						m_log.printEx("%s: success - flocks list updated in driver, internal code %d.", __FUNCTION__, driverResponse);
					}

					return driverResponse;
				}
				else
				{
					m_log.printEx("%s: error - can't remove a flock from storage.", __FUNCTION__);
				}

				return ::logic::common::InternalStatus::Int_UnknownError;
			}

			//
			//	We do not remove Extended Attributes of target file.
			//	It is little bit messy but may be that file or directory is used by some other policies.
			//
			//	An example:
			//		Directory - "X:\dir1\potato" can have flock's meta and earlier user added two different
			//		access policies - hide and lock, and we remove just one flock for hiding and do not touch
			//		lock policy. After removing first policy, object continues to be protected. But if we removed 
			//		flock's meta from the file after removing first access policy, we could break all protection.
			//

			logic::common::InternalStatus FileGuard::removeById(const std::string& _flockId /* AF098DC3100EACB23CDE59 */)
			{
				m_log.printEx("%s", __FUNCTION__);

				//
				//	1. Remove in user-mode storage.
				//

				if ( m_storage.removeById(_flockId) )
				{
					m_log.printEx("%s: success - flock was removed in storage.", __FUNCTION__);

					// 2. Ok. It's time to remove the flock in driver's list.
					auto driverResponse = sendFlocksToDriver();

					if (IntSuccess(driverResponse))
					{
						m_log.printEx("%s: success - flocks list updated in driver.", __FUNCTION__);
					}
					else
					{
						m_log.printEx("%s: success - flocks list updated in driver, internal code %d.", __FUNCTION__, driverResponse);
					}

					return driverResponse;
				}
				else
				{
					m_log.printEx("%s: error - can't remove flock from storage.", __FUNCTION__);
				}

				return ::logic::common::InternalStatus::Int_UnknownError;
			}

			logic::common::InternalStatus FileGuard::changeState(std::wstring _path /* c:\myown\dx\salary.doc */, ::logic::folderlock::storage::FLockState _newState)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_storage.isLoaded())
				{
					m_log.printEx("%s: error - storage was not found.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_NotLoaded;
				}

				if (!m_storage.present(_path))
				{
					m_log.printEx("%s: error - flock not found.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_NotFound;
				}

				m_log.print(std::string(__FUNCTION__) + ": info - change a flock's state on " + std::to_string((int)_newState));

                //
				// 1. Mark flock's entry with a new access state. Changes rely only to service's list.
                //

				if (m_storage.setState(_path, _newState))
				{
					// Success, guys!
					m_log.print(std::string(__FUNCTION__) + ": info - flock's state was successfully changed.");

                    //
					// 2. Update driver's list.
                    //

					auto driverResponse = sendFlocksToDriver(false); // merge;

					if (IntSuccess(driverResponse))
					{
						m_log.printEx("%s: success - flocks list also was updated in driver.", __FUNCTION__);
					}
					else
					{
						m_log.printEx("%s: success - flocks list updated in driver, internal error code %d.", __FUNCTION__, driverResponse);
					}

					return driverResponse;
				}
				else
				{
					m_log.printEx("%s: error - could not change state for a flock the entry .", __FUNCTION__);

					return ::logic::common::InternalStatus::Int_UnknownError;
				}

				return ::logic::common::InternalStatus::Int_UnknownError;
			}

            logic::common::InternalStatus FileGuard::changeStateById(const std::string& _flockId /* AF098DC3100EACB23CDE59 */, ::logic::folderlock::storage::FLockState _newState)
            {
                m_log.printEx("%s", __FUNCTION__);

                if (!m_storage.isLoaded())
                {
                    m_log.printEx("%s: error - storage was not found.", __FUNCTION__);
                    return ::logic::common::InternalStatus::Int_NotLoaded;
                }

#ifdef _DEBUG
                std::stringstream ss;
                for (int i = 0; i < _flockId.length(); ++i)
                    ss << std::hex << (int)_flockId.at(i);

                std::string flockIdHex = ss.str();

                m_log.print(std::string(__FUNCTION__) + ": info - flock id - " + flockIdHex);

#endif

                if (!m_storage.presentById(_flockId))
                {
                    m_log.printEx("%s: error - flock not found.", __FUNCTION__);
                    return ::logic::common::InternalStatus::Int_NotFound;
                }

                m_log.print(std::string(__FUNCTION__) + ": info - change a flock's state on " + std::to_string((int)_newState));

                //
                // 1. Mark flock's entry with a new access state. Changes rely only to service's list.
                //

                if (m_storage.setStateById(_flockId, _newState))
                {
                    // Success, guys!
                    m_log.print(std::string(__FUNCTION__) + ": info - flock's state was successfully changed.");

                    //
                    // 2. Update driver's list and do not refresh driver's cache.
                    //

                    auto driverResponse = sendFlocksToDriver(false); // merge;

                    if (IntSuccess(driverResponse))
                    {
                        m_log.printEx("%s: success - flocks list also was updated in driver.", __FUNCTION__);
                    }
                    else
                    {
                        m_log.printEx("%s: success - flocks list updated in driver, internal error code %d.", __FUNCTION__, driverResponse);
                    }

                    return driverResponse;
                }
                else
                {
                    m_log.printEx("%s: error - could not change state for a flock the entry .", __FUNCTION__);

                    return ::logic::common::InternalStatus::Int_UnknownError;
                }

                return ::logic::common::InternalStatus::Int_UnknownError;
            }

            logic::common::InternalStatus FileGuard::clear()
			{
				m_log.print(std::string(__FUNCTION__));

				//
				//	Clear flocks list of current service application and do the same in driver.
				//

				if (m_storage.clear())
				{
					auto driverResponse = sendFlocksToDriver(false);

					if (IntSuccess(driverResponse))
					{
						m_log.printEx("%s: success - flocks list also was updated in driver.", __FUNCTION__);
					}
					else
					{
						m_log.printEx("%s: success - flocks list updated in driver, internal error code is %d.", __FUNCTION__, driverResponse);
					}

					return driverResponse;
				}
				else
				{
					m_log.printEx("%s: error - could not remove flock entries in service part.", __FUNCTION__);
					return ::logic::common::InternalStatus::Int_UnknownError;
				}

				return ::logic::common::InternalStatus::Int_UnknownError;
			}

			bool FileGuard::verifyDriverConnection()
			{
				m_log.print(std::string(__FUNCTION__));

				bool established = m_driver.canConnect();
				if (!established)
				{
					m_log.print(std::string(__FUNCTION__) + ": error - have no driver connection.");
				}

				return established;
			}

			logic::common::InternalStatus FileGuard::sendFlocksToDriver(bool _refreshCache)
			{
				m_log.print(std::string(__FUNCTION__));

				//
				//	1. Get all entries only if the storage is loaded.
				//

				if (!m_storage.isLoaded())
				{
					m_log.print(std::string(__FUNCTION__) + ": error - flocks storage is not loaded.");
					return ::logic::common::InternalStatus::Int_NotLoaded;
				}

				auto flocks = m_storage.getAll();

				//
				//	Do not touch the driver if there is nothing to update.
				//

				if (flocks.empty())
				{
					m_log.print(std::string(__FUNCTION__) + ": info - there is nothing to update. Empty list.");
					return ::logic::common::InternalStatus::Int_Success;
				}

				//
				//	2. Clear all FLocks in kernel-mode driver.
				//
				//	Please note:
				//			It could be dangerous if we have more then one list of flocks.
				//			(The second flocks list could be a part of self-protection).
				//

				if (!m_driver.storageClear())
				{
					m_log.print(std::string(__FUNCTION__) + ": error - could not clear driver's flocks.");
				}

				int nFlock = 0;
				for (auto flock : flocks)
				{
					nFlock++;

					::driver::FLOCK_STORAGE_ENTRY lowLevelFlock = { 0 };
					lowLevelFlock.version = 0;
					memcpy(lowLevelFlock.id, flock.uniqueId, sizeof(flock.uniqueId));

					lowLevelFlock.flockFlag = getLowLevelPolicy(flock.state);

					m_log.print(std::string(__FUNCTION__) + ": send driver request - to add flock #" + std::to_string(nFlock));

                    //
					// Handle special trick for directories which includes files which should be hidden.
                    //

					if (lowLevelFlock.flockFlag & FLOCK_FLAG_HIDE)
					{
                        //
						//  Now we ignore that because we handle that case when user adds new FLock.
						//
					}

					m_log.print(std::string(__FUNCTION__) + ": send driver request - to add flock #" + std::to_string(nFlock) + " policy flag " + std::to_string(lowLevelFlock.flockFlag));

					if (m_driver.storageAdd(lowLevelFlock))
					{
						m_log.print(std::string(__FUNCTION__) + ": success - #" + std::to_string(nFlock) + " flock was added.");
					}
					else
					{
						m_log.print(std::string(__FUNCTION__) + ": error - #" + std::to_string(nFlock) + " flock was not added.");
					}
				}

				//
				//	3. Refresh kernel-mode cache and invalidate filter's contexts if require.
				//	That's important to do if we changed flock's meta somewhere on disk.
				//

				if (_refreshCache)
				{
					if (!refreshDriverCaches())
					{
						m_log.print(std::string(__FUNCTION__) + ": error - failed to refresh drv caches.");
					}
				}

				//
				//	4. Flush all changes to disk.
				//

				if (m_driver.storageFlush())
				{
					m_log.print(std::string(__FUNCTION__) + ": storage was flushed.");
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error - storage was not flushed.");
				}

				//
				//	Success. FLocks were transferred to driver.
				//	Thanks, God. 
				//

				return ::logic::common::InternalStatus::Int_Success;
			}

			logic::common::InternalStatus FileGuard::mergeFlocksWithDriver(bool _refreshCache)
            {
				m_log.print(std::string(__FUNCTION__));

				// 0. Get all entries only if the storage is loaded.
				if (!m_storage.isLoaded())
				{
					m_log.print(std::string(__FUNCTION__) + ": error - flocks storage is not loaded.");
					return ::logic::common::InternalStatus::Int_NotLoaded;
				}

				auto applicationFlocks = m_storage.getAll();

				// Do not touch the driver if there is nothing to update.
				if (applicationFlocks.empty())
				{
					m_log.print(std::string(__FUNCTION__) + ": info - there is nothing to update. Empty list.");
					return ::logic::common::InternalStatus::Int_Success;
				}

				//
				// 1. Get flocks from driver's list.
				//
				std::map<std::string /* flock id */, DWORD /* access policy */ > commonList;

				std::vector<::driver::FLOCK_STORAGE_ENTRY> currentDrvFlocks;
				if (!m_driver.storageGetAll(currentDrvFlocks))
				{
					m_log.print(std::string(__FUNCTION__) + ": error - failed to read drv flocks.");
					return ::logic::common::InternalStatus::Int_DriverNotConnected;
				}

				//
				//	2. Add flocks which we received from driver to our common temporary list of flocks.
				//

				for (auto drvFlock : currentDrvFlocks)
				{
					std::string flock_id((const char*)drvFlock.id, sizeof(drvFlock.id));
					unsigned long flockProtection = drvFlock.flockFlag;

					commonList[flock_id] = flockProtection;
				}

				//
				//	3. Handle flocks from user-mode service part and add them to all already present driver's flocks.
				//	Actually it is a merge operation. At this step we can update some old flock policies, save others and add new policies. 
				//

				for (auto flock : applicationFlocks)
				{
					std::string flock_id((const char*)flock.uniqueId, sizeof(flock.uniqueId));
					unsigned long flockProtection = getLowLevelPolicy(flock.state);

					commonList[flock_id] = flockProtection;
				}

				//
				//	3. Clear all FLocks in kernel-mode driver.
				//
				//	Please note:
				//			It could be dangerous if we have more then one list of flocks.
				//			(The second flocks list could be a part of self-protection).
				//
				if (!m_driver.storageClear())
				{
					m_log.print(std::string(__FUNCTION__) + ": error - could not clear driver's flocks.");
					return ::logic::common::InternalStatus::Int_Success;
				}

				//
				//	4. Send result list to kernel-mode driver.
				//

				for (auto flock : commonList)
				{
					::driver::FLOCK_STORAGE_ENTRY lowLevelFlock = { 0 };
					lowLevelFlock.version = 0;
					lowLevelFlock.flockFlag = flock.second; // Protection type.
					memcpy(lowLevelFlock.id, flock.first.data(), sizeof(lowLevelFlock.id)); // Unique ID.

					//
					//	Handle special trick for directories which includes files which should be hidden.
					//

					if (lowLevelFlock.flockFlag & FLOCK_FLAG_HIDE)
					{
						//
						//	In case of file hiding I prefer to mark parent directory in time of first flock creating.
						//	Nothing to do here. Just left that comment here as short notes.
						//
					}

					m_log.print(std::string(__FUNCTION__) + ": send driver request - to add flock #" + flock.first + " policy flag " + std::to_string(lowLevelFlock.flockFlag));

					if (m_driver.storageAdd(lowLevelFlock))
					{
						m_log.print(std::string(__FUNCTION__) + ": success - #" + flock.first + " flock was added.");
					}
					else
					{
						m_log.print(std::string(__FUNCTION__) + ": error - #" + flock.first + " flock was not added.");
					}
				}

                //
                //  It is not required to clear drivers caches because we do not change any EAs on disk!
                //
				// 5. Refresh kernel-mode cache.
				//if (!m_driver.cacheClear())
				//{
				//	m_log.print(std::string(__FUNCTION__) + ": failed to clear cache.");
				//}

				// 6. Flush all changes to disk.
				if (m_driver.storageFlush())
				{
					m_log.print(std::string(__FUNCTION__) + ": driver storage was flushed.");
				}
				else
				{
					m_log.print(std::string(__FUNCTION__) + ": error - driver storage was not flushed.");
				}

				// Success. Application flocks was merged with driver flocks and applied on a kernel-mode level.
				return ::logic::common::InternalStatus::Int_Success;
			}

			bool FileGuard::reloadStorage()
			{
				return m_storage.load();
			}

            bool FileGuard::isSupportedFs(std::wstring _path, bool& _supported)
            {
                auto fn = std::string(__FUNCTION__);

                std::wstring path = _path;

                auto pos = path.find(L':');
                if (pos == std::wstring::npos)
                {
                    m_log.print(fn + ": error - wrong file path is received.");
                    return false;
                }

                std::wstring volume = path.substr(0, pos + 1);

                if (std::wstring::npos == volume.find(L"\\??\\"))
                {
                    volume = L"\\??\\" + volume;
                }

                WCHAR tempfile[128] = { 0 };
                wsprintfW(tempfile, L"dgt-%d-%x-%x-%d.simple", rand(), rand(), rand(), rand());

                std::wstring tempFilePath = volume + L'\\' + tempfile;

                HANDLE hTempFile = CreateFileW(
                    tempFilePath.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                    NULL);

                if (hTempFile == INVALID_HANDLE_VALUE)
                {
                    auto lastError = GetLastError();
                    m_log.print(fn + ": error - failed to create temp file, last error is " + std::to_string(lastError));

                    return false;
                }

                CloseHandle(hTempFile);

                ::driver::FLOCK_META fm = { 0 };
                UCHAR metaSignature[] = FLOCK_META_SIGNATURE;
                memcpy(fm.signature, metaSignature, sizeof(metaSignature));

                if (_supported = m_driver.writeFileMeta(tempFilePath, fm))
                {
                    m_driver.makeMetaInvalid(tempFilePath);

                    DeleteFileW(tempFilePath.c_str());
                }
                else
                {
                    m_log.print(fn + ": warning - failed to write flock-meta, it means that used on volume file system is not supported.");
                }

                return true;
            }

            DWORD FileGuard::getLowLevelPolicy(::logic::folderlock::storage::FLockState _policy)
			{
				DWORD policy = 0;

				if (_policy == ::logic::folderlock::storage::FLockState::FLock_Locked)
				{
					policy |= FLOCK_FLAG_LOCK_ACCESS;
				}

				if (_policy == ::logic::folderlock::storage::FLockState::FLock_Hidden)
				{
					policy |= FLOCK_FLAG_HIDE;
				}

				if (_policy == ::logic::folderlock::storage::FLockState::FLock_HiddenAndLocked)
				{
					policy |= FLOCK_FLAG_LOCK_ACCESS | FLOCK_FLAG_HIDE;
				}

				return policy;
			}

			logic::common::InternalStatus FileGuard::markParentDirHasFLocks(std::wstring _path)
			{
				windir::removeLastSeparator(_path);
				if (_path.find(L"\\??\\") == std::string::npos)
				{
					_path = L"\\??\\" + _path; // Add prefix if it is not present. 
				}

				//
				//	Parent directory of the protected file system object should be marked with special flag - FLOCK_FLAG_HAS_FLOCKS.
				//	That flag helps our driver to improve common system performance and not to process IRP_MJ_DIRECTORY_CONTROL requests.
				//

				auto parentPath = _path;
				auto pos = parentPath.find_last_of(L'\\');

				if (pos == std::wstring::npos)
				{
					m_log.print(std::string(__FUNCTION__) + ": error - invalid path format for " + strings::ws_s(parentPath));
					return ::logic::common::InternalStatus::Int_InvalidFormat;
				}

				//
				//	Cut the string - take only parent directory path.
				//

				parentPath = parentPath.substr(0, pos);

				m_log.print(std::string(__FUNCTION__) + ": info - mark parent dir " + strings::ws_s(parentPath));

				//
				//	Before to mark parent directory path with FLOCK_META attributes, we should to verify presence of already exists attributes.
				//	Because we can replace old important information!
				//

				::driver::FLOCK_META oldFlockMeta = { 0 }, fmParentDirMark = { 0 };
				if (m_driver.readFileMeta(parentPath, oldFlockMeta))
				{
					m_log.print(std::string(__FUNCTION__) + ": info - parent dir already has flock-meta " + strings::ws_s(parentPath));

					//
					//	Use the old flock meta information but include FLOCK_FLAG_HAS_FLOCKS flag.
					//

					memcpy( &fmParentDirMark, &oldFlockMeta, sizeof(oldFlockMeta) );
					fmParentDirMark.flags |= FLOCK_FLAG_HAS_FLOCKS;
				}
				else
				{
					//
					//	Ok, parent directory has no any flock meta attributes.
                    //  We need to generate random flock id and add FLOCK_FLAG_HAS_FLOCKS flag.
					//
					
					UCHAR metaSignature[] = FLOCK_META_SIGNATURE;
					memcpy(fmParentDirMark.signature, metaSignature, sizeof(metaSignature));
                    generateFLockId(fmParentDirMark.uniqueId, sizeof(fmParentDirMark.uniqueId));
					fmParentDirMark.flags = FLOCK_FLAG_HAS_FLOCKS;
				}

				if (!m_driver.writeFileMeta(parentPath, fmParentDirMark))
				{
					m_log.print(std::string(__FUNCTION__) + ": critical error - parent dir was not marked with HAS_FLOCKS.");
					return ::logic::common::InternalStatus::Int_FLockWasNotSigned;
				}

				return ::logic::common::InternalStatus::Int_Success;
			}

			bool FileGuard::isVolume(std::wstring _path)
			{
				windir::removeLastSeparator(_path);

				auto parentPath = _path;
				auto pos = parentPath.find_last_of(L'\\');

				if (pos == std::wstring::npos)
				{
					return false;
				}
				else
				{
					parentPath = parentPath.substr(0, pos);

					//	"C:\dir\" -> "C:\dir" -> "C:" 

					if (parentPath.size() > 1)
					{
						return parentPath.at(parentPath.size() - 1) == L':';
					}
				}

				return false;
			}

			bool FileGuard::refreshDriverCaches()
			{
				bool res = true;

                m_driver.ctxReset();

				if (!m_driver.cacheClear())
				{
					res = false;

					m_log.print(std::string(__FUNCTION__) + ": error - failed to clear cache.");
				}

				if (!m_driver.ctxReset())
				{
					res = false;

					m_log.print(std::string(__FUNCTION__) + ": error - failed to reset ctxs.");
				}

				return res;
			}

			
			//
			//	Implementation of the ::logic::common::DgiCommonControl interface.
			//

			bool FileGuard::ctrInit()
			{
				return true;
			}

			bool FileGuard::ctrlLateInit()
			{
				return true;
			}

			bool FileGuard::ctrlIsRunning()
			{
				return true;
			}

			bool FileGuard::ctrlShutdown(bool _canWait)
			{
				return true;
			}

			std::string FileGuard::ctrlGetName()
			{
				return SYSTEM_FLOCK;
			}

			bool FileGuard::ctrlSetPassword(::logic::common::MasterPassword _password)
			{
				bool res = false;

				if (res = m_storage.setPassword(_password))
				{
					m_storage.load();
				}

				return res;
			}

			bool FileGuard::ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword)
			{
				m_log.print(std::string(__FUNCTION__));

				bool success = m_storage.reEncrypt(_password, _currentPassword);

				if (success)
				{
					this->ctrlSetPassword(_password);
				}

				return success;
			}
		}
	}
}
