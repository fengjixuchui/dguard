//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <string>
#include <boost/noncopyable.hpp>


#define SYSTEM_FLOCK		"flock"
#define SYSTEM_WALLET		"wallet"
#define SYSTEM_SHREDDER		"shredder"
#define SYSTEM_ENCRYPTION	"encryption"


namespace logic
{
	namespace common
	{
		enum HashAlgorithm
		{
			Unknown,
			Hash_Crc32,
			Hash_Md5,
			Hash_Sha1,
			Hash_Sha256,
			Hash_Sha512,
			Hash_Whirepool
		};

		enum CryptAlgorithm
		{
			CA_Unknown = 0,
			CA_Grader = 1, // Symmetric with 64 bit key.
			CA_Aes = 2, // With 128 bit blocks and 128 bits key.
			CA_Aes256 = 3 // With 256 bit key.
		};

		enum InternalStatus
		{
			Int_Success = 0, // means SUCCESS!
			Int_UnknownError = 1,
			Int_NotFound = 2,
			Int_CriticalError = 3,
			Int_AccessDenied = 4,
			Int_InvalidFormat = 5,
			Int_InProcess = 6,
			Int_Completed = 7,
			Int_LimitAchieved = 8,
			Int_PresentAlready = 9,
			Int_DriverNotConnected = 10,
			Int_UnknownType = 11,
			Int_HaveNoResponse = 12,
			Int_NoMasterPassword = 13,
			Int_LicenseExpired = 14,
			Int_LicenseExpireSoon = 15,
			Int_NotEncoded = 16,
			Int_EncodedAlready = 17,
			Int_WrongEncryptionKey = 18,
			Int_DecodedButIntegrityCompromised = 19,
			Int_TypeConvertionError = 20,
			Int_NotLoaded = 21,
			Int_FLockWasNotSigned = 22
		};

		inline bool IntSuccess(InternalStatus _state)
		{
			return _state == InternalStatus::Int_Success;
		}

		//
		// Structure for Future Compatibility. 
		//
		struct Sfci
		{
			unsigned long ci_version;
			unsigned long ci_size;
		};

		enum EraseObjectType
		{
			EOT_Unknown,
			EOT_File,
			EOT_Directory,
			EOT_Disk
		};
		
		//
		// Description of the object which should be removed without recovery ability.
		//
		struct EraseObject
		{
			EraseObjectType objectType;
			std::wstring path;
		};

		//
		// Contains information about finished erase operation.
		//
		struct EraseObjectResult
		{
			EraseObject object;
			InternalStatus result;
		};

		struct EntryProtect
		{
			// Information about current version of structure.
			::logic::common::Sfci sfci;

			//
			// This is a context information is used to future compatibility and modifications.
			//
			union ContextApplication{
				char contextArgs[128];
			};

			// Type of used cryptography algorithm.
			::logic::common::CryptAlgorithm usedCryptAlgorithm;

			// Used 
			common::HashAlgorithm usedHashAlgorithm;

			//
			// Information about password which was used for encryption bank card.
			//
			unsigned long passwordSize;
			char passwordEncoded[256 + 1];
			char passwordHash[64 + 1]; // Hash by original (decoded) password.
			//common::HashAlgorithm passwordHashType; // Algorithm - md5 by default.

			//
			// Hash from original (decoded) protected data.
			//
			char dataHash[64 + 1];
			//common::HashAlgorithm dataHashType; // Algorithm - md5 by default.
		};

		//
		// Reads encoded password from 'EntryProtect' object.
		//
		std::string getEncodedPassword(const EntryProtect& _entry);

		//
		// Writes '_password' in protection structure.
		//
		void setProtPassword(EntryProtect& _protection, std::string _password);


		class dgexception
		{
		public:
			dgexception(){};
			dgexception(std::wstring _message) :m_msg(_message){}
			std::wstring getMsg() const { return m_msg; }
		private:
			std::wstring m_msg;
		};
	}
}
