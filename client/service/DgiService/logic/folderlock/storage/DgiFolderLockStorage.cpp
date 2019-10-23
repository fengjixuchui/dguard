//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiFolderLockStorage.h"
#include "../../../helpers/internal/helpers.h"
#include "../../../logic/common/DgiEngine.h"

namespace logic
{
	namespace folderlock
	{
		namespace storage
		{
			Keeper::Keeper(::logic::common::DgiCrypt& _dgiCrypt, logfile& _log, std::string _storageFile) :
				m_log(_log),
				m_storage(_storageFile),
				m_cryptor(_dgiCrypt),
				m_defaultHash(logic::common::HashAlgorithm::Hash_Md5),
				m_defaultCrypt(logic::common::CryptAlgorithm::CA_Grader)
			{
				//
				// Load all cards from file system storage.
				//
				bool loaded = loadEntries();
				if (!loaded)
				{
					logMessage(std::string(__FUNCTION__) + ": error - data wasn't loaded.");
				}
			}

			Keeper::~Keeper()
			{
				logMessage(std::string(__FUNCTION__));

				flushEntries();

				m_storage.flush();
				m_storage.close();

				logMessage(std::string(__FUNCTION__) + ": finished.");
			}

			bool Keeper::isLoaded()
			{
				std::unique_lock<std::mutex> lock(m_lock);

				return m_storage.isOpened();
			}

			bool Keeper::present(std::wstring _path)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				return this->isPresent(_path);
			}

			bool Keeper::presentById(std::string _flockId)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				// Return false immediately in case of wrong length of flock id.
				if (_flockId.length() != ::logic::folderlock::storage::FlockIdLength)
				{
					return false;
				}

				for (auto entry : m_entries)
				{
					FLockEntry fe = entry.second;
					FLockObject flock;

					if (decodeEntry(fe, flock))
					{
						if (memcmp(flock.uniqueId, _flockId.data(), sizeof(flock.uniqueId)) == 0)
						{
							return true;
						}
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't decode an entry");
					}
				}

				return false;
			}

			bool Keeper::add(FLockObject _flockobject)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
					return false;
				}

				std::wstring path = _flockobject.path;
				path = perform(path);

				//
				// Change letter-case to lower case before to add the flock to internal storage.
				//
				fill_wchars(_flockobject.path, path.c_str());

				if (!isPresent(path))
				{
					FLockEntry fe;
					if (encodeEntry(_flockobject, fe, m_defaultHash, m_defaultCrypt))
					{
						m_entries[path] = fe;

						if (flushEntries())
						{
                            logMessage(std::string(__FUNCTION__) + ": entries were flushed!");

                            return true;
						}
						else
						{
							logMessage(std::string(__FUNCTION__) + ": critical error - can't flush entries.");
						}
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't add because of encoding errors.");
					}
				}
				else
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't add an entry because it's already present.");
				}

				return false;
			}

            bool Keeper::change(std::wstring _path, const FLockObject& _changedFlock)
			{
				FLockEntry feOld;

				if (take(_path, feOld))
				{
					FLockEntry changedEncoded;

					// Encode new data using old cryptographic information.
					//
					if (encodeEntry(_changedFlock, changedEncoded, feOld.protection.usedHashAlgorithm, feOld.protection.usedCryptAlgorithm))
					{
						// Change old one to changed new one.
						//
						m_entries[_path] = changedEncoded;
						return true;
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't encode an entry");
					}
				}

				return false;
			}

			bool Keeper::setState(std::wstring _path, ::logic::folderlock::storage::FLockState _newState)
			{
                auto fn = std::string(__FUNCTION__);
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
                    logMessage(fn + ": error - some problems with storage...");
					return false;
				}

				_path = perform(_path);

				FLockObject flock;
				if (this->take(_path, flock))
				{
					flock.state = _newState;

					if (change(_path, flock))
					{
                        logMessage(fn + ": success - flock was changed in memory, it is require to flush on disk.");

                        bool flushed = this->flushEntries();

                        if (flushed)
                        {
                            logMessage(fn + ": success - data were flushed.");
                        }
                        else
                        {
                            logMessage(fn + ": error - data were not flushed.");
                        }

						return flushed;
					}
					else
					{
						logMessage(fn + ": error - can't change a flock");
					}
				}
				else
				{
					logMessage(fn + ": warning - flock path not found");
				}

				return false;
			}

            bool Keeper::setStateById(std::string _flockId, ::logic::folderlock::storage::FLockState _newState)
            {
                auto fn = std::string(__FUNCTION__);
                std::unique_lock<std::mutex> lock(m_lock);

                FLockObject changedFLock;
                FLockEntry decodedFlockEntry, changedAndEncodedEntry;

                if (!isOk())
                {
                    logMessage(fn + ": error - some problems with storage...");

                    return false;
                }

                if (takeEntryById(_flockId, decodedFlockEntry))
                {
                    changedFLock = decodedFlockEntry.flockObject;
                    changedFLock.state = _newState;

                    //
                    //  Encode new changed flock using old encryption info (hash and crypt alg).
                    //

                    if (encodeEntry(changedFLock, changedAndEncodedEntry, decodedFlockEntry.protection.usedHashAlgorithm, decodedFlockEntry.protection.usedCryptAlgorithm))
                    {
                        //
                        //  Save new changed flock in memory storage.
                        //

                        std::wstring flockFilePath(changedFLock.path);

                        m_entries[flockFilePath] = changedAndEncodedEntry;

                        //
                        //  Flush all last changes on disk.
                        //

                        logMessage(fn + ": success - flock was changed in memory, it is require to flush on disk.");

                        bool flushed = this->flushEntries();

                        if (flushed)
                        {
                            logMessage(fn + ": success - data were flushed.");
                        }
                        else
                        {
                            logMessage(fn + ": error - data were not flushed.");
                        }

                        return flushed;
                    }
                    else
                    {
                        logMessage(std::string(__FUNCTION__) + ": error - can't encode an entry");
                    }
                }

                return false;
            }

            bool Keeper::unlock(std::wstring _path)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
					return false;
				}

				_path = perform(_path);

				FLockObject flock;
				if (this->take(_path, flock))
				{
					flock.state = FLock_Unlocked;

					if (change(_path, flock))
					{
						return true;
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't change a flock");
					}
				}
				else
				{
					logMessage(std::string(__FUNCTION__) + ": warning - flock path not found");
				}

				return false;
			}

			bool Keeper::isLocked(std::wstring _path)
			{
				std::unique_lock<std::mutex> lock(m_lock);
				
				if (!isOk())
				{
					return false;
				}

				FLockObject flock;
				if (this->take(_path, flock))
				{
					return flock.state == FLock_Locked;
				}

				return false;
			}

			bool Keeper::getInfo(std::wstring _path, FLockObject& _outInfo)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
					return false;
				}

// 				if (!this->isPresent(_path))
// 				{
// 					return false;
// 				}

				_path = this->perform(_path);

				auto pos = m_entries.find(_path);

				if (pos != m_entries.end())
				{
					if (decodeEntry(pos->second, _outInfo))
					{
						return true;
					}
					else
					{
						logMessage( std::string(__FUNCTION__) + ": error - can't decode an entry");
					}
				}

				return false;
			}

            bool Keeper::getInfoById(std::string _flockId, FLockObject& _outInfo)
            {
                std::unique_lock<std::mutex> lock(m_lock);

                if (!isOk())
                {
                    return false;
                }

                for (auto entry : m_entries)
                {
                    FLockObject tempObject;

                    if (decodeEntry(entry.second, tempObject))
                    {
                        auto foundFlockId = std::string((const char*)tempObject.uniqueId, sizeof(tempObject.uniqueId));

                        if (foundFlockId == _flockId)
                        {
                            _outInfo = tempObject;
                            return true;
                        }
                    }
                    else
                    {
                        logMessage(std::string(__FUNCTION__) + ": error - can't decode an entry.");
                    }
                }

                logMessage(std::string(__FUNCTION__) + ": warning - requested entry is not found " + _flockId);

                return false;
            }

            std::vector<FLockObject> Keeper::getAll()
			{
				FLockEntryMap copyMap;

				// Take a copy and work with it.
				m_lock.lock();
				copyMap = m_entries;
				m_lock.unlock();

				std::vector<FLockObject> decodedEntries;

				for (auto i : copyMap)
				{
					FLockObject flockObj;

					if (decodeEntry(i.second, flockObj))
					{
						decodedEntries.push_back(flockObj);
					}
					else
					{
						logMessage( std::string(__FUNCTION__) + ": error - can't decode entry, skip an error");
					}
				}

				return decodedEntries;
			}

			unsigned long Keeper::length()
			{
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
					return false;
				}

				return m_entries.size();
			}

			bool Keeper::remove(std::wstring _path)
			{
				bool result = false;
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
					return false;
				}

				// Do preparations
				_path = perform(_path);

				// erase the flock object from memory and flush all current entries to file.
				m_entries.erase(_path);

				// flush all changes
				result = flushEntries();

				return result;
			}

			bool Keeper::removeById(std::string _flockId)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				// Return false immediately in case of wrong length of flock id.
				if (_flockId.length() != sizeof(::logic::folderlock::storage::FlockIdLength))
				{
					return false;
				}

				// Enumerate all entries and remove the one.
				for (auto entry : m_entries)
				{
					FLockEntry fe = entry.second;
					FLockObject flock;

					if (decodeEntry(fe, flock))
					{
						if (memcmp(flock.uniqueId, _flockId.data(), sizeof(flock.uniqueId)) == 0)
						{
							// Do preparations
							std::wstring flockPath = perform(entry.first);

							// Erase the flock object from memory and flush all current entries to file.
							m_entries.erase(flockPath);

							// Flush all changes.
							bool removed = flushEntries();

							if (removed)
							{
								logMessage(std::string(__FUNCTION__) + ": success - flock was removed.");
							} else {
								logMessage(std::string(__FUNCTION__) + ": error - flock was not removed, flushing problems.");
							}

							return removed;
						}
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't decode an entry");
					}
				}

				return false;
			}

			bool Keeper::clear()
			{
				std::unique_lock<std::mutex> lock(m_lock);

				if (!isOk())
				{
					return false;
				}

				m_entries.clear();

				return flushEntries();
			}

			::logic::common::HashAlgorithm Keeper::getAlgHash() const
			{
				std::unique_lock<std::mutex> lock(m_lock);

				return m_defaultHash;
			}

			::logic::common::CryptAlgorithm Keeper::getAlgCrypt() const
			{
				std::unique_lock<std::mutex> lock(m_lock);

				return m_defaultCrypt;
			}

			bool Keeper::setAlgHash(::logic::common::HashAlgorithm _alg)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				bool changed = false;
				if (changed = m_cryptor.isSupported(_alg))
				{
					m_defaultHash = _alg;
				}
				else
				{
					logMessage(std::string(__FUNCTION__) + ": error - is not supported, algorithm code is " + std::to_string((int)_alg));
				}

				return changed;
			}

			bool Keeper::setAlgCrypt(::logic::common::CryptAlgorithm _alg)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				bool changed = false;
				if (changed = m_cryptor.isSupported(_alg))
				{
					m_defaultCrypt = _alg;
				}
				else
				{
					logMessage(std::string(__FUNCTION__) + ": error - is not supported, algorithm code is " + std::to_string((int) _alg ) );
				}

				return changed;
			}

			bool Keeper::setPassword(common::MasterPassword _password)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				if (!_password.isSet())
				{
					return false;
				}

				m_masterPassword = _password;
				return true;
			}

			bool Keeper::load()
			{
				logMessage(std::string(__FUNCTION__) + ": an attempt to load entries..");

				bool loaded = loadEntries();
				if (loaded)
				{
					logMessage(std::string(__FUNCTION__) + ": success - storage was loaded.");
				}
				else
				{
					logMessage(std::string(__FUNCTION__) + ": error - storage was not loaded.");
				}

				return loaded;
			}

			bool Keeper::reEncrypt(::logic::common::MasterPassword _newPassword, ::logic::common::MasterPassword _currentPassword)
			{
				std::unique_lock<std::mutex> lock(m_lock);

				bool canFlush = true;

				if (isOk())
				{
					FLockEntryMap modified;

					// Make temporary copy.
					auto copyEntries = m_entries;

					for (auto entry : copyEntries)
					{
						FLockObject flockInfo;
						if (decodeEntry(entry.second, flockInfo, _currentPassword))
						{
							FLockEntry newEncoded;
							if (encodeEntry(flockInfo, newEncoded, entry.second.protection.usedHashAlgorithm, entry.second.protection.usedCryptAlgorithm, _newPassword))
							{
								modified[entry.first] = newEncoded;
							}
							else
							{
								// canFlush = false;
								logMessage(std::string(__FUNCTION__) + ": error - can't encode data with new password.");
							}

							ZeroMemory(&flockInfo, sizeof flockInfo);
						}
						else
						{
							//canFlush = false;
							logMessage(std::string(__FUNCTION__) + ": error - can't decode entry.");
						}
					}

					if (canFlush)
					{
						// Change old entries on new-encoded entries.
						m_entries = modified;

						if (!flushEntries())
						{
							logMessage(std::string(__FUNCTION__) + ": critical error - can't flush re-encoded entries, roll back entries.");
							m_entries = copyEntries;
							return false;
						}

						return true;
					}
				}

				return false;
			}

			bool Keeper::isOk()
			{
				// Is file open? 
				//
				if (!m_storage.isOpened())
				{
					logMessage(std::string(__FUNCTION__) + ": error - the storage isn't open.");

					// Reload if not.
					//
					if (!m_storage.reload())
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't reload storage.");

						// It is an error. File should be loaded correctly.
						//
						return false;
					}
					else
					{
						if (!loadEntries())
						{
							logMessage(std::string(__FUNCTION__) + ": error - can't load storage.");

							// If it couldn't load information - may be it is a problem with their decoding.
							//
							return false;
						}
					}
				}

				if (m_entries.empty())
				{
					logMessage(std::string(__FUNCTION__) + ": entries list is empty, load entries from storage.");

					if (!loadEntries())
					{
						logMessage(std::string(__FUNCTION__) + ": error - could not load entries from storage.");
					}
				}

				return true;
			}

			bool Keeper::isPresent(std::wstring _path)
			{
				_path = this->perform(_path);

				//
				// It's a basic validator. Better to do more complex verification for skipping 
				// wide-spaces and any other symbols.
				//
				return (m_entries.count(_path) != 0);
			}

            bool Keeper::findById(std::string _flockId, FLockObject& _outFlock)
            {
                //std::unique_lock<std::mutex> lock(m_lock);

                for (auto entry : m_entries)
                {
                    FLockObject tempObject;

                    if (decodeEntry(entry.second, tempObject))
                    {
                        auto foundFlockId = std::string((const char*)tempObject.uniqueId, sizeof(tempObject.uniqueId));

                        if (foundFlockId == _flockId)
                        {
                            _outFlock = tempObject;
                            return true;
                        }
                    }
                    else
                    {
                        logMessage(std::string(__FUNCTION__) + ": error - can't decode an entry.");
                    }
                }

                logMessage(std::string(__FUNCTION__) + ": warning - requested entry is not found " + _flockId);

                return false;
            }

            bool Keeper::take(std::wstring _path, FLockEntry& _outEntry)
			{
				_path = perform(_path);

				if (isPresent(_path))
				{
					_outEntry = m_entries.at(_path);
					return true;
				}

				return false;
			}

            bool Keeper::takeEntryById(std::string _id, FLockEntry& _outDecodedEntry)
            {
                for (auto entry : m_entries)
                {
                    FLockObject tempObject;

                    if (decodeEntry(entry.second, tempObject))
                    {
                        auto foundFlockId = std::string((const char*)tempObject.uniqueId, sizeof(tempObject.uniqueId));

                        if (foundFlockId == _id)
                        {
                            _outDecodedEntry.protection = entry.second.protection;
                            _outDecodedEntry.flockObject = tempObject;

                            return true;
                        }
                    }
                    else
                    {
                        logMessage(std::string(__FUNCTION__) + ": error - can't decode an entry.");
                    }
                }

                logMessage(std::string(__FUNCTION__) + ": warning - requested entry is not found " + _id);

                return false;
            }

			bool Keeper::take(std::wstring _path, FLockObject& _outFlock)
			{
				FLockEntry fe;
				_path = perform(_path);

				if (isPresent(_path))
				{
					fe = m_entries.at(_path);

					if (decodeEntry(fe, _outFlock))
					{
						return true;
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't decode entry");
					}
				}

				return false;
			}

			std::wstring Keeper::perform(std::wstring _path)
			{
				// Keep all paths in a lower case.
				//
				strings::toLower(_path);

				return _path;
			}

			bool Keeper::decodeEntry(const FLockEntry& _entry, FLockObject& _resultInfo)
			{
				auto mp = m_masterPassword;

				if (!mp.isSet())
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't work because master-password isn't set.");
					return false;
				}

				return decodeEntry(_entry, _resultInfo, mp);
			}

			bool Keeper::decodeEntry(const FLockEntry& _entry, FLockObject& _resultInfo, ::logic::common::MasterPassword _masterPassword)
			{
				std::string funcname = std::string(__FUNCTION__);
				FLockEntry entryCopy = _entry;

				if (entryCopy.protection.passwordSize == 0)
				{
					logMessage(funcname + ": error - can't process the entry because encryption key has zero-size.");
					return false;
				}

				//
				// At first, we need to know - Do we support all used cryptographical algorithms?
				//
				bool supportedHashes = m_cryptor.isSupported(entryCopy.protection.usedHashAlgorithm) &&
					m_cryptor.isSupported(entryCopy.protection.usedCryptAlgorithm);

				if (!supportedHashes)
				{
					logMessage(funcname + ": error - can't decode entry, unsupported algorithm.");

					if (!m_cryptor.isSupported(entryCopy.protection.usedCryptAlgorithm))
					{
						logMessage(funcname + ": error - unsupported encoding algorithm - " + std::to_string(entryCopy.protection.usedCryptAlgorithm));
					}
					else if (!m_cryptor.isSupported(entryCopy.protection.usedHashAlgorithm))
					{
						logMessage(funcname + ": error - unsupported hashing algorithm algorithm - " + std::to_string(entryCopy.protection.usedHashAlgorithm));
					}

					return false;
				}

				::logic::common::Hasher* hasher = m_cryptor.get(entryCopy.protection.usedHashAlgorithm);
				::logic::common::SymCryptor* symCryptor = m_cryptor.get(entryCopy.protection.usedCryptAlgorithm);

				if (!(hasher && symCryptor))
				{
					return false;
				}

				//
				// 1. Decode '.encodedPassword' field with current user password.
				// 2. Calculate hash of just decoded password.
				// 3. Compare two hashes and continue if it is ok.
				//

				std::string passwordBuffer;
				if (!_masterPassword.getPasswordBuffer(passwordBuffer))
				{
					logMessage(funcname + ": error - can't decrypt because can't get password buffer.");
					return false;
				}

				bool decrypted = symCryptor->decrypt(entryCopy.protection.passwordEncoded,
					entryCopy.protection.passwordSize /*sizeof(entryCopy.protection.passwordEncoded)*/,
					passwordBuffer.c_str(),
					passwordBuffer.size());

				common::MasterPassword::secureErase(passwordBuffer);

				if (!decrypted)
				{
					logMessage(funcname + ": error - can't decrypt password.");
					return false;
				}

				std::string decodedPassword = getEncodedPassword(entryCopy.protection);
				std::string hashOfPassword = hasher->getHash(decodedPassword.data(), decodedPassword.size());

				SET_LAST_ZERO_CHAR(entryCopy.protection.passwordHash);
				std::string storedPasswordHash = entryCopy.protection.passwordHash;

				if (hashOfPassword != storedPasswordHash)
				{
					logMessage(funcname + ": error - password hashes are not equal.");
					return false;
				}

				//
				// Decrypt body of the structure with just decrypted password.
				// (Do the same steps as we did earlier.)
				//

				decrypted = symCryptor->decrypt((char*)&entryCopy.flockObject, sizeof(entryCopy.flockObject), decodedPassword.c_str(), decodedPassword.size());

				if (!decrypted)
				{
					logMessage(funcname + ": error - can't decrypt body-part.");
					return false;
				}

				//
				// Calculate hash of just decrypted data and compare it with stored hash in CardProtect section.
				//

				SET_LAST_ZERO_CHAR(entryCopy.protection.dataHash);
				std::string storedDataHash = entryCopy.protection.dataHash;
				std::string recalculateHash = hasher->getHash((char*)&entryCopy.flockObject, sizeof(entryCopy.flockObject));

				if (recalculateHash == storedDataHash)
				{
					_resultInfo = entryCopy.flockObject;

					// Erase data by security issues.
					decodedPassword.clear();
					ZeroMemory(&entryCopy, sizeof(entryCopy));

					return (true);
				}
				else
				{
					logMessage(funcname + ": error - recalculated hashes not matched.");
				}

				return false;
			}

			bool Keeper::encodeEntry(const FLockObject& _flockInfo, FLockEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt)
			{
				// Use current master-password.
				auto mp = m_masterPassword;

				if (!mp.isSet())
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't work because master-password isn't set.");
					return false;
				}

				return encodeEntry(_flockInfo, _resultEntry, _hash, _crypt, mp);
			}

			bool Keeper::encodeEntry(const FLockObject& _flockInfo, FLockEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt, ::logic::common::MasterPassword _masterPassword)
			{
				std::string funcname = std::string(__FUNCTION__);
				FLockEntry entry;

				entry.protection.usedCryptAlgorithm = _crypt;
				entry.protection.usedHashAlgorithm = _hash;

				//
				// At first, we need to know - Do we support all used crypto algorithms?
				//
				bool supportedCryptography = m_cryptor.isSupported(_hash) && m_cryptor.isSupported(_crypt);

				if (!supportedCryptography)
				{
					logMessage(funcname + ": error - advised cryptography algorithms are not supported.");

					if (!m_cryptor.isSupported(entry.protection.usedCryptAlgorithm))
					{
						logMessage(funcname + ": error - unsupported encoding algorithm - " + std::to_string(entry.protection.usedCryptAlgorithm));
					}
					else if (!m_cryptor.isSupported(entry.protection.usedHashAlgorithm))
					{
						logMessage(funcname + ": error - unsupported hashing algorithm algorithm - " + std::to_string(entry.protection.usedHashAlgorithm));
					}

					return false;
				}

				::logic::common::Hasher* hasher = m_cryptor.get(entry.protection.usedHashAlgorithm);
				::logic::common::SymCryptor* symCryptor = m_cryptor.get(entry.protection.usedCryptAlgorithm);

				if (!(hasher && symCryptor))
				{
					logMessage(funcname + ": error - hasher && symCryptor failed.");
					return false;
				}

				std::string hashOfData = hasher->getHash((const char*)&_flockInfo, sizeof(_flockInfo));
				if (hashOfData.empty())
				{
					logMessage(funcname + ": error - can't calculate hash of data.");
					return false;
				}

				// Save hash of original data.
				fill_chars(entry.protection.dataHash, hashOfData.c_str());

				//
				// Encode original data using current valid user password.
				//
				std::string passwordBuffer;
				if (!_masterPassword.getPasswordBuffer(passwordBuffer))
				{
					logMessage(funcname + ": error - can't get password's buffer.");
					return false;
				}

				entry.flockObject = _flockInfo;
				bool encoded = symCryptor->encrypt((char*)&entry.flockObject, sizeof(entry.flockObject), passwordBuffer.c_str(), passwordBuffer.size());
				if (!encoded)
				{
					ZeroMemory(&entry, sizeof entry);
					logMessage(funcname + ": error - can't encode data-part.");
					common::MasterPassword::secureErase(passwordBuffer);
					return false;
				}

				//
				// Calculate hash of used for encryption password.
				//
				std::string hashOfUsedPassword = hasher->getHash(passwordBuffer.c_str(), passwordBuffer.size());
				if (hashOfData.empty())
				{
					logMessage(funcname + ": error - can't calculate hash of data.");
					common::MasterPassword::secureErase(passwordBuffer);
					return false;
				}

				// Save hash of used for encryption password.
				//
				fill_chars(entry.protection.passwordHash, hashOfUsedPassword.c_str());

				//
				// Save encoded password which was used for encryption of original information.
				//
				//fill_chars(entry.protection.passwordEncoded, passwordBuffer.c_str());
				::logic::common::setProtPassword(entry.protection, passwordBuffer);

				//
				// Hide just written password through encoding it.
				//
				encoded = symCryptor->encrypt((char*)entry.protection.passwordEncoded,
					entry.protection.passwordSize /*sizeof(entry.protection.passwordEncoded)*/,
					passwordBuffer.c_str(),
					passwordBuffer.size());
				
				// Remove master-password's data.
				common::MasterPassword::secureErase(passwordBuffer);

				if (encoded)
				{
					_resultEntry = entry;
					ZeroMemory(&entry, sizeof entry);
					return true;
				}
				else
				{
					ZeroMemory(&entry, sizeof entry);
					logMessage(funcname + ": error - can't encode encryption key.");
					return false;
				}
			}

			bool Keeper::loadEntries()
			{
				bool noCriticalErrors = true;

				if (!m_storage.isOpened())
				{
					if (!m_storage.reload())
					{
						return false;
					}
				}

				FLockEntryList entries;
				m_storage.fill_vector(m_storage.count(), entries);

				// What we read now from disk will be here.
				//
				FLockEntryMap justReadEntries;

				for (auto entry : entries)
				{
					FLockObject decodedEntry;
					bool decoded = decodeEntry(entry, decodedEntry);
					if (decoded)
					{
						// It will be better to add one additional verification
						// for the last terminated zero symbol in the string.
						//
						std::wstring flockPath = decodedEntry.path;

						// Add just decoded entry to common list.
						//
						justReadEntries[flockPath] = entry;

						// Erase decrypted data from memory by security issues.
						//
						ZeroMemory(&decodedEntry, sizeof decodedEntry);
					}
					else
					{
						// Print error message about problems with decoding.
						//
						logMessage(std::string(__FUNCTION__) + ": error - there was a problem with decoding.");
					}
				}

				if (noCriticalErrors)
				{
					m_entries.swap(justReadEntries);
					justReadEntries.clear();
					return true;
				}

				return false;
			}

			bool Keeper::flushEntries()
			{
				bool noCriticalErrors = true;

				if (!m_storage.isOpened())
				{
					if (!m_storage.reload())
					{
						return false;
					}
				}

				// Save all current entries for a while. 
				// May be later we will need restore just removed.
				//
				FLockEntryList reservedEntries;
				m_storage.fill_vector(m_storage.count(), reservedEntries);

				if (m_storage.clear())
				{
					for (auto entry : m_entries)
					{
						if (m_storage.push_back(entry.second))
						{
							// Ok. Entry in storage.
						}
						else
						{
							// An error. Something went wrong, we couldn't add entry in file-storage.
							//
							logMessage(std::string(__FUNCTION__) + ": error - we couldn't add entry to storage.");

							// Here I should register a critical error.
							//
							noCriticalErrors = false;
							break;
						}
					}
				}
				else
				{
					return false;
				}

				if (noCriticalErrors)
				{
					if (!m_storage.flush())
					{
						logMessage(std::string(__FUNCTION__) + ": critical error - can't flush storage data.");
					}
				}
				else
				{
					// Roll back all entries.
					//
					if (m_storage.clear())
					{
						for (auto e : reservedEntries)
						{
							if (!m_storage.push_back(e))
							{
								// It's something critical. May be no free space on a hard drive, in vm and etc.
								//
								logMessage(std::string(__FUNCTION__) + ": critical error - can't add entry in storage while rollback.");
								return false;
							}
						}

						m_storage.flush();
					}
					else
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't clear storage while rollback operation.");
					}
				}

				return noCriticalErrors;
			}

			void Keeper::logMessage(const std::string& _msg)
			{
				m_log.print(_msg);
			}

		}
	}
}
