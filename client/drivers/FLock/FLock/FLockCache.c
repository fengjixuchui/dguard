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

#include "FLockCache.h"
#include "FLock.h"

#define FLOCK_CACHE_TABLE_INDEX_LIMIT		(100 * 1000)
#define FLOCK_CACHE_TABLE_SIZE				(FLOCK_CACHE_TABLE_INDEX_LIMIT - 1)
#define FLOCK_CACHE_OCCUPANCY_LIMIT			(33 * 1000)

extern ULONG gTraceFlags;
extern FLOCK_DEVICE_DATA g_flockData;

FLOCK_CACHE_DATA g_cache = { 0 };


BOOLEAN FLockCacheInit()
{
	RtlZeroMemory(&g_cache, sizeof(FLOCK_CACHE_DATA));

	ExInitializeResourceLite(&g_cache.lock);

	g_cache.capacity = FLOCK_CACHE_TABLE_SIZE;
	g_cache.length = 0;
	g_cache.occupancyLimit = /*(FLOCK_CACHE_TABLE_INDEX_LIMIT / 3);*/ FLOCK_CACHE_OCCUPANCY_LIMIT;
	g_cache.collisionResolveIfNoPlace = 700;
	g_cache.enabled = TRUE;

	g_cache.cached = (PFLOCK_CACHE_ENTRY)ExAllocatePool(NonPagedPool, (g_cache.capacity + 1) * sizeof(FLOCK_CACHE_ENTRY) );

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: Cache configuration - %d buckets, %d limit, %d collision resolve if no place.\n",
			__FUNCTION__,
			(g_cache.capacity + 1),
			g_cache.occupancyLimit,
			g_cache.collisionResolveIfNoPlace)
		);

	if ( g_cache.cached == NULL )
	{
		g_cache.enabled = FALSE;

		PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s error - couldn't create hash table with %d buckets.\n", __FUNCTION__, (g_cache.capacity + 1) ));
	}

	return (g_cache.cached != NULL);
}

void FLockCacheDeinitialyze()
{
	ExDeleteResourceLite(&g_cache.lock);
}

BOOLEAN FLockCacheIsEnable()
{
	return g_cache.enabled;
}

VOID FLockCacheEnable()
{
	g_cache.enabled = TRUE;
}

VOID FLockCacheDisable()
{
	g_cache.enabled = FALSE;
}

void FLockCacheLock()
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&g_cache.lock, TRUE);
}

void FLockCacheUnlock()
{
	ExReleaseResourceLite(&g_cache.lock);
	KeLeaveCriticalRegion();
}

ULONG FLockCacheCapacity()
{
	return g_cache.capacity;
}

ULONG FLockCacheLength()
{
	return g_cache.length;
}

ULONG FLockCacheCalcIndex(
	__in PUCHAR _hash,
	__in ULONG _length, // length should be equal to 16 bytes
	__in ULONG _highBorder
	)
{
	ULONG dataForIndex = 0, index = 0;
	UCHAR dword_[4] = { 0 };

// 	dword_[3] = _hash[3];
// 	dword_[2] = _hash[2];
	dword_[1] = _hash[1];
	dword_[0] = _hash[0];

// 	dword_[3] = 0;
// 	dword_[2] = 0;

	dataForIndex = *((ULONG*)dword_);

// 	for (int i = 0; i < _length; ++i)
// 	{
// 		dword_[]
// 	}

	index = (dataForIndex % ( _highBorder /*FLOCK_CACHE_TABLE_SIZE*/));

	//PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: data for index %lu, index itself %d\n", __FUNCTION__, dataForIndex, index));

	return index;
}

BOOLEAN FLockCacheLookupIndexForNewRoom(
	__in ULONG _lookupIndexLimit,
	__in  PUCHAR _hash,
	__out PULONG _freeIndex,
	__out PBOOLEAN _collisionOccured,
	__out PULONG _stepsToPlaceNewEntry
	)
{
	ULONG index = FLockCacheCalcIndex(_hash, 16, _lookupIndexLimit);

	(*_collisionOccured) = FALSE;

	for (ULONG i = index; i <= _lookupIndexLimit; i++)
	{
		PFLOCK_CACHE_ENTRY cache_pos = g_cache.cached + i;

		if (cache_pos->occupied == TRUE)
		{
			if (memcmp(cache_pos->hash, _hash, 16) == 0)
			{
				// Yes, Strike!

				*_freeIndex = i;
				return TRUE;
			}
			else
			{
				(*_collisionOccured) = TRUE;

				(*_stepsToPlaceNewEntry)++;
			}
		}
		else
		{
			// It is a free room.

			*_freeIndex = i;

			return TRUE;
		}
	}

	// If we here, it means that we did not find right free room for new entry,
	// start search from beginning.

	for (ULONG i = 0; i <= g_cache.collisionResolveIfNoPlace; i++)
	{
		PFLOCK_CACHE_ENTRY cache_pos = g_cache.cached + i;

		if (cache_pos->occupied == TRUE)
		{
			if (memcmp(cache_pos->hash, _hash, 16) == 0)
			{
				// Yes, Strike!

				*_freeIndex = i;
				return TRUE;
			}
			else
			{
				(*_collisionOccured) = TRUE;

				(*_stepsToPlaceNewEntry)++;
			}
		}
		else
		{
			// It is a free room.

			*_freeIndex = i;

			return TRUE;
		}
	}

	return FALSE;
}


BOOLEAN FLockCacheLookup(__in PUCHAR _hash, __out PFLOCK_CACHE_ENTRY _result, __out ULONG* _stepsRequiredToFind)
{
	ULONG i = 0;
	ULONG index = FLockCacheCalcIndex(_hash, 16, g_cache.capacity);

	SETPTR(_stepsRequiredToFind, 0);

	// Index was successfully calculated, find available bucket for use.
	for (i = index; i <= g_cache.capacity; i++)
	{
		PFLOCK_CACHE_ENTRY cache_pos = g_cache.cached + i;

		if (cache_pos->occupied == TRUE)
		{
			if ( memcmp(cache_pos->hash, _hash, 16) == 0 )
			{
				// Yes, Strike!

				memcpy( _result, cache_pos, sizeof(FLOCK_CACHE_ENTRY) );
				return TRUE;
			}
			else
			{
				// It is collision, we need to further.
				(*_stepsRequiredToFind)++;
			}
		}
		else
		{
			// Stop search if found non occupied bucket.
			// Search further has no sense.
			return FALSE;
		}
	}

	// If we achieved bottom of the table and did not find what we actually need,
	// in that case need start search from beginning.
	for (i = 0; i < g_cache.collisionResolveIfNoPlace; ++i)
	{
		PFLOCK_CACHE_ENTRY cache_pos = g_cache.cached + i;

		// Does a bucket occupied?
		if (cache_pos->occupied == TRUE)
		{
			// Yes, verify its content.
			if (memcmp(cache_pos->hash, _hash, 16) == 0)
			{
				// Yes, Strike!
				memcpy(_result, cache_pos, sizeof(FLOCK_CACHE_ENTRY));
				return TRUE;
			}
			else
			{
				(*_stepsRequiredToFind)++;
			}
		}
		else
		{
			break;
		}
	}

	return FALSE;
}

VOID FLockCacheAdd(__in PFLOCK_CACHE_ENTRY _newEntry)
{
	BOOLEAN collisionOccured = FALSE;
	ULONG freeSpace = g_cache.capacity - g_cache.length, insertIndex = 0, stepsToPlaceEntry = 0;

	if (g_cache.length > g_cache.occupancyLimit)
	{
		PT_DBG_PRINT(PTDBG_TRACE_CACHE_COLLISION, ("FLock!%s: FLOCK_CACHE_OCCUPANCY_LIMIT achieved! Cache.length %d\n", __FUNCTION__, g_cache.length));

		RtlZeroMemory(g_cache.cached, sizeof(FLOCK_CACHE_ENTRY) * g_cache.capacity);
		g_cache.length = 0;
	}

	if (FLockCacheLookupIndexForNewRoom(g_cache.capacity, _newEntry->hash, &insertIndex, &collisionOccured, &stepsToPlaceEntry))
	{
		PFLOCK_CACHE_ENTRY tableEntry = g_cache.cached + insertIndex;

		RtlCopyMemory(tableEntry, _newEntry, sizeof(FLOCK_CACHE_ENTRY));

		tableEntry->occupied = TRUE;
		g_cache.length++;

		if (collisionOccured)
		{
			PT_DBG_PRINT(PTDBG_TRACE_CACHE_COLLISION, ("FLock!%s: Info. Added with collision. Index is %d, steps counter %d, cache.length %d\n",
				__FUNCTION__, insertIndex, stepsToPlaceEntry, g_cache.length));
		}
	}
	else
	{
		PT_DBG_PRINT(PTDBG_TRACE_CACHE_COLLISION, ("FLock!%s: CRITICAL! No free index to use in cache table. Index is %d, cache.length %d\n", __FUNCTION__, insertIndex, g_cache.length));

		RtlZeroMemory(g_cache.cached, sizeof(FLOCK_CACHE_ENTRY) * g_cache.capacity);
		g_cache.length = 0;

		FLockCacheAdd(_newEntry);
	}
}

VOID FLockCacheUpdateOrAdd(__in PFLOCK_CACHE_ENTRY _newEntry)
{
	FLockCacheAdd(_newEntry);
}

VOID FLockCacheErase()
{
	RtlZeroMemory(g_cache.cached, sizeof(FLOCK_CACHE_ENTRY) * g_cache.capacity);

	g_cache.length = 0;
	g_cache.collisionMaxResolveOffset = 0;
}
