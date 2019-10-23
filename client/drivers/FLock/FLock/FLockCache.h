//
// Project:
//
//		Data Guard FLock driver.
//
// Author:
//
//		Burlutsky Stanislav
//		burluckij@gmail.com
//

#pragma once

#include "flock.h"

typedef struct _FLOCK_CACHE_ENTRY
{
	BOOLEAN presentMeta;
	BOOLEAN occupied;
	UCHAR hash[16];

}FLOCK_CACHE_ENTRY, *PFLOCK_CACHE_ENTRY;

typedef struct _FLOCK_CACHE_DATA
{
	ULONG capacity;
	ULONG length;
	//ULONG currentOccupancy;
	ULONG occupancyLimit;
	ULONG collisionOccurrences;
	ULONG collisionMaxResolveOffset;

	// Imagine we need insert element in to last position which is occupied,
	// In that case we need start search from first position to that - 'collisionResolveIfNoPlace'.
	ULONG collisionResolveIfNoPlace;

	PFLOCK_CACHE_ENTRY cached;
	ERESOURCE	lock;
	BOOLEAN enabled;
	BOOLEAN needStop;

} FLOCK_CACHE_DATA, *PFLOCK_CACHE_DATA;

BOOLEAN FLockCacheInit();
void FLockCacheDeinitialyze();

// Returns TRUE if cache works good and enabled.
//
BOOLEAN FLockCacheIsEnable();
VOID FLockCacheEnable();
VOID FLockCacheDisable();

void FLockCacheLock();
void FLockCacheUnlock();

ULONG FLockCacheCapacity();
ULONG FLockCacheLength();

BOOLEAN FLockCacheLookup(__in PUCHAR _hash, __out PFLOCK_CACHE_ENTRY _result, __out ULONG* _stepsRequiredToFind);
VOID FLockCacheAdd(__in PFLOCK_CACHE_ENTRY _newEntry);
VOID FLockCacheUpdateOrAdd(__in PFLOCK_CACHE_ENTRY _newEntry);

// Removes all data in hash table and changes current length to 0, but does not change .capacity.
//
VOID FLockCacheErase();
