//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"

enum FLockObjectType
{
	FLock_Unknown = 0,
	FLock_File = 1,
	FLock_Directory = 2,
	FLock_HardDisk = 3
}

enum FLockState
{
	FLock_UnknownState = 0,
	FLock_Missed = 1, // when file is missed
	FLock_Locked = 2, // file or folder is locked 
	FLock_Unlocked = 3, // folder accessed normally
	FLock_Hidden = 4, // File or folder is hidden
	FLock_HiddenAndLocked = 5 // File system object is hidden on a local disk and access to it is disabled.
}

struct FLockObject
{
	1: FLockObjectType type,
	2: string flockId, // Unique 16 bytes identificator.
	3: string path
}

typedef list<FLockObject> FLockList

struct FLockObjectError
{
	1: FLockObject erObject, // file system object
	2: dgiCommonTypes.DgiStatus status, // error code is here
	3: string description // why we could not erase the object, optional to fill
}

typedef list<FLockObjectError> FLockErrorList

struct FLockInfo
{
	1: FLockObject obj,
	2: FLockState state
}

typedef list<FLockInfo> FLockInfoList

struct FLockStateResponse
{
	1: dgiCommonTypes.DgiResult result,
	2: FLockInfo flinf
}

struct FLockListResponse
{
	1: dgiCommonTypes.DgiResult result,
	2: FLockInfoList flocks
}

struct FLockCacheInfo
{
	1: i32 totalEntries,
	2: i32 freeEntries,
	3: i32 maxCollisionLength
	
	// Will be later...
}
