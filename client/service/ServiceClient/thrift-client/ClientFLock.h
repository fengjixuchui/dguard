//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "..\ServiceClient.h"
#include "..\..\..\thrift\cpp\DgiFolderLock.h"

namespace thrift_client
{
	

	class ClientFLock
	{
	public:

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		// Some important types declared here.
		//
		// Begin.
		//

		// Copied from this file - DgiFolderLockStorage.h.
		// All values equal to thrift-defined values. Be careful with changing of that value!
		enum FLockObjectType
		{
			FLock_Unknown = 0,
			FLock_File = 1,
			FLock_Directory = 2,
			FLock_HardDisk = 3
		};

		// Copied from this file - DgiFolderLockStorage.h.
		//
		// All values equal to thrift-defined values. Be careful with changing of that value!
		enum FLockState
		{
			FLock_UnknownState = 0,
			FLock_Missed = 1, // when file is missed
			FLock_Locked = 2, // file or folder is locked 
			FLock_Unlocked = 3, // folder accessed normally
			FLock_Hidden = 4, // File or folder is hidden
			FLock_HiddenAndLocked = 5 // File system object is hidden on a local disk and access to it is disabled.
		};

		struct FLockObject
		{
			FLockObjectType type;
			FLockState state;
			std::string uniqueId;
			std::wstring filePath;
		};

		typedef std::vector<FLockObject> FLocks;

		// The end.
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		ClientFLock(std::string _host, int _port) : m_host(_host), m_port(_port) {

		}

		bool add(const std::string& _sid, const FLockObject& _newFlock);
		bool getFlocks(const std::string& _sid, FLocks& _outFlocks);

		bool getState(const std::string& _sid, const std::wstring& _filepath, FLockState& _outState);
		bool setState(const std::string& _sid, const std::wstring& _filepath, const FLockState& _newState);

		bool present(const std::string& _sid, std::wstring _filepath);
		bool presentId(const std::string& _sid, std::string _id);
		bool remove(const std::string& _sid, std::wstring _filepath);
		bool removeAll(const std::string& _sid);

        bool isSupportedFs(std::wstring _path, bool& _supported);

// 		dgiCommonTypes.SubSystemStateResponse getSubsState();
// 		dgiFolderLockTypes.FLockCacheInfo getCacheInfo()

	private:
		std::string m_host;
		int m_port;
	};
}
