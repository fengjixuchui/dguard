//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"
include "dgiFolderLockTypes.thrift"

//
// Here is an interface of FLock service.
//

service DgiFolderLock
{
	//
	// Returns File-system-Lock subsystem internal state.
	// The system could be in different states: not started, failed, started and so on.
	//
	dgiCommonTypes.SubSystemStateResponse getSubsState()
	
	//
	//	The routine returns 'true' in case of target file system is supported.
	//	We support all file systems which support Extended-Attributes (EAs) - NTFS, ReFS, Ext3, Ext4.
	//	Not supported - FAT, FAT32.
	//
	//	_path is any path on target disk - "x:\dir", "\??\x:\windows\d", "x:\", "x:".
	//
	dgiCommonTypes.BoolResponse isSupportedFs(1: string _path)

	//
	// Adds new flock object.
	//
	dgiCommonTypes.DgiStatus add(1: dgiCommonTypes.DgiSid _sid, 2: dgiFolderLockTypes.FLockInfo _flock)
	
	//
	// Returns list of all locked objects with state information.
	//
	dgiFolderLockTypes.FLockListResponse getFlocks(1: dgiCommonTypes.DgiSid _sid)
	
	//
	// Returns actual information about the flock with status - missed, locked, unlocked, hidden.
	//
	dgiFolderLockTypes.FLockStateResponse getState(1: dgiCommonTypes.DgiSid _sid, 2: dgiCommonTypes.utf8string _flockPath)
	
	//
	// Changes state for early added flock object. Provides an ability to lock, unlock and hide target file system object from third-party persons.
	//
	dgiCommonTypes.DgiResult setState(1: dgiCommonTypes.DgiSid _sid, 2: dgiCommonTypes.utf8string _flockPath, 3: dgiFolderLockTypes.FLockState _newState)
	
	dgiCommonTypes.DgiResult setStateById(1: dgiCommonTypes.DgiSid _sid, 2: string _flockId, 3: dgiFolderLockTypes.FLockState _newState)
	
	//
	// Returns true if the 'flock' object was added earlier.
	//
	dgiCommonTypes.BoolResponse present(1: dgiCommonTypes.DgiSid _sid, 2: dgiCommonTypes.utf8string _flockPath)
	
	//
	// Returns true if the 'flock' object was added earlier.
	//
	dgiCommonTypes.BoolResponse presentById(1: dgiCommonTypes.DgiSid _sid, 2: string _flockId)
	
	//
	// Removes earlier added flock object and unlocks it.
	//
	dgiCommonTypes.DgiResult remove(1: dgiCommonTypes.DgiSid _sid, 2: dgiCommonTypes.utf8string _flockPath)
	
	//
	// Clears list with protected files, folders.
	//
	dgiCommonTypes.DgiResult removeAll(1: dgiCommonTypes.DgiSid _sid)
	
	//
	// Method provides information about internal state of kernel-mode flock chache.
	//
	dgiFolderLockTypes.FLockCacheInfo getCacheInfo()
}

