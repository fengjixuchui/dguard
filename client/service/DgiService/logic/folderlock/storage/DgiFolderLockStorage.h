//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <map>
#include <mutex>
#include <vector>
#include <boost/noncopyable.hpp>
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCrypt.h"
#include "../../../logic/common/DgiCommon.h"
#include "../../../helpers/containers/file/tstorage.h"
#include "../../common/master-password.h"

namespace logic
{
	namespace folderlock
	{
		namespace storage
		{
			//using namespace ::logic::common;

			const unsigned long FlockIdLength = 16;

			//namespace internal
			//{

			// Depends on thrift-defined types!
			enum FLockObjectType
			{
				FLock_Unknown = 0,
				FLock_File = 1,
				FLock_Directory = 2,
				FLock_HardDisk = 3
			};

			// All values equal to thrift-defined values.
			// be careful with changing of that value!
			enum FLockState
			{
				FLock_UnknownState = 0,
				FLock_Missed = 1, // when file is missed
				FLock_Locked = 2, // file or folder is locked 
				FLock_Unlocked = 3, // folder accessed normally
				FLock_Hidden = 4, // File or folder is hidden
				FLock_HiddenAndLocked = 5 // File system object is hidden on a local disk and access to it is disabled.
			};

			typedef struct _FLOCK_META
			{
				UCHAR signature[16];
				DWORD version; /* zero by default */
				UCHAR uniqueId[FlockIdLength];
				DWORD flags;
			} FLOCK_META, *PFLOCK_META;

			struct FLockObject
			{
				::logic::common::Sfci sfci;

				FLockObjectType type;
				UCHAR uniqueId[FlockIdLength];
				FLockState state;
				unsigned short cbPathLength;
				wchar_t path[1024 + 1];
			};

			struct FLockEntry
			{
				::logic::common::EntryProtect protection;
				FLockObject flockObject;
			};

			class Selector{
				public:
					bool operator()(const FLockEntry& _entry) const{
						return false;
					}
				};
			
			typedef TStorage<FLockEntry, Selector> FLockStorage;
			typedef std::vector<FLockEntry> FLockEntryList;
			typedef std::vector<std::wstring /* file path */, FLockObject> FLockList;
			typedef std::map< std::wstring /* file path */, FLockEntry> FLockEntryMap;

			//
			// Keeps information about locked files and folders. 
			//
			class Keeper
			{
			public:
				Keeper(::logic::common::DgiCrypt& _dgiCrypt, logfile& _log, std::string _storageFile = "flock.storage");
				~Keeper();


				bool isLoaded();
				bool present(std::wstring _path);
				bool presentById(std::string _flockId);
				bool add(FLockObject _flockobject);
				bool setState(std::wstring _path, ::logic::folderlock::storage::FLockState _newState);
                bool setStateById(std::string _flockId, ::logic::folderlock::storage::FLockState _newState);
				bool unlock(std::wstring _path);
				bool isLocked(std::wstring _path);
				bool getInfo(std::wstring _path, FLockObject& _outInfo);
                bool getInfoById(std::string _flockId, FLockObject& _outInfo);

				std::vector<FLockObject> getAll();

				unsigned long length();
				bool remove(std::wstring _path);
				bool removeById(std::string _flockId);
				bool clear();

				// Cryptographic options.
				//
				::logic::common::HashAlgorithm getAlgHash() const;
				::logic::common::CryptAlgorithm getAlgCrypt() const;
				bool setAlgHash(::logic::common::HashAlgorithm _alg);
				bool setAlgCrypt(::logic::common::CryptAlgorithm _alg);

				// Methods to keep wallet in secure state.
				//
				bool setPassword(common::MasterPassword _password);

				// For external use.
				//
				bool load();

				//
				// Decodes data with current password and encodes with new password.
				// It occurs in process of changing master-password.
				//
				bool reEncrypt(::logic::common::MasterPassword _newPassword, ::logic::common::MasterPassword _currentPassword);

				// Verifies - how much entries it can decode with specified '_password'.
				// Returns list of FLocks which it could decode and 'false' as a result if one or more entries were not decoded.
				//
				bool canDecodeEntires(::logic::common::MasterPassword _password, std::vector<FLockObject>& _outFLocks);

			protected:

				//
				// Verifies connection with storage file - Is it loaded or not? 
				//
				bool isOk();

				bool isPresent(std::wstring _path);

                bool findById(std::string _flockId, FLockObject& _outFlock);

				bool take(std::wstring _path, FLockObject& _outFlock);
				bool take(std::wstring _path, FLockEntry& _outEntry);
                bool takeEntryById(std::string _id, FLockEntry& _outEntry);
				bool change(std::wstring _path, const FLockObject& _changedFlock);

				// Returns path in lower case.
				std::wstring perform(std::wstring _path);

				bool decodeEntry(const FLockEntry& _entry, FLockObject& _resultInfo);
				bool decodeEntry(const FLockEntry& _entry, FLockObject& _resultInfo, ::logic::common::MasterPassword _password);

				bool encodeEntry(const FLockObject& _flockInfo, FLockEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt);
				bool encodeEntry(const FLockObject& _flockInfo, FLockEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt, ::logic::common::MasterPassword _password);

				bool loadEntries();
				bool flushEntries();

				void logMessage(const std::string& _msg);

			private:
				logfile& m_log;
				
				mutable std::mutex m_lock;
				FLockEntryMap m_entries;

				common::MasterPassword m_masterPassword;

				// Component which does all necessary encryption.
				::logic::common::DgiCrypt& m_cryptor;

				// What we use by-default if no other choices.
				::logic::common::HashAlgorithm m_defaultHash;
				::logic::common::CryptAlgorithm m_defaultCrypt;

				FLockStorage m_storage;
			};
		}
	}
}
