//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//


#include "ClientFLockNative.h"
#include <vector>


namespace driver
{

	ClientFLock::ClientFLock(logfile& _log) :m_log(_log)
	{
		m_log.print(std::string(__FUNCTION__) + ": created.");
	}

	ClientFLock::~ClientFLock()
	{
		m_log.print(std::string(__FUNCTION__) + ": closed.");
	}

	bool ClientFLock::canConnect()
	{
		bool driverConnected = false;
		HANDLE hDrv = getDriverHandle();

		if (driverConnected = (hDrv != NULL))
		{
			CloseHandle(hDrv);
		}

		return driverConnected;
	}

	bool ClientFLock::registerAsService()
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_REGISTER_SERVICE,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error 0x%x - could not be registered.", __FUNCTION__, response.flockStatus);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DeviceIoControl failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::unregisterAsService()
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_UNREGISTER_SERVICE,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error 0x%x - could not be unregistered.", __FUNCTION__, response.flockStatus);
			}
		} else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DeviceIoControl failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::shutdown()
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_SHUTDOWN,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::setDbgPrintFlags(ULONG _flags, ULONG& _oldFlags)
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		request.params.context = _flags;

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_SET_DBGOUTPUT,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else{
				// Success - I changed debug output flag in a driver. Copy old one.
				_oldFlags = response.params.context;
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::readFileMeta(std::wstring _path, FLOCK_META& _metaInformation)
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		// Input data for driver.
		DWORD inputSize = sizeof(FLOCK_REQUEST_HEADER) + sizeof(FLOCK_FILE_PATH) + (_path.length()*sizeof(WCHAR));
		unsigned char* pInput = (unsigned char*) new (std::nothrow) unsigned char[inputSize];

		if (pInput == NULL) {

			m_log.printEx("%s: error - could not reserve enough memory, required size is %d.", __FUNCTION__, inputSize);
			CloseHandle(hDrv);
			return false;
		}

		// Output.
		DWORD outputSize = sizeof(FLOCK_RESPONSE_HEADER) + sizeof(FLOCK_META) + 10;
		unsigned char* pOutput = (unsigned char*) new (std::nothrow) unsigned char[outputSize];

		if (pOutput == NULL) {

			m_log.printEx("%s: error - could not reserve enough ->memory, required size is %d.", __FUNCTION__, outputSize);
			delete [] pInput;
			CloseHandle(hDrv);
			return false;
		}

		//
		// Prepare request to driver.
		//
		memcpy(pInput, &request, sizeof(request));

		FLOCK_REQUEST_HEADER* header = (FLOCK_REQUEST_HEADER*)(pInput /*+sizeof(FLOCK_REQUEST_HEADER)*/);
		header->length = sizeof(FLOCK_FILE_PATH) + ( _path.length() * sizeof(WCHAR) );

		PFLOCK_FILE_PATH pathInfo = (PFLOCK_FILE_PATH)(pInput + sizeof(FLOCK_REQUEST_HEADER));
		pathInfo->filePathLength = (_path.length() * sizeof(WCHAR));
		memcpy(pathInfo->filePath, _path.c_str(), _path.length() * sizeof(WCHAR));

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_READ_META,
			pInput,
			inputSize,
			pOutput,
			outputSize,
			&cdReturned,
			NULL
			);

		if (res) {

			PFLOCK_RESPONSE_HEADER responseHeader = (PFLOCK_RESPONSE_HEADER)pOutput;
			
			if (!(res = (responseHeader->flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, responseHeader->flockStatus);
			}
			else {
				memcpy(&_metaInformation, pOutput + sizeof(FLOCK_RESPONSE_HEADER), sizeof(FLOCK_META));
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		delete[] pInput;
		delete[] pOutput;

		CloseHandle(hDrv);
		return res == TRUE;
	}

	bool ClientFLock::verifyMeta(std::wstring _path)
	{
		FLOCK_META fm = { 0 };
		return readFileMeta(_path, fm);
	}

	bool ClientFLock::writeFileMeta(std::wstring _path, FLOCK_META& _meta)
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		// Input.
		DWORD inputSize = sizeof(FLOCK_REQUEST_HEADER) + sizeof(FLOCK_REQUEST_MARK_FILE) + /*sizeof(FLOCK_FILE_PATH)*/ + (_path.length() * sizeof(WCHAR));

		unsigned char* pInput = (unsigned char*) new (std::nothrow) unsigned char[inputSize];

		if (pInput == NULL){
			m_log.printEx("\n%s: error - *could not reserve memory failed.\n", __FUNCTION__);
			return false;
		}

		// Output.
		DWORD outputSize = sizeof(FLOCK_RESPONSE_HEADER);

		// Prepare request to driver.
		memcpy(pInput, &request, sizeof(request));

		FLOCK_REQUEST_HEADER* header = (FLOCK_REQUEST_HEADER*)(pInput);
		header->length = sizeof(FLOCK_REQUEST_MARK_FILE) + (_path.length() * sizeof(WCHAR));

		PFLOCK_REQUEST_MARK_FILE requestBody = (PFLOCK_REQUEST_MARK_FILE)(pInput + sizeof(FLOCK_REQUEST_HEADER));
		memcpy(&requestBody->info, &_meta, sizeof(_meta));

		requestBody->filePathLength = (_path.length() * sizeof(WCHAR));
		memcpy(requestBody->filePath, _path.c_str(), _path.length() * sizeof(WCHAR));

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_MARK_FILE,
			pInput,
			inputSize,
			&response,
			outputSize,
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else{
				// Success - I file's meta were changed.
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		delete[] pInput;
		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageClear()
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_CLEAR_ALL,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else{
				m_log.printEx("%s: success.", __FUNCTION__);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageFlush()
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_FLUSH,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else{
				m_log.printEx("%s: success.", __FUNCTION__);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageIsOpen()
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_FILE_OPENED,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageGetAll(std::vector<FLOCK_STORAGE_ENTRY>& _flocks)
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_HEADER request = buildNewRequest();
		FLOCK_RESPONSE_HEADER* responseHeader;
		DWORD cbReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		// Output.
		DWORD outputSize = sizeof(FLOCK_RESPONSE_HEADER) + 4096;
		UCHAR* pOutput = new (std::nothrow) UCHAR[outputSize];

		if (outputSize == NULL)
		{
			m_log.printEx("%s: error - could not allocate enough memory, required %d bytes.", __FUNCTION__, outputSize);
			CloseHandle(hDrv);
			return false;
		}

		ZeroMemory(pOutput, outputSize);

		responseHeader = (FLOCK_RESPONSE_HEADER*) pOutput;

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_QUERY_LIST,
			&request,
			sizeof(request),
			pOutput,
			outputSize,
			&cbReturned,
			NULL
			);

		if (res) {
			if (res = (responseHeader->flockStatus == FLOCK_STATUS_SUCCESS)) {

				FLOCK_STORAGE_ENTRY* flockStorageEntry = (FLOCK_STORAGE_ENTRY*)(((UCHAR*)pOutput) + sizeof(FLOCK_RESPONSE_HEADER));

				for (ULONG i = 0; i < responseHeader->params.context; i++) {
					_flocks.push_back(*flockStorageEntry);
					flockStorageEntry++;
				}
			}
			else
			{
				if (responseHeader->flockStatus == FLOCK_STATUS_SMALL_BUFFER)
				{
					m_log.printEx("%s: flock error - input buffer is too small.", __FUNCTION__);

					//
					// Prepare the second request with larger output buffer.
					//

					cbReturned = 0;
					outputSize = responseHeader->params.requireLength;
					PUCHAR pBuffer = new (std::nothrow) UCHAR[outputSize];

					if (pBuffer)
					{
						responseHeader = (FLOCK_RESPONSE_HEADER*)pBuffer;

						res = DeviceIoControl(
							hDrv,
							IOCTL_FLOCK_QUERY_LIST,
							&request,
							sizeof(request),
							pBuffer,
							outputSize,
							&cbReturned,
							NULL
							);

						if (res) {
							if (res = (responseHeader->flockStatus == FLOCK_STATUS_SUCCESS)) {

								FLOCK_STORAGE_ENTRY* flockStorageEntry = (FLOCK_STORAGE_ENTRY*)(((UCHAR*)pBuffer) + sizeof(FLOCK_RESPONSE_HEADER));

								for (ULONG i = 0; i < responseHeader->params.context; i++) {
									_flocks.push_back(*flockStorageEntry);
									flockStorageEntry++;
								}
							} else {
								m_log.printEx("%s: flock error 0x%x - second request failed too.", __FUNCTION__, responseHeader->flockStatus);
							}
						} else {
							DWORD lerror = GetLastError();
							m_log.printEx("%s: error - the second DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
						}

						delete[] pBuffer;
					}
				}
				else
				{
					m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, responseHeader->flockStatus);
				}
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		delete[] pOutput;

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageGetOne(std::string _flock16bytesId, FLOCK_STORAGE_ENTRY& _flock)
	{
		HANDLE hDrv = getDriverHandle();
		REQUEST_FLOCK_INFO request = { 0 };
		RESPONSE_GET_ONE_FLOCK response = { 0 };
		DWORD cdReturned = 0;

		request.header = buildNewRequest();
		request.header.length = sizeof(request.flockId);

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			CloseHandle(hDrv);
			return false;
		}

		if (_flock16bytesId.empty() || (_flock16bytesId.size() > FLOCK_UNIQUE_ID_LENGTH))
		{
			m_log.printEx("%s: error - wrong flock id length is %d.", __FUNCTION__, _flock16bytesId.size());
			CloseHandle(hDrv);
			return false;
		}

		memcpy(request.flockId.uniqueId, _flock16bytesId.data(), _flock16bytesId.size());

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_QUERY_ONE,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.header.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.header.flockStatus);
			}
			else {
				// Copy flock entry in case of success.
				_flock = response.entry;
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageAdd(FLOCK_STORAGE_ENTRY _flock)
	{
		HANDLE hDrv = getDriverHandle();
		FLOCK_REQUEST_ADD request = { 0 };
		request.header = buildNewRequest();
		request.header.length = sizeof(request.flock);

		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("\n%s: error - could not get driver handle.\n", __FUNCTION__);
			return false;
		}

		// Copy flock storage entry to outgoing request.
		request.flock = _flock;

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_ADD,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else{
				m_log.printEx("%s: success.", __FUNCTION__);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageSetFlags(std::string _flock16bytesId, bool _toSet, ULONG32 _flags)
	{
		struct REQUEST_TO_CHANGE_FLOCK_FLAGS
		{
			FLOCK_REQUEST_HEADER header;
			FLOCK_REQUEST_SET_FLAG setFlagInfo;
		};

		HANDLE hDrv = getDriverHandle();
		REQUEST_TO_CHANGE_FLOCK_FLAGS request = { 0 };
		request.header = buildNewRequest();
		request.header.length = sizeof(FLOCK_REQUEST_SET_FLAG);

		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if ( _flock16bytesId.empty() || (_flock16bytesId.size() > FLOCK_UNIQUE_ID_LENGTH) )
		{
			// ID should be shorter than 16 bytes.
			m_log.printEx("%s: error - wrong flock id length is %d.", __FUNCTION__, _flock16bytesId.size());
			
			CloseHandle(hDrv);
			return false;
		}

		// Prepare structure for kernel mode driver.
		request.setFlagInfo.toSet = _toSet;
		request.setFlagInfo.flockFlag = _flags;

		// Copy flock id here.
		int lim = _flock16bytesId.length();
		for (int i = 0; i < lim; ++i) {
			request.setFlagInfo.flockId[i] = _flock16bytesId.at(i);
		}

		if (!hDrv){
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			return false;
		}

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_UPDATE_FLAGS,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else{
				m_log.printEx("%s: success.", __FUNCTION__);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storageRemove(std::string _flock16bytesId)
	{
		struct REQUEST_TO_REMOVE_FLOCK
		{
			FLOCK_REQUEST_HEADER header;
			FLOCK_REQUEST_QUERY_INFO flockId;
		};

		HANDLE hDrv = getDriverHandle();
		REQUEST_TO_REMOVE_FLOCK request = { 0 };
		request.header = buildNewRequest();
		request.header.length = sizeof(FLOCK_REQUEST_QUERY_INFO);

		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			CloseHandle(hDrv);
			return false;
		}

		if (_flock16bytesId.empty() || (_flock16bytesId.size() > FLOCK_UNIQUE_ID_LENGTH))
		{
			// ID should be shorter or equal to 16 bytes.
			m_log.printEx("%s: error - wrong flock id length is %d.", __FUNCTION__, _flock16bytesId.size());

			CloseHandle(hDrv);
			return false;
		}

		memcpy(request.flockId.uniqueId, _flock16bytesId.data(), _flock16bytesId.size());

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_REMOVE,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_SUCCESS))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);
			}
			else {
				m_log.printEx("%s: success.", __FUNCTION__);
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	bool ClientFLock::storagePresent(std::string _flock16bytesId)
	{
		HANDLE hDrv = getDriverHandle();
		REQUEST_FLOCK_INFO request = { 0 };
		FLOCK_RESPONSE_HEADER response = { 0 };
		DWORD cdReturned = 0;

		request.header = buildNewRequest();
		request.header.length = sizeof(FLOCK_REQUEST_QUERY_INFO);

		if (!hDrv) {
			m_log.printEx("%s: error - could not get driver handle.", __FUNCTION__);
			CloseHandle(hDrv);
			return false;
		}

		if (_flock16bytesId.empty() || (_flock16bytesId.size() > FLOCK_UNIQUE_ID_LENGTH))
		{
			m_log.printEx("%s: error - wrong flock id length is %d.", __FUNCTION__, _flock16bytesId.size());
			CloseHandle(hDrv);
			return false;
		}

		memcpy(request.flockId.uniqueId, _flock16bytesId.data(), _flock16bytesId.size());

		BOOL res = DeviceIoControl(
			hDrv,
			IOCTL_FLOCK_STORAGE_PRESENT,
			&request,
			sizeof(request),
			&response,
			sizeof(response),
			&cdReturned,
			NULL
			);

		if (res) {
			if (!(res = (response.flockStatus == FLOCK_STATUS_PRESENT))) {
				m_log.printEx("%s: flock error is 0x%x.", __FUNCTION__, response.flockStatus);

				if (response.flockStatus == FLOCK_STATUS_NOT_FOUND) {
					m_log.printEx("%s: flock is not found.", __FUNCTION__);
				}
			}
			else {
				// Success, flock is present in driver's storage.
			}
		}
		else {
			DWORD lerror = GetLastError();
			m_log.printEx("%s: error - DevIo failed with 0x%x last error.", __FUNCTION__, lerror);
		}

		CloseHandle(hDrv);
		return (res == TRUE);
	}

	FLOCK_REQUEST_HEADER ClientFLock::buildNewRequest()
	{
		FLOCK_REQUEST_HEADER request = { 0 };
		UCHAR signature[] = FLOCK_REQUEST_SIGNATURE;

		request.length = 0;
		request.version = 0;
		memcpy( request.signature, &signature, sizeof(request.signature) );

		return request;
	}

	HANDLE ClientFLock::getDriverHandle()
	{
		HANDLE hDev = CreateFileW(FLOCK_DEV_APP_LINK,
			FILE_WRITE_ACCESS | FILE_READ_ACCESS,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hDev == INVALID_HANDLE_VALUE)
		{
			DWORD errorCode = GetLastError();

			m_log.printEx("%s: error - CreateFile failed, last error 0x%x.", __FUNCTION__, errorCode);
			return NULL;
		}

		return hDev;
	}

}
