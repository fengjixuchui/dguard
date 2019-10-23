//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <iostream>
#include <windows.h>
#include <vector>
#include "../../../helpers/internal/log.h"

#define	FLOCK_DEVICE_LINK				L"\\DosDevices\\FLockFsFilter"
#define FLOCK_DEVICE_NAME				L"\\Device\\FLockFsFilter"
#define FLOCK_DEV_APP_LINK				L"\\\\.\\FlockFsFilter"

#define FLOCK_DEVICE					FILE_DEVICE_UNKNOWN

//
//	Status codes.
//

#define FLOCK_STATUS_SUCCESS			0
#define FLOCK_STATUS_ERROR				1
#define FLOCK_STATUS_NOT_FOUND			3
#define FLOCK_STATUS_PRESENT			4
#define FLOCK_STATUS_CANT_CHANGE		5
#define FLOCK_STATUS_HAVE_NO_BODY		6
#define FLOCK_STATUS_SMALL_BUFFER		7
#define FLOCK_STATUS_WRONG_DATA			8
#define FLOCK_STATUS_WRONG_SIZE			9
#define FLOCK_STATUS_NOT_LOADED			10

//
//	Signature for secure identificating a request to the driver.
//

#define FLOCK_REQUEST_SIGNATURE			{0xA3, 0xFE, 0x01, 0x14, /*1*/ 0xE2, 0xCE, 0x77, 0x21, /*2*/ 0xF3, 0x12, 0x12, 0x01 /*3*/, 0x28, 0x03, 0x19, 0x00 /*4*/}
#define FLOCK_RESPONSE_SIGNATURE		{0x11, 0xC3, 0x21, 0x94, /*1*/ 0xA2, 0xFE, 0x60, 0x08, /*2*/ 0xAA, 0xBE, 0xD3, 0x38 /*3*/, 0x48, 0x51, 0x23, 0x00 /*4*/}

#define FLOCK_META_SIGNATURE			{0xB1, 0x0E, 0x21, 0xf4, /*1*/ 0xb2, 0x1E, 0x27, 0x21, /*2*/ 0x12, 0x12, 0x12, 0x12 /*3*/, 0x28, 0x33, 0x92, 0x11 /*4*/}
#define FLOCK_META_NAME					"FLOCK_META" /* 10 bytes */
#define FLOCK_META_NAME_SIZE			10
#define FLOCK_UNIQUE_ID_LENGTH			16


//	Provides information about internal state of the driver.
#define IOCTL_FLOCK_COMMON_INFO				CTL_CODE(FLOCK_DEVICE, 0x0710, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Detaches service from driver.
//
#define IOCTL_FLOCK_UNREGISTER_SERVICE		CTL_CODE(FLOCK_DEVICE, 0x0713, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This is a service registration request.
// There is could be registered only one service.
// Service could be registered twice or more times only if it was crashed or restarted. 
//

#define IOCTL_FLOCK_REGISTER_SERVICE		CTL_CODE(FLOCK_DEVICE, 0x0714, METHOD_BUFFERED, FILE_ANY_ACCESS)

//	Reads flock-meta from file's EAs.
#define IOCTL_FLOCK_READ_META				CTL_CODE(FLOCK_DEVICE, 0x0717, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Writes flock-meta into EAs.
#define IOCTL_FLOCK_MARK_FILE				CTL_CODE(FLOCK_DEVICE, 0x0725, METHOD_BUFFERED, FILE_ANY_ACCESS)

//	Zeros (makes invalid) flock-meta attributes in file's EAs.
#define IOCTL_FLOCK_MAKE_BAD				CTL_CODE(FLOCK_DEVICE, 0x0718, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Removes all flock entries in driver's storage.
#define IOCTL_FLOCK_CLEAR_ALL				CTL_CODE(FLOCK_DEVICE, 0x0723, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Returns info abut the storage - Was the storage loaded correctly?
#define IOCTL_FLOCK_STORAGE_LOADED			CTL_CODE(FLOCK_DEVICE, 0x0724, METHOD_BUFFERED /*METHOD_NEITHER*/, FILE_ANY_ACCESS)

// Was the storage file opened?
#define IOCTL_FLOCK_STORAGE_FILE_OPENED		CTL_CODE(FLOCK_DEVICE, 0x0740, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Query list of all FLocks with detailed information.
#define IOCTL_FLOCK_QUERY_LIST				CTL_CODE(FLOCK_DEVICE, 0x0715, METHOD_BUFFERED /*METHOD_NEITHER*/, FILE_ANY_ACCESS)

// Request to flush flocks from memory to disk with overriding old data.
#define IOCTL_FLOCK_STORAGE_FLUSH			CTL_CODE(FLOCK_DEVICE, 0x0741, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FLOCK_STORAGE_ADD				CTL_CODE(FLOCK_DEVICE, 0x0716, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Returns info about one flock.
#define IOCTL_FLOCK_STORAGE_QUERY_ONE		CTL_CODE(FLOCK_DEVICE, 0x0719, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Removes a flock from common flocks list in deriver storage.
#define IOCTL_FLOCK_STORAGE_REMOVE			CTL_CODE(FLOCK_DEVICE, 0x0720, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Verifies presence of a flock in common list of known flocks.
#define IOCTL_FLOCK_STORAGE_PRESENT			CTL_CODE(FLOCK_DEVICE, 0x0721, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Let as enable\disable protection for the flock.
#define IOCTL_FLOCK_STORAGE_UPDATE_FLAGS	CTL_CODE(FLOCK_DEVICE, 0x0722, METHOD_BUFFERED, FILE_ANY_ACCESS)


/************************************************************************/
/*        IOCTLs for driver managing						           */
/************************************************************************/

#define IOCTL_FLOCK_SET_DBGOUTPUT			CTL_CODE(FLOCK_DEVICE, 0x0801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FLOCK_SHUTDOWN				CTL_CODE(FLOCK_DEVICE, 0x0802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FLOCK_CACHE_RESIZE			CTL_CODE(FLOCK_DEVICE, 0x0803, METHOD_BUFFERED, FILE_ANY_ACCESS)

//	Helps to disable or enable use of internal cache.
#define IOCTL_FLOCK_CACHE_ENABLE			CTL_CODE(FLOCK_DEVICE, 0x0805, METHOD_BUFFERED, FILE_ANY_ACCESS)

//	Helps to clear internal cache.
#define IOCTL_FLOCK_CACHE_CLEAR				CTL_CODE(FLOCK_DEVICE, 0x0806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FLOCK_CACHE_XXXXXX			CTL_CODE(FLOCK_DEVICE, 0x0807, METHOD_BUFFERED, FILE_ANY_ACCESS)

//	Forces driver to generate new time stamp value for invalidating all available contexts.
#define IOCTL_FLOCK_CONTEXT_RESET			CTL_CODE(FLOCK_DEVICE, 0x0808, METHOD_BUFFERED, FILE_ANY_ACCESS)

//	Provides an ability to install or disable using contexts.
#define IOCTL_FLOCK_CONTEXT_ENABLE			CTL_CODE(FLOCK_DEVICE, 0x0809, METHOD_BUFFERED, FILE_ANY_ACCESS)



/************************************************************************/
/*       Flock policy flags									           */
/************************************************************************/

//
// Flag says that the directory includes objects which should be protected.
//
#define FLOCK_FLAG_HAS_FLOCKS		0x01

//
// Flag says that we need ho have an access to the file.
//
#define FLOCK_FLAG_HIDE				0x02

//
// Flag says that we need to protect an access to the file.
//
#define FLOCK_FLAG_LOCK_ACCESS		0x04

//////////////////////////////////////////////////////////////////////////


namespace driver
{
	//
	// Internal driver structure declarations.
	//

#pragma pack(push, 1)

	typedef struct _FLOCK_REQUEST_HEADER
	{
		UCHAR signature[16]; // FLOCK_REQUEST_SIGNATURE
		DWORD version;
		DWORD requestId;
		DWORD length; // Body part size in bytes.

		union
		{
			BOOLEAN boolValue;
			DWORD context;
			DWORD counter;
		} params;

	}FLOCK_REQUEST_HEADER, *PFLOCK_REQUEST_HEADER;

	typedef struct _FLOCK_RESPONSE_HEADER
	{
		UCHAR signature[16]; // FLOCK_RESPONSE_SIGNATURE
		DWORD version;
		DWORD flockStatus; // FLOCK_STATUS_XXX
		DWORD length; // Size of response body in bytes.

		union
		{
			BOOLEAN boolValue;
			DWORD context;
			DWORD requireLength;
		}params;

	} FLOCK_RESPONSE_HEADER, *PFLOCK_RESPONSE_HEADER;

	typedef struct _FLOCK_META
	{
		UCHAR signature[16];
		DWORD version; /* zero by default */
		UCHAR uniqueId[16];
		DWORD flags;
	} FLOCK_META, *PFLOCK_META;

	typedef struct _FLOCK_FILE_PATH
	{
		USHORT filePathLength; // in bytes.
		WCHAR filePath[1]; // It can not include the last zero symbol.
	} FLOCK_FILE_PATH, *PFLOCK_FILE_PATH;

	typedef struct _FLOCK_REQUEST_MARK_FILE
	{
		FLOCK_META info;
		USHORT filePathLength; // in bytes.
		WCHAR filePath[1];
	} FLOCK_REQUEST_MARK_FILE, *PFLOCK_REQUEST_MARK_FILE;

	typedef struct _FLOCK_STORAGE_ENTRY
	{
		UCHAR version;
		UCHAR id[16];
		ULONG32 flockFlag;
	} FLOCK_STORAGE_ENTRY, *PFLOCK_STORAGE_ENTRY;

	typedef struct _FLOCK_REQUEST_ADD {
		FLOCK_REQUEST_HEADER header;
		FLOCK_STORAGE_ENTRY flock;
	} FLOCK_REQUEST_ADD;

	typedef struct _FLOCK_REQUEST_SET_FLAG
	{
		UCHAR flockId[16]; // unique id.
		BOOLEAN toSet; // TRUE if need to raise a flag, remove means remove the flag.
		ULONG flockFlag; // FLOCK_FLAG_HIDE , FLOCK_FLAG_LOCK_ACCESS, FLOCK_FLAG_XXX and etc.

	}FLOCK_REQUEST_SET_FLAG, *PFLOCK_REQUEST_SET_FLAG;

	typedef struct _FLOCK_REQUEST_QUERY_INFO
	{
		UCHAR uniqueId[FLOCK_UNIQUE_ID_LENGTH];
	}FLOCK_REQUEST_QUERY_INFO, *PFLOCK_REQUEST_QUERY_INFO;

	typedef struct _FLOCK_RESPONSE_QUERY_INFO
	{
		FLOCK_STORAGE_ENTRY info;
	}FLOCK_RESPONSE_QUERY_INFO, *PFLOCK_RESPONSE_QUERY_INFO;

	typedef struct _REQUEST_FLOCK_INFO
	{
		FLOCK_REQUEST_HEADER header;
		FLOCK_REQUEST_QUERY_INFO flockId;
	} REQUEST_FLOCK_INFO, *PREQUEST_FLOCK_INFO;

	typedef struct _RESPONSE_GET_ONE_FLOCK
	{
		FLOCK_RESPONSE_HEADER header;
		FLOCK_STORAGE_ENTRY entry;
	} RESPONSE_GET_ONE_FLOCK, *PRESPONSE_GET_ONE_FLOCK;

	typedef struct _FLOCK_TIME_STAMP
	{
		union
		{
			LARGE_INTEGER stamp;
			UCHAR signature[8];
		};

	} FLOCK_TIME_STAMP, *PFLOCK_TIME_STAMP;


	//
	//	These structures reflect all internal information of FLock driver.
	//

	typedef struct _FLOCK_CACHE_INFO
	{
		BOOLEAN enabled;

		ULONG capacity;
		ULONG currentSize;
		ULONG occupancyLimit;
		ULONG collisionResolveIfNoPlaceBorder;
		ULONG collisionMaxResolveOffset;

	} FLOCK_CACHE_INFO, *PFLOCK_CACHE_INFO;


	typedef struct _FLOCK_COMMON_INFO
	{
        DWORD version;
		DWORD serviceProcessId;

		BOOLEAN stopAll;
		BOOLEAN createProcessNotificatorRegistered;
		BOOLEAN storageLoaderFinished;

		ULONG traceFlags;
		ULONG flocksCount;
		BOOLEAN storageLoaded;

		BOOLEAN ctxEnabled;
		FLOCK_TIME_STAMP ctxLastStamp;

		FLOCK_CACHE_INFO cache;
	} FLOCK_COMMON_INFO, *PFLOCK_COMMON_INFO;


	typedef struct _RESPONSE_FLOCK_INTERNAL_INFO
	{
		FLOCK_RESPONSE_HEADER header;
		FLOCK_COMMON_INFO info;
	} RESPONSE_FLOCK_INTERNAL_INFO, *PRESPONSE_FLOCK_INTERNAL_INFO;

#pragma pack(pop)


	class ClientFLock
	{
	public:

		ClientFLock(logfile& _log);
		~ClientFLock();

		bool canConnect();

		bool registerAsService();

		bool unregisterAsService();

		bool shutdown();

		bool storageClear();

		bool storageFlush();

		bool storageIsOpen();

		bool setDbgPrintFlags(ULONG _flags, ULONG& _oldFlags);

		bool readFileMeta(std::wstring _path, FLOCK_META& _metaInformation);

		bool verifyMeta(std::wstring _path);

		bool writeFileMeta(std::wstring _path, FLOCK_META& _meta);

		bool makeMetaInvalid(std::wstring _path);

		bool storageGetAll(std::vector<FLOCK_STORAGE_ENTRY>& _flocks);

		bool storageGetOne(std::string _flock16bytesId, FLOCK_STORAGE_ENTRY& _flock);

		bool storageAdd(FLOCK_STORAGE_ENTRY _flock);

		bool storageSetFlags(std::string _flock16bytesId, bool _toSet, ULONG32 _flags);

		bool storageRemove(std::string _flock16bytesId);

		bool storagePresent(std::string _flock16bytesId);

		bool ctxReset();

		bool ctxEnable(bool _toEnable);

		bool getDriverInfo(FLOCK_COMMON_INFO& _info);

		bool cacheClear();

		bool cacheReSize(ULONG _newTableSize);

		bool cacheEnable(bool _toEnable);

		bool cacheGetInfo(/* Return info here in arguments. */);


	private:
		logfile& m_log;
		//HANDLE h_driver;

		FLOCK_REQUEST_HEADER buildNewRequest();
		HANDLE getDriverHandle();
	};
}
