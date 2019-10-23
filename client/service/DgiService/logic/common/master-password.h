//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <set>
#include <map>
#include <mutex>
#include <memory>
#include <boost/noncopyable.hpp>

#include "../../helpers/internal/log.h"
#include "../../helpers/keeper/filesKeeper.h"
#include "DgiCommon.h"
#include "DgiCrypt.h"
#include "../../helpers/containers/file/mapper.h"

namespace logic
{
	namespace common
	{
		const DWORD MasterPasswordSignature = 0xAF0E1188;

		//
		//	Keeps master-password in memory.
		//

		class MasterPassword /*: private boost::noncopyable*/
		{
		public:
			MasterPassword();
			MasterPassword(std::wstring _password);
			MasterPassword(const MasterPassword& _other);
			~MasterPassword();
			MasterPassword& operator=(const MasterPassword& rhs);
			MasterPassword(MasterPassword&& _passw);

			//
			//	Returns true if hashes are equal.
			//

			bool operator==(const MasterPassword& _rhs);

			//
			//	Returns true when master password is set.
			//

			bool isSet() const;

			//
			//	Sets new password without any verifications.
			//

			bool reset(std::wstring _password);

			//
			//	Before to set new password it verifies info about old password.
			//

			bool change(std::wstring _old, std::wstring _new);

			//
			//	Returns hash of password.
			//

			bool getHash(std::string& _hash, HashAlgorithm& _hashAlgType) const;

			//
			//	Returns original decoded-password.
			//

			bool getPassword(std::wstring& _password) const;

			//
			//	Returns password buffer.
			//

			bool getPasswordBuffer(std::string& _buffer) const;

			//
			//	Returns performed password which can be used as encryption key.
			//

			bool getTempKey(std::string& _buffer) const;

			//
			//	Deletes password for security issues.
			//

			void erasePassword();

			//	Erases password key.
			static void secureErase(std::string& _toEraseData);
			static void secureErase(std::wstring& _toEraseData);

		protected:
			void generateKey();

			bool getDecodedPassword(std::wstring& _decodedPassword) const;
			bool encodePassword(std::wstring _password, std::wstring& _encodedPassword);

			// Returns true if we have password, hash of password and encryption key.
			bool hasPassword() const;

			void copyInternals(const MasterPassword& _other);

		private:
			mutable std::mutex m_lock;

			std::wstring m_encodedPassword;
			std::string m_hashPassword; // sha256 hash of original password.

			std::string m_key;
			mutable EncodingGrader m_encoder;

			//
			//	Lock copy operations.
			//

			MasterPassword& operator=(MasterPassword&&);
		};


#pragma pack(push, 1)
		struct MprInfo
		{
			DWORD signature; // should be equal to 'MasterPasswordSignature'.
			bool inited;
			::logic::common::HashAlgorithm hashalg; // Algorithm used for encryption.
			char hashbuf[512 + 1]; // Null terminated string.
		};
#pragma pack(pop)
		
		
		class MPassKeeper
		{
			//
			//	That object acquires file with hash information about master password and locks it from an external access.
			//

		public:

			MPassKeeper(std::wstring _passinfofile, logfile& _log);
			~MPassKeeper();

			bool isStorageLoaded() const;

			//
			//	Returns true if we have master-password in virtual memory.
			//

			bool hasPassword();

			//
			//	Returns true when info we have hash of master password which was loaded from file-storage.
			//

			bool hasHash();

			//
			//	Returns copy of master-password in protected container.
			//

			MasterPassword getPassword() const;

			//
			//	Gets password hash which was read from file-storage on disk.
			//

			std::string getHash();

			//
			//	Sets password. We can't do that in constructor because at initialization moment 
			//	the dgi-windows service knows only hash of master-password. The master-password 
			//	loads later when user logins in Data Guard manually.
			//
			//	(!) Also this method returns false if user tries to set empty password.
			//

			bool setPassword(MasterPassword _password);

			//
			//	Returns true the stored hash is equal to the password.
			//

			bool isThePassword(MasterPassword _password);

		protected:

			bool readStorageData(MprInfo& _readTo);

		private:
			mutable std::mutex m_lock;
			logfile& m_log;
			whlp::MapperW m_hashStorage;
			MasterPassword m_password;
		};
	}
}
