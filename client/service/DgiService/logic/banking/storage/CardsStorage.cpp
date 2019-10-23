//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "CardsStorage.h"
#include "../../../logic/common/DgiEngine.h"
#include "../../../helpers/internal/helpers.h"

namespace logic
{
	namespace banking
	{
		namespace storage
		{
			CardsKeeper::CardsKeeper(::logic::common::DgiCrypt& _dgiCrypt, logfile& _log, std::string _storageFile) :
				m_log(_log),
				m_crypt(_dgiCrypt),
				m_storageFile(_storageFile),
				m_storage(_storageFile),
				m_defaultHash(::logic::common::Hash_Md5),
				m_defaultCrypt(::logic::common::CA_Grader)
			{
				//
				// Load all cards from file system storage.
				//
// 				bool loaded = loadCards();
// 				if (!loaded)
// 				{
// 					logMessage(std::string(__FUNCTION__) + ": error - data wasn't loaded.");
// 				}
			}


			CardsKeeper::~CardsKeeper()
			{
				logMessage(std::string(__FUNCTION__));

				flushCards();

				m_storage.flush();
				m_storage.close();

				logMessage(std::string(__FUNCTION__) + ": finished.");
			}

			bool CardsKeeper::add(const CardInfo& _card)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				if (!isOk())
				{
					return false;
				}

				std::string cardNumber = _card.cardNumber;

				if (isPresent(cardNumber))
				{
					//
					// The card already present in Wallet.
					//
					logMessage(std::string(__FUNCTION__) + ": error - card already present.");

					return false;
				}

				CardEntry entry;
				if (!encodeEntry(_card, entry, m_defaultHash, m_defaultCrypt))
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't encode new entry.");
					return false;
				}

				m_entries[cardNumber] = entry;

				if (!flushCards())
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't flush data.");
					m_entries.erase(cardNumber);
					return false;
				}

				return true;
			}

			void CardsKeeper::remove(const CardInfo& _card)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				if (!isOk())
				{
					return;
				}

				std::string cardNumber = _card.cardNumber;

				cardNumber = this->perform(cardNumber);

				//
				// Remove the card from in-memory list and flush that modified list on disk.
				//
				m_entries.erase(cardNumber);

				//
				// Flush current cards list on disk.
				//
				if (!flushCards())
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't flush data.");
				}
			}

			void CardsKeeper::remove(std::string _cardNumber)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				if (!isOk())
				{
					return;
				}

				m_entries.erase(_cardNumber);

				if (!flushCards())
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't flush data.");
				}
			}

			bool CardsKeeper::present(std::string _cardNumber)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				if (!isOk())
				{
					return false;
				}

				return isPresent(_cardNumber);
			}

			bool CardsKeeper::isOk()
			{
				//
				// Is file open? 
				//
				if (!m_storage.isOpened())
				{
					logMessage(std::string(__FUNCTION__) + ": error - the storage isn't open.");

					//
					// Reload if not.
					//
					if (!m_storage.reload())
					{
						logMessage(std::string(__FUNCTION__) + ": error - can't reload storage.");

						//
						// It is an error. File should be loaded correctly.
						//
						return false;
					}
					else
					{
						if (!loadCards())
						{
							logMessage(std::string(__FUNCTION__) + ": error - can't load storage.");

							//
							// If it couldn't load cards information - may be it is a problem with their decoding.
							//
							return false;
						}
					}
				}
				else
				{
					if (m_entries.empty())
					{
						logMessage(std::string(__FUNCTION__) + ": entries list is empty, load entries from storage.");

						if (!loadCards())
						{
							logMessage(std::string(__FUNCTION__) + ": error - could not load entries from storage.");
						}
					}
				}

				return true;
			}

			bool CardsKeeper::loadCards()
			{
				bool everythingOk = true;

				if (!m_storage.isOpened())
				{
					logMessage(std::string(__FUNCTION__) + ": error - storage is not opened yet.");

					if (!m_storage.reload())
					{
						logMessage(std::string(__FUNCTION__) + ": error - could not reload storage.");

						return false;
					}
				}

				CardEntryList entries;
				m_storage.fill_vector(m_storage.count(), entries);

				//
				// What we read now from disk will be here.
				//
				CardEntryMap justReadEntries;

				for (auto entry : entries)
				{
					CardInfo ci;
					bool decoded = decodeEntry(entry, ci);
					if (decoded)
					{
						//
						// It will be better to add one additional verification
						// for the last terminated zero symbol in card number array.
						//
						std::string cardNumber = ci.cardNumber;

						//
						// Add just decoded card to common list of all available in Wallet cards.
						//
						justReadEntries[cardNumber] = entry;

						//
						// Erase decrypted data from memory by security issues.
						//
						ZeroMemory(&ci, sizeof ci);
					}
					else
					{
						// Print error message about problems with decoding.
						logMessage(std::string(__FUNCTION__) + ": error - there was a problem with decoding.");
					}
				}

				if (everythingOk)
				{
					m_entries.swap(justReadEntries);
					justReadEntries.clear();
					return true;
				}

				return false;
			}

			bool CardsKeeper::flushCards()
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
				CardEntryList tmpEntries;
				m_storage.fill_vector(m_storage.count(), tmpEntries);

				if ( m_storage.clear() )
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
						for (auto e : tmpEntries)
						{
							if (!m_storage.push_back(e))
							{
								// It's something critical. May be no free space on a hard drive, in vm and etc.
								//
								logMessage(std::string(__FUNCTION__) + ": error - can't add entry in storage while rollback.");
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

			bool CardsKeeper::get(std::string _cardNumber, CardInfo& _cardInfo)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				if (isOk())
				{
					CardEntry entry;
					if (take(_cardNumber, entry))
					{
						CardInfo ci;
						if (decodeEntry(entry, ci))
						{
							_cardInfo = ci;

							ZeroMemory(&ci, sizeof ci);

							return true;
						}
						else
						{
							logMessage(std::string(__FUNCTION__) + ": error - can't decode an entry.");
						}
					}
				}

				return false;
			}

			bool CardsKeeper::getAll(std::vector<CardInfo>& _cards)
			{
				bool noCriticalError = true;

				std::unique_lock<std::mutex> lock(m_lockCards);

				if (isOk())
				{
					for (auto entry : m_entries)
					{
						CardInfo ci;
						if (decodeEntry(entry.second, ci))
						{
							_cards.push_back(ci);
							ZeroMemory(&ci, sizeof ci);
						}
						else
						{
							logMessage(std::string(__FUNCTION__) + ": error - can't decode entry.");
						}
					}

					if (noCriticalError)
					{
						return true;
					}
				}

				return false;
			}

			bool CardsKeeper::isPresent(std::string _cardNumber)
			{
				//
				// It's a basic validator. Better to do more complex verification for skipping 
				// wide-spaces and any other symbols.
				//
				return (m_entries.count(_cardNumber) != 0);
			}

			bool CardsKeeper::take(std::string _cardNumber, CardEntry& _outCard)
			{
				_cardNumber = perform(_cardNumber);

				if (isPresent(_cardNumber))
				{
					_outCard = m_entries[_cardNumber];
					return true;
				}

				return false;
			}

			std::string CardsKeeper::perform(std::string _cardNumber)
			{
				//
				// Nothing to perform.
				//
				return _cardNumber;
			}

			bool CardsKeeper::decodeEntry(const CardEntry& _entry, CardInfo& _resultInfo)
			{
				std::string funcname = std::string(__FUNCTION__);

				auto mp = m_masterPassword;

				if(! mp.isSet() )
				{
					logMessage(funcname + ": error - can't work because master-password isn't set.");
					return false;
				}

				return decodeEntry(_entry, _resultInfo, mp);
			}

			bool CardsKeeper::decodeEntry(const CardEntry& _entry, CardInfo& _resultInfo, ::logic::common::MasterPassword _masterPassword)
			{
				std::string funcname = std::string(__FUNCTION__);
				CardEntry entry = _entry;

				if (entry.protection.passwordSize == 0)
				{
					logMessage(funcname + ": error - can't process the entry because encryption key has zero-size.");
					return false;
				}

				if (!_masterPassword.isSet())
				{
					logMessage(funcname + ": error - can't work because master-password isn't set.");
					return false;
				}

				//
				// At first, we need to know - Do we support all used cryptographical algorithms?
				//
				bool supportedHashes = m_crypt.isSupported(entry.protection.usedHashAlgorithm) &&
					m_crypt.isSupported(entry.protection.usedCryptAlgorithm);

				if (!supportedHashes)
				{
					logMessage(funcname + ": error - can't decode entry, unsupported algorithm.");

					if (!m_crypt.isSupported(entry.protection.usedCryptAlgorithm))
					{
						logMessage(funcname + ": error - unsupported encoding algorithm - " + std::to_string(entry.protection.usedCryptAlgorithm));
					}
					else if (!m_crypt.isSupported(entry.protection.usedHashAlgorithm))
					{
						logMessage(funcname + ": error - unsupported hashing algorithm algorithm - " + std::to_string(entry.protection.usedHashAlgorithm));
					}

					return false;
				}

				::logic::common::Hasher* hasher = m_crypt.get(entry.protection.usedHashAlgorithm);
				::logic::common::SymCryptor* symCryptor = m_crypt.get(entry.protection.usedCryptAlgorithm);

				if (!(hasher && symCryptor))
				{
					return false;
				}

				//
				// Decode 'CardProtect.encodedPassword' field with current user password.
				// Calculate hash of just decoded password.
				// Compare two hashes and continue if it is ok.
				//

				std::string passwordBuffer;
				if(!_masterPassword.getPasswordBuffer(passwordBuffer))
				{
					logMessage(funcname + ": error - can't decrypt because can't get password buffer.");
					return false;
				}

				bool decrypted = symCryptor->decrypt(entry.protection.passwordEncoded,
					entry.protection.passwordSize /*sizeof(entry.protection.passwordEncoded)*/, // decode not hole buffer 
					passwordBuffer.c_str(),
					passwordBuffer.size());

				common::MasterPassword::secureErase(passwordBuffer);

				if (!decrypted)
				{
					logMessage(funcname + ": error - can't decrypt password.");
					return false;
				}

				// !!!
				// Ошибка тут! Причина в том, что пароль хотим считывать как нуль терменированную строку,
				// но в нашем случае это wide-chars которые вторым символом всегда содержат нуль!
				//
				// 06.09.2018 update:
				//		Можно не считать данный подход неверным.
				//
				std::string decodedPassword = getEncodedPassword(entry.protection); // !!!
				std::string hashOfPassword = hasher->getHash(decodedPassword.c_str(), decodedPassword.size());

				if (decodedPassword.empty())
				{
					logMessage(funcname + ": error - decoded password is empty.");
					return false;
				}

				//
				// Можно в 
				//
				SET_LAST_ZERO_CHAR(entry.protection.passwordHash);
				std::string storedPasswordHash = entry.protection.passwordHash;

				if (hashOfPassword != storedPasswordHash)
				{
					logMessage(funcname + ": error - password hashes are not equal.");
					return false;
				}

				//
				// Decrypt body of the structure with just decrypted password.
				// (Do the same steps as we did earlier.)
				//

				decrypted = symCryptor->decrypt((char*)&entry.info, sizeof(entry.info), decodedPassword.c_str(), decodedPassword.size());

				if (!decrypted)
				{
					logMessage(funcname + ": error - can't decrypt body-part.");
					return false;
				}

				//
				// Calculate hash of just decrypted data and compare it with stored hash in CardProtect section.
				//

				SET_LAST_ZERO_CHAR(entry.protection.dataHash);
				std::string storedDataHash = entry.protection.dataHash;
				std::string recalculateHash = hasher->getHash((char*)&entry.info, sizeof(entry.info));

				if (recalculateHash == storedDataHash)
				{
					_resultInfo = entry.info;

					// Erase data by security issues.
					decodedPassword.clear();
					ZeroMemory(&entry, sizeof(entry));

					return (true);
				}
				else
				{
					logMessage(funcname + ": error - recalculated hashes not matched.");
				}

				return false;
			}

			//
			// Note: It's better to generate new password for encryption! 
			//
			bool CardsKeeper::encodeEntry(const CardInfo& _cardInfo, CardEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt)
			{
				std::string funcname = std::string(__FUNCTION__);

				// Use current master-password.
				auto mp = m_masterPassword;

				if (!mp.isSet())
				{
					logMessage(funcname + ": error - can't work because master-password isn't set.");
					return false;
				}

				return encodeEntry(_cardInfo, _resultEntry, _hash, _crypt, mp);
			}

			bool CardsKeeper::encodeEntry(const CardInfo& _cardInfo, CardEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt, ::logic::common::MasterPassword _password)
			{
				std::string funcname = std::string(__FUNCTION__);
				CardEntry entry;

				entry.protection.usedCryptAlgorithm = _crypt;
				entry.protection.usedHashAlgorithm = _hash;

				//
				// At first, we need to know - Do we support all used crypto algorithms?
				//
				bool supportedCryptography = m_crypt.isSupported(_hash) && m_crypt.isSupported(_crypt);

				if (!supportedCryptography)
				{
					logMessage(funcname + ": error - advised cryptography algorithms are not supported.");

					if (!m_crypt.isSupported(entry.protection.usedCryptAlgorithm))
					{
						logMessage(funcname + ": error - unsupported encoding algorithm - " + std::to_string(entry.protection.usedCryptAlgorithm));
					}
					else if (!m_crypt.isSupported(entry.protection.usedHashAlgorithm))
					{
						logMessage(funcname + ": error - unsupported hashing algorithm algorithm - " + std::to_string(entry.protection.usedHashAlgorithm));
					}

					return false;
				}

				::logic::common::Hasher* hasher = m_crypt.get(entry.protection.usedHashAlgorithm);
				::logic::common::SymCryptor* symCryptor = m_crypt.get(entry.protection.usedCryptAlgorithm);

				if (!(hasher && symCryptor))
				{
					logMessage(funcname + ": error - hasher && symCryptor failed.");
					return false;
				}

				std::string hashOfData = hasher->getHash((const char*)&_cardInfo, sizeof(_cardInfo));
				if (hashOfData.empty())
				{
					logMessage(funcname + ": error - can't calculate hash of data.");
					return false;
				}

				// Save hash of original bank card information.
				fill_chars(entry.protection.dataHash, hashOfData.c_str());

				//
				// Encode card's data using current actual user password.
				//
				std::string passwordBuffer;
				if (!_password.getPasswordBuffer(passwordBuffer))
				{
					logMessage(funcname + ": error - can't get password's buffer.");
					return false;
				}

				entry.info = _cardInfo;
				bool encoded = symCryptor->encrypt((char*)&entry.info, sizeof(entry.info), passwordBuffer.c_str(), passwordBuffer.size());
				if (!encoded)
				{
					logMessage(funcname + ": error - can't encode data-part.");
					ZeroMemory(&entry, sizeof entry);
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

				//
				// Save hash of used for encryption password.
				//
				fill_chars(entry.protection.passwordHash, hashOfUsedPassword.c_str());

				//
				// Save encoded password which used for encryption card's information.
				//
				::logic::common::setProtPassword(entry.protection, passwordBuffer);

				encoded = symCryptor->encrypt((char*)entry.protection.passwordEncoded, entry.protection.passwordSize /*sizeof(entry.protection.passwordEncoded)*/, passwordBuffer.c_str(), passwordBuffer.size());

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
					logMessage(funcname + ": error - can't encode password field.");
					return false;
				}
			}

			void CardsKeeper::logMessage(const std::string& _msg)
			{
				m_log.print(_msg);
			}

			unsigned long CardsKeeper::length() const
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				return m_storage.count();
			}

			bool CardsKeeper::clear()
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				bool erased = m_storage.clear();
				
				if (erased)
				{
					m_entries.clear();

					m_storage.flush();
				}

				return erased;
			}

			bool CardsKeeper::reEncrypt(::logic::common::MasterPassword _newPassword, ::logic::common::MasterPassword _currentPassword)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				bool canFlush = true;

				if (isOk())
				{
					CardEntryMap modifiedEntries;

					// Make temporary copy.
					auto copyEntries = m_entries;

					for (auto entry : copyEntries)
					{
						CardInfo ci;
						if (decodeEntry(entry.second, ci, _currentPassword))
						{
							CardEntry newEncoded;
							if (encodeEntry(ci, newEncoded, entry.second.protection.usedHashAlgorithm, entry.second.protection.usedCryptAlgorithm, _newPassword))
							{
								modifiedEntries[entry.first] = newEncoded;
							}
							else
							{
								// canFlush = false;
								logMessage(std::string(__FUNCTION__) + ": error - can't encode data with new password.");
							}

							ZeroMemory(&ci, sizeof ci);
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
						m_entries = modifiedEntries;
						
						if(!flushCards())
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

			bool CardsKeeper::setPassword(::logic::common::MasterPassword _password)
			{
				std::unique_lock<std::mutex> lock(m_lockCards);

				if (_password.isSet())
				{
					m_masterPassword = _password;
					return true;
				}
				else
				{
					logMessage(std::string(__FUNCTION__) + ": error - can't set the password because its empty.");
					return false;
				}
			}
		}
	}
}
