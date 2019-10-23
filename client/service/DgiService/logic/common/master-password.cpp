//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "master-password.h"
#include "../../helpers/encryption/cryptoHelpers.h"
#include "../../helpers/internal/helpers.h"

namespace whlp{
	class Mapper;
}

namespace logic
{
	namespace common
	{
		MasterPassword::MasterPassword()
		{
			//
			// Generate random encryption key.
			//
			generateKey();
		}

		MasterPassword::MasterPassword(std::wstring _password)
		{
			if (_password.empty())
			{
				// throw an exception or mark that object as uninitialized.
				throw dgexception(std::wstring(__FUNCTIONW__) + L": error - passed empty password");
			}

			//
			// Generate random encryption key.
			//
			generateKey();

			if(reset(_password))
			{
				// ok password was set.
			}
			else
			{
				// An error, password was not changed.
				throw dgexception(std::wstring(__FUNCTIONW__) + L": error - can't set password");
			}
		}

		MasterPassword::MasterPassword(const MasterPassword& _other)
		{
			if (this != &_other)
			{
				copyInternals(_other);
			}
		}

		MasterPassword::~MasterPassword()
		{
			//
			// Clear encoded password.
			//
			secureErase(m_encodedPassword);
			secureErase(m_hashPassword);
			secureErase(m_key);
		}

		void MasterPassword::secureErase(std::string& _toEraseData)
		{
			for (size_t i = 0; i < _toEraseData.size(); ++i)
			{
				_toEraseData.at(i) = '\0';
			}
			_toEraseData.erase();
		}

		void MasterPassword::secureErase(std::wstring& _toEraseData)
		{
			for (size_t i = 0; i < _toEraseData.size(); ++i)
			{
				_toEraseData.at(i) = L'\0';
			}
			_toEraseData.erase();
		}

		void MasterPassword::generateKey()
		{
			std::string newkey;

			for (unsigned int i = 0; i < m_encoder.keySize(); ++i)
			{
				auto newbyte = (char) ((rand() + 1) % 255);

				newkey.push_back(newbyte);
			}

			newkey.swap(m_key);
		}

		bool MasterPassword::encodePassword(std::wstring _password, std::wstring& _encodedPassword)
		{
			bool result = false;

			bool encoded = m_encoder.encrypt((char*)_password.data(), _password.size() * sizeof(wchar_t), m_key.data(), m_key.size());

			if (encoded)
			{
				_encodedPassword = _password;
				result = true;
			}

			secureErase(_password);
			return result;
		}

		bool MasterPassword::getDecodedPassword(std::wstring& _decodedPassword) const
		{
			bool result = false;
			std::wstring currentPassword = m_encodedPassword;

			bool isOk = m_encoder.decrypt((char*)currentPassword.data(), currentPassword.size() * sizeof(wchar_t), m_key.data(), m_key.size());

			if (isOk)
			{
				_decodedPassword = currentPassword;
				result = true;
			}
			else
			{
				secureErase(currentPassword);
			}

			return result;
		}

		bool MasterPassword::getPassword(std::wstring& _password) const
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (!hasPassword())
			{
				return false;
			}

			std::wstring passw;
			auto isOk = getDecodedPassword(passw);

			if (isOk)
			{
				_password = passw;
			}

			secureErase(passw);
			return isOk;
		}


		bool MasterPassword::getPasswordBuffer(std::string& _buffer) const
		{
			std::wstring password;
			bool isOk = getPassword(password);

			if (isOk)
			{
				if ( isOk = !password.empty())
				{
					std::string buffer((const char*)password.data(), password.length() * sizeof(wchar_t));

					_buffer.swap(buffer);
				}
			}

			return isOk;
		}

		bool MasterPassword::getTempKey(std::string& _buffer) const
		{
			std::string buffer;
			bool isOk = getPasswordBuffer(buffer);

			if (isOk)
			{
				for (std::size_t i = 0; i < buffer.length(); ++i)
				{
					unsigned char ch = buffer.at(i);

					if (ch == 0){
						ch = GetTickCount() % 255;
						if (ch == 0){
							ch = 0xFE;
						}

						buffer[i] = ch;
					}
				}
			}

			return isOk;
		}

		bool MasterPassword::change(std::wstring _old, std::wstring _new)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (!hasPassword())
			{
				return false;
			}

			if (_old.empty() || _new.empty())
			{
				return false;
			}

			bool res = false;

			std::string hashOldPassw = crypto::getSha256((const unsigned char*)_old.data(), _old.size() * sizeof(wchar_t));

			//
			// If old password is valid then we can change it.
			//
			if (hashOldPassw == m_hashPassword)
			{
				std::string hashNewPassword = crypto::getSha256((const unsigned char*)_new.data(), _new.size() * sizeof(wchar_t));

				std::wstring newEncoded;
				res = encodePassword(_new, newEncoded);

				// Change password info if it's success.
				//
				if (res)
				{
					m_encodedPassword = newEncoded;
					m_hashPassword = hashNewPassword;
				}
			}

			secureErase(_old);
			secureErase(_new);

			return res;
		}

		void MasterPassword::erasePassword()
		{
			std::unique_lock<std::mutex> lock(m_lock);

			secureErase(m_hashPassword);
			secureErase(m_encodedPassword);
		}

		bool MasterPassword::reset(std::wstring _password)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			std::wstring encodedPassword;
			std::string hash = crypto::getSha256(reinterpret_cast<const unsigned char*>(_password.data()), _password.size() * sizeof(wchar_t));
			bool isOk = encodePassword(_password, encodedPassword);

			if (isOk)
			{
				m_hashPassword = hash;
				m_encodedPassword = encodedPassword;
			}

			//
			// Erase temporary password variable.
			//
			secureErase(_password);
			return isOk;
		}

		bool MasterPassword::hasPassword() const
		{
			bool notSet = (m_encodedPassword.empty() || m_hashPassword.empty() || m_key.empty());

			return !notSet;
		}

		bool MasterPassword::isSet() const
		{
			std::unique_lock<std::mutex> lock(m_lock);

			return hasPassword();
		}

		bool MasterPassword::getHash(std::string& _hash, HashAlgorithm& _hashAlgType) const
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (!m_hashPassword.empty())
			{
				_hash = m_hashPassword;
				_hashAlgType = logic::common::HashAlgorithm::Hash_Sha256; // hard coded value.
				return true;
			}

			return false;
		}

		bool MasterPassword::operator==(const MasterPassword& _rhs)
		{
			if (this == &_rhs)
			{
				return true;
			}

			return (this->m_hashPassword == _rhs.m_hashPassword);
		}

		MasterPassword& MasterPassword::operator=(const MasterPassword& _rhs)
		{
			if (this != &_rhs)
			{
				copyInternals(_rhs);
			}

			return *this;
		}

		MasterPassword::MasterPassword(MasterPassword&& _rhs)
		{
			if (this != &_rhs)
			{
				copyInternals(_rhs);

				// remove '_rhs' data.
			}
		}

		void MasterPassword::copyInternals(const MasterPassword& _other)
		{
			if (this != &_other)
			{
				this->m_encodedPassword = _other.m_encodedPassword;
				this->m_hashPassword = _other.m_hashPassword;
				this->m_key = _other.m_key;
				this->m_encoder = _other.m_encoder;
			}
		}

		//
		//
		//
		// Master password keeper.
		//
		//
		//

		MPassKeeper::MPassKeeper(std::wstring _passinfofile, logfile& _log) :
			m_log(_log),
			m_hashStorage(_passinfofile)
		{
			auto loaded = m_hashStorage.load(sizeof(MprInfo));

			if (!loaded)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - can't load " + strings::ws_s(_passinfofile) );
			}
		}

		MPassKeeper::~MPassKeeper()
		{
			m_hashStorage.flush();
			m_hashStorage.unload();
		}

		bool MPassKeeper::isStorageLoaded() const
		{
			std::unique_lock<std::mutex> lock(m_lock);

			return m_hashStorage.isLoaded();
		}

		bool MPassKeeper::hasPassword()
		{
			std::unique_lock<std::mutex> lock(m_lock);

			return m_password.isSet();
		}

		bool MPassKeeper::hasHash()
		{
			std::unique_lock<std::mutex> lock(m_lock);

			MprInfo mpr;
			return readStorageData(mpr);
		}

		logic::common::MasterPassword MPassKeeper::getPassword() const
		{
			std::unique_lock<std::mutex> lock(m_lock);

			return m_password;
		}

		std::string MPassKeeper::getHash()
		{
			std::unique_lock<std::mutex> lock(m_lock);

			MprInfo mpr;
			if (readStorageData(mpr))
			{
				SET_LAST_ZERO_CHAR(mpr.hashbuf);

				std::string hash = mpr.hashbuf;
				return hash;
			}

			return "";
		}

		bool MPassKeeper::setPassword(MasterPassword _password)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (_password.isSet())
			{
				std::string hash;
				HashAlgorithm halg;

				if (_password.getHash(hash, halg))
				{
					MprInfo mpr;

					mpr.signature = MasterPasswordSignature;
					mpr.hashalg = halg;
					fill_chars(mpr.hashbuf, hash.c_str());

					if (m_hashStorage.getSizeOfMap() < sizeof(MprInfo))
					{
						m_hashStorage.cutSize(sizeof(MprInfo));
					}

					if (m_hashStorage.getSizeOfMap() == sizeof(MprInfo))
					{
						MprInfo* phashInfo = (MprInfo*)m_hashStorage.getMappedData();

						*phashInfo = mpr;

						if (m_hashStorage.flush())
						{
							//
							// Change the password in memory only after success flushing data to disk.
							//

							m_password = _password;

                            //
                            //  Success.
                            //

							return true;
						}
					}
				}
			}

			return false;
		}

		bool MPassKeeper::isThePassword(MasterPassword _password)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			std::string hash, storedHash;
			HashAlgorithm halg;

			if (_password.getHash(hash, halg))
			{
				MprInfo mpr;
				if (readStorageData(mpr))
				{
					SET_LAST_ZERO_CHAR(mpr.hashbuf);
					storedHash = mpr.hashbuf;

					//
					// Hashes are equal when and its algorithms match.
					//
					if ( (halg == mpr.hashalg) && (storedHash == hash) )
					{
						return true;
					}
				}
			}

			return false;
		}

		bool MPassKeeper::readStorageData(MprInfo& _readTo)
		{
			if (!m_hashStorage.isLoaded())
			{
				m_hashStorage.load();
			}

			if (m_hashStorage.isLoaded() )
			{
				auto st = m_hashStorage.getSizeOfMap();

				if (st >= sizeof(MprInfo))
				{
					MprInfo mpr = *((MprInfo*)m_hashStorage.getMappedData());

					if (mpr.signature == ::logic::common::MasterPasswordSignature)
					{
						_readTo = mpr;
						return true;
					}
				}
			}

			return false;
		}

	}
}
