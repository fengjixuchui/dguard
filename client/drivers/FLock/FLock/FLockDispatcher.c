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

#include "flock.h"
#include "FLockStorage.h"



EXTERN_C ULONG gTraceFlags;


static const char* MSG_WRONG_INPUT_REQUEST = "FLock!%s: error - request buffer is too small.\n";
static const char* MSG_INVALID_REQUEST = "FLock!%s: error - can't treat the incoming request as FLock request, validation failed.\n";
static const char* MSG_WRONG_REQUEST_BODYSIZE = "FLock!%s: error - wrong size of the request body, request failed.\n";



//
// Verifies on request signature and copies data to '_outHeader' if it is.
// Returns TRUE when the passed _buffer has right FLOCK_REQUEST_SIGNATURE.
//
BOOLEAN FLockGetRequestHeader(
	PVOID _buffer,
	PFLOCK_REQUEST_HEADER _outHeader
	)
{
	UCHAR requestSignature[16] = FLOCK_REQUEST_SIGNATURE;

	if (_buffer != NULL)
	{
		PFLOCK_REQUEST_HEADER pHdr = (PFLOCK_REQUEST_HEADER)_buffer;

		if (memcmp(pHdr->signature, requestSignature, sizeof(requestSignature)) == 0)
		{
			if (_outHeader)
			{
				RtlCopyMemory(_outHeader, pHdr, sizeof(FLOCK_REQUEST_HEADER) );
				return TRUE;
			}
		}
	}

	return FALSE;
}

//
// Please do not forget to free memory for '_outBodyBuffer' using ExFreePool(..).
//
BOOLEAN FLockGetRequestAndBody(
	ULONG _inputBufferSize,
	PVOID _ptrRawRequestData,
	PFLOCK_REQUEST_HEADER _copyToHeader,
	PUCHAR* _outBodyBuffer
	)
{
	if ((!_copyToHeader) || (!_outBodyBuffer))
	{
		return FALSE;
	}

	BOOLEAN result = FLockGetRequestHeader(_ptrRawRequestData, _copyToHeader);

	if (result)
	{
		*_outBodyBuffer = NULL;

		//
		// Verify actual size for the body part.
		//
		if ( _inputBufferSize < (sizeof(FLOCK_REQUEST_HEADER) + _copyToHeader->length) )
		{
			//
			// This is a wrong size in request header for the body part.
			// Actually an input buffer is less than asked.
			//
			return FALSE;
		}

		if (_copyToHeader->length)
		{
			PUCHAR pNewCopyOfRequestBodyPart = ExAllocatePool(PagedPool, _copyToHeader->length);
			result = (pNewCopyOfRequestBodyPart != NULL); // FALSE if we have no memory.

			if (result)
			{
				//PUCHAR copyFrom = ((PUCHAR)_ptrRawRequestData + _copyToHeader->length);
				PUCHAR copyFrom = ((PUCHAR)_ptrRawRequestData + sizeof(FLOCK_REQUEST_HEADER));
				RtlCopyMemory(pNewCopyOfRequestBodyPart, copyFrom, _copyToHeader->length);

				*_outBodyBuffer = pNewCopyOfRequestBodyPart;
				// *_outBodyBuffer = /*copyFrom*/;
			}
			else
			{
				// We have no memory.
			}
		}
	}

	return result;
}

//BOOLEAN FLockHasPlaceFor

void FLockFreeBody(
	PVOID _p
	)
{
	if (_p != NULL) {
		ExFreePool(_p);
	}
}



PFLOCK_RESPONSE_HEADER FLockPrepareResponse(PVOID _buffer, DWORD _flockStatus)
{
	PFLOCK_RESPONSE_HEADER pResponseHeader = (PFLOCK_RESPONSE_HEADER)_buffer;
	RtlZeroMemory(pResponseHeader, sizeof(FLOCK_RESPONSE_HEADER));

	UCHAR signatureBuffer[] = FLOCK_RESPONSE_SIGNATURE;
	RtlCopyMemory(pResponseHeader->signature, signatureBuffer, sizeof(signatureBuffer));
	pResponseHeader->version = 0;
	pResponseHeader->flockStatus = _flockStatus;

	return pResponseHeader;
}


void FLockWriteResponseBody(PFLOCK_RESPONSE_HEADER _response, PVOID _dataWriteToResponseBody, ULONG _responseBodySize)
{
	PUCHAR ptrBody = (PUCHAR)( ((PUCHAR)_response) + sizeof(FLOCK_RESPONSE_HEADER));
	_response->length = _responseBodySize;

	RtlCopyMemory(ptrBody, _dataWriteToResponseBody, _responseBodySize);
}


BOOLEAN FLockWriteResponse(PVOID _responseHeader, ULONG _outputBufferSize, PVOID _body, ULONG _bodySize)
{
	BOOLEAN res = (_outputBufferSize >= (_bodySize + sizeof(FLOCK_RESPONSE_HEADER)));

	if ( res )
	{
		PUCHAR pResponseBody = ((PUCHAR)_responseHeader) + sizeof(FLOCK_RESPONSE_HEADER);
		RtlCopyMemory(pResponseBody, _body, _bodySize);
	}

	return res;
}

//
// Returns TRUE input buffer has enough size to hold FLOCK_REQUEST_HEADER and
// if output buffer has enough size to hold FLOCK_RESPONSE_HEADER.
//
BOOLEAN FLockCanProcessRequest(
	__in ULONG ioInputSize,
	__in ULONG ioOutputSize
	)
{
	return (ioInputSize >= sizeof(FLOCK_REQUEST_HEADER)) && (ioOutputSize >= sizeof(FLOCK_RESPONSE_HEADER));
}


//
// Returns TRUE if output buffer can keep response header and required body part.
//
BOOLEAN FlockHasPlaceForResponseAndBody(ULONG _outputBufferSize, ULONG _bodyPartSize)
{
	return (_outputBufferSize >= (sizeof(FLOCK_RESPONSE_HEADER) + _bodyPartSize));
}


NTSTATUS FLockSuccessDispatcher(PDEVICE_OBJECT _deviceObject, PIRP _irp)
{
	UNREFERENCED_PARAMETER(_deviceObject);

	_irp->IoStatus.Status = STATUS_SUCCESS;
	_irp->IoStatus.Information = 0;

	IoCompleteRequest(_irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// Dispatch handler for user-mode requests handler.
//

NTSTATUS FLockDeviceControlDispatcher(PDEVICE_OBJECT _deviceObject, PIRP _irp)
{
	UNREFERENCED_PARAMETER(_deviceObject);

	NTSTATUS ioRequestStatus = STATUS_SUCCESS, dispatcherFinalStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION stack;
	FLOCK_REQUEST_HEADER requestHeader = { 0 };
	PUCHAR requestBody = NULL;
	DWORD ioWrittenBytes = 0;
	ULONG count = 0;
	PFLOCK_STORAGE_ENTRY pStorageEntry = NULL;

	stack = IoGetCurrentIrpStackLocation(_irp);

	// NEITHER method
	// PVOID pInput = stack->Parameters.DeviceIoControl.Type3InputBuffer;
	// PVOID pOutput = _irp->UserBuffer;

	DWORD ioBufferedInputSize = stack->Parameters.DeviceIoControl.InputBufferLength;
	DWORD ioBufferedOutputSize = stack->Parameters.DeviceIoControl.OutputBufferLength;

	PVOID ioBufferedInput = _irp->AssociatedIrp.SystemBuffer;
	PVOID ioBufferedOutput = _irp->AssociatedIrp.SystemBuffer;

	if (stack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - unknown error irp->MajorFunction code ", __FUNCTION__, stack->MajorFunction));

		_irp->IoStatus.Information = 0;
		_irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
		IoCompleteRequest(_irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_FLOCK_GET_SERVICE:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			if (FLockGetServiceProcess()) {
				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS)->params.context = FLockGetServiceProcessId();
			} else {
				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_UNREGISTER_SERVICE:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			//
			// Unregister can only that process which was registered as service.
			//
			if ( FLockAreWeInServiceProcessContext() )
			{
				FLockUnregisterServiceProcess();

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS)->params.context = FLockGetServiceProcessId();

				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - service process was unregistered.\n", __FUNCTION__));
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - can't unregister the service. Current process is %d PID, but service process is %d PID.\n",
					__FUNCTION__,
					(DWORD)PsGetProcessId(PsGetCurrentProcess()),
					FLockGetServiceProcessId()) );

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR)->params.context = FLockGetServiceProcessId();
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_REGISTER_SERVICE:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			// There are no one service process.
			if (FLockGetServiceProcessId() == 0)
			{
				//
				// Register service process here.
				//
				FLockRegisterServiceProcess(PsGetCurrentProcess());

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS)->params.context = FLockGetServiceProcessId();

				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - service process (%d PID) was registered.\n",
					__FUNCTION__, FLockGetServiceProcessId()));
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - ignore incoming request, because service (%d PID) was registered earlier.\n",
					__FUNCTION__, FLockGetServiceProcessId()));

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR)->params.context = FLockGetServiceProcessId();
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_ENABLE_UNLOADING:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (requestHeader.params.context != FALSE)
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: Enable unload.\n", __FUNCTION__));
					FLockData()->driverObject->DriverUnload = DriverUnload;
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: Disable unload.\n", __FUNCTION__));
					FLockData()->driverObject->DriverUnload = NULL;
				}

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);

				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_FILE_OPENED:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			// Now we have no any validations. It will be implemented later.
			// ...

			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			BOOLEAN opened = FLockStorageIsOpened();

			if (opened)
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: status - flock storage is opened.\n", __FUNCTION__));
				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: status - flock storage is not opened.\n", __FUNCTION__));
				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_NOT_LOADED);
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_LOADED:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			// Now we have no any validations. It will be implemented later.
			// ...

			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			BOOLEAN loaded = FLockStorageIsLoaded();

			if (loaded)
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: yes flock storage is loaded.\n", __FUNCTION__));
				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - flock storage is not loaded.\n", __FUNCTION__));

				if (FLockStorageIsOpened())
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: (!)info - flock storage is opened.\n", __FUNCTION__));
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: (!)info - flock storage was not even opened.\n", __FUNCTION__));
				}

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_NOT_LOADED);
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////
			
	case IOCTL_FLOCK_READ_META:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				// Body part should have minimal size which is greater or equal than size of empty FLOCK_FILE_PATH struct.
				if (requestHeader.length >= sizeof(FLOCK_FILE_PATH))
				{
					PFLOCK_FILE_PATH pFilePath = (PFLOCK_FILE_PATH) requestBody;

					//
					// Verify border for PFLOCK_FILE_PATH->filePath string.
					//
					ULONG actualFilePathBufferSize = requestHeader.length - sizeof(FLOCK_FILE_PATH) + sizeof(WCHAR);
					if (actualFilePathBufferSize < pFilePath->filePathLength)
					{
						//
						// This is an error, because actual size is smaller then in FLOCK_FILE_PATH::filePathLength.
						//

						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - wrong file path length, actual file path buffer size (%d bytes), but marked as  (%d bytes).\n",
							__FUNCTION__,
							actualFilePathBufferSize,
							pFilePath->filePathLength));

						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_DATA);
					}
					else
					{
						//
						// Build a string with file path.
						// An example of path: "\DosDevices\C:\flock_ea.txt", "\??\C:\flock_ea.txt".
						//

						UNICODE_STRING usFilePath = { 0 };
						usFilePath.Buffer = pFilePath->filePath;
						usFilePath.Length = pFilePath->filePathLength;
						usFilePath.MaximumLength = (USHORT)pFilePath->filePathLength;

						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: Request to verify flock's meta in - %wZ.\n", __FUNCTION__, &usFilePath));

						//
						// <--> Set global access on reading EAs.
						//

						FLOCK_META fm = { 0 };
						NTSTATUS operationErrorStatus = STATUS_UNSUCCESSFUL;

						BOOLEAN found = FLockFileReadFastFirstMeta2(&usFilePath, &fm, &operationErrorStatus);

						//
						// <--> Reset global access policy on reading EAs.
						//
						// (Note!): It is already implemented in handlers of IRP_MJ_SET_EA and IRP_MJ_QUERY_EA.
						//

						if (found)
						{
							if (FlockHasPlaceForResponseAndBody(ioBufferedOutputSize, sizeof(FLOCK_META)))
							{
								//
								// Success - flock-meta was found and response body buffer has a space for it.
								//

								PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: Success - flocks meta is found in %wZ.\n", __FUNCTION__, &usFilePath));

								PFLOCK_RESPONSE_HEADER responseHeader = FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
								FLockWriteResponseBody(responseHeader, &fm, sizeof(fm));

								// Total size to return data back to user application consist of response_header and flock_meta.
								ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER) + sizeof(fm);
							}
							else
							{
								PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - require more memory for the body part %d bytes.\n",
									__FUNCTION__,
									sizeof(FLOCK_META)));

								FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SMALL_BUFFER)->params.requireLength = sizeof( FLOCK_META );
							}
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - flocks meta was not found in %wZ.\n", __FUNCTION__, &usFilePath));

							// Flock-meta was not found.
							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_NOT_FOUND);
						}
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_REQUEST_BODYSIZE, __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_SIZE);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_MARK_FILE:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				PFLOCK_REQUEST_MARK_FILE requestMarkFile = (PFLOCK_REQUEST_MARK_FILE)requestBody;

				ULONG actualFilePathBufferSize = requestHeader.length - sizeof(FLOCK_REQUEST_MARK_FILE) + sizeof(WCHAR);

				if (actualFilePathBufferSize < requestMarkFile->filePathLength)
				{
					// This is an error, because actual size is smaller then in FLOCK_REQUEST_MARK_FILE::filePathLength.
					// ...

					PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - actual file path buffer size (%d bytes) is less then real (%d bytes).\n",
						__FUNCTION__,
						actualFilePathBufferSize,
						requestHeader.length));

					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_DATA);
				}
				else
				{
					//
					// It is Ok, we can write special flocks data to Extended-Attributes.
					//

					UNICODE_STRING usFilePath = { 0 };
					usFilePath.Buffer = requestMarkFile->filePath;
					usFilePath.Length = (USHORT)requestMarkFile->filePathLength;
					usFilePath.MaximumLength = (USHORT)requestMarkFile->filePathLength;
					//RtlInitUnicodeString(&usFilePath, requestMarkFile->szFilePath);

					UCHAR fmSignature[] = FLOCK_META_SIGNATURE;

					if (memcmp(requestMarkFile->info.signature, fmSignature, sizeof(fmSignature)) == 0)
					{
						NTSTATUS opStatus = STATUS_UNSUCCESSFUL;

						if (FLockFileWriteMeta2(&usFilePath, &requestMarkFile->info, &opStatus))
						{
							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - %wZ was marked with flock-meta.\n", __FUNCTION__, &usFilePath));
							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: info - %wZ was not marked with flock-meta, status is 0x%x\n", __FUNCTION__, &usFilePath, opStatus));
							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
						}
					}
					else
					{
						PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - wrong flock signature in incoming request.\n", __FUNCTION__));

						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_DATA);
					}
				}

				// Free request after all actions.
				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_MAKE_BAD:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				// Body part of the incoming request should have a minimal size to hold FLOCK_FILE_PATH.
				if (requestHeader.length >= sizeof(FLOCK_FILE_PATH))
				{
					PFLOCK_FILE_PATH pFilePath = (PFLOCK_FILE_PATH)requestBody;

					// Verify border for PFLOCK_FILE_PATH->filePath string to escape buffer overflow.
					ULONG actualFilePathBufferSize = requestHeader.length - sizeof(FLOCK_FILE_PATH) + sizeof(WCHAR);
					if (actualFilePathBufferSize < pFilePath->filePathLength)
					{
						//
						// Buffer overflow, actual size is smaller then in FLOCK_FILE_PATH::filePathLength.
						//

						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - wrong file path length, actual file path buffer size (%d bytes), but marked as  (%d bytes).\n",
							__FUNCTION__,
							actualFilePathBufferSize,
							pFilePath->filePathLength));

						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_DATA);
					}
					else
					{
						//
						// Build a string with file path.
						// An example of path: "\DosDevices\C:\flock_ea.txt", "\??\C:\flock_ea.txt".
						//

						UNICODE_STRING usFilePath = { 0 };
						usFilePath.Buffer = pFilePath->filePath;
						usFilePath.Length = pFilePath->filePathLength;
						usFilePath.MaximumLength = (USHORT)pFilePath->filePathLength;

						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: Request to make bad flock's meta in - %wZ.\n", __FUNCTION__, &usFilePath));

						// We can read\write FLock's meta freely only if we are in a context of main Data Guard service process.
						// Please note, that service should be registered earlier.

						// Kill FLock meta here =)
						FLOCK_META fm = { 0 };
						fm.signature[0] = 0xD;
						fm.signature[1] = 0xE;
						fm.signature[2] = 0xA;
						fm.signature[3] = 0xD;

						NTSTATUS errStatus = STATUS_UNSUCCESSFUL;

						// Write invalid FLock meta in file EAs.
						if (FLockFileWriteMeta2(&usFilePath, &fm, &errStatus))
						{
							PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: Success - FLock was killed in %wZ.\n", __FUNCTION__, &usFilePath));

							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - could not kill flock in %wZ.\n", __FUNCTION__, &usFilePath));

							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR)->params.context = errStatus;
						}
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_REQUEST_BODYSIZE, __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_SIZE);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_FLUSH:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			// Read request header and body.
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				//
				// Because that request executes in service manager context, we can not touch storage file
				// right now. It is require to flush data in 'System' process context.
				//

				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: generate flush event.\n", __FUNCTION__));

				SyncGenerateFlushEvent();

				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);

// 				if (FLockStorageFlushFromMemoryToDisk())
// 				{
// 					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: data were flushed from memory to disk.\n", __FUNCTION__));
// 					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS)->length = 0;
// 				}
// 				else
// 				{
// 					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - couldn't flush data from memory to disk.\n", __FUNCTION__));
// 					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
// 				}

				FLockFreeBody(requestBody);
			}
			else
			{
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////


	case IOCTL_FLOCK_QUERY_LIST:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			// Read request header and body.
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (FLockStorageGetAll(FALSE, &count, &pStorageEntry))
				{
					if (count)
					{
						// Calculate size of response body.
						ULONG responseBodyPartSize = count * sizeof(FLOCK_STORAGE_ENTRY);
						ULONG holeResponseSize = responseBodyPartSize + sizeof(FLOCK_RESPONSE_HEADER);

						// Output buffer is too small.
						if (ioBufferedOutputSize < holeResponseSize)
						{
							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: input buffer too small (%d bytes), but require %d bytes.\n", __FUNCTION__, ioBufferedOutputSize, holeResponseSize));

							PFLOCK_RESPONSE_HEADER respHeader = FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SMALL_BUFFER);
							respHeader->params.context = holeResponseSize;
							respHeader->length = 0;
						}
						else
						{
							PFLOCK_RESPONSE_HEADER respHeader = FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
							respHeader->params.context = count; // count of entries
							respHeader->length = responseBodyPartSize; // size of body part in bytes

							// Copy flock entries to body of the response.
							FLockWriteResponse(ioBufferedOutput, ioBufferedOutputSize, pStorageEntry, responseBodyPartSize);
						}

						if (pStorageEntry){
							ExFreePool(pStorageEntry);
						}
					}
					else // empty list
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: flock storage is empty.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS)->length = 0;
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - couldn't read data from storage.\n", __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0 /*sizeof(FLOCK_RESPONSE_HEADER)*/;
				// FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_UPDATE_FLAGS:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			// Read request header and body.
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (requestHeader.length >= sizeof(FLOCK_REQUEST_SET_FLAG))
				{
					// Can work with body part because it has right size.
					PFLOCK_REQUEST_SET_FLAG markInfo = (PFLOCK_REQUEST_SET_FLAG)requestBody;

					FLOCK_ID fid;
					RtlCopyMemory(fid.id, markInfo->flockId, sizeof(fid.id));

					FLOCK_STORAGE_ENTRY storageEntry = { 0 };
					if (FLockStorageLookup(fid.id, &storageEntry))
					{
						if (markInfo->toSet) {
							SetFlag(storageEntry.flockFlag, markInfo->flockFlag);
						} else {
							ClearFlag(storageEntry.flockFlag, markInfo->flockFlag);
						}

						// Update information about flock in driver's storage.
						if (FLockStorageUpdateFlags(&fid, storageEntry.flockFlag))
						{
							// Success - entry was changed.
							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - could not change flags.\n", __FUNCTION__));
							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_CANT_CHANGE);
						}
					}
					else
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - requested entry was not found.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_NOT_FOUND);
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - request has no body part.\n", __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_HAVE_NO_BODY);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0; // sizeof(FLOCK_RESPONSE_HEADER);
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_CLEAR_ALL:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			// Now we have no any validations. It will be implemented later.
			// ...

			//DbgBreakPoint();

			ioRequestStatus = STATUS_SUCCESS;
			ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

			if (FLockStorageClearInMemory())
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: the storage was cleared in memory.\n", __FUNCTION__));

				if (FLockStorageFlushFromMemoryToDisk())
				{
					//
					// Success! All storages were cleared.
					//

					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: the storage was cleared.\n", __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - couldn't clear the storage on disk.\n", __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
				}
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - couldn't clear the storage in memory.\n", __FUNCTION__));
				FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_PRESENT:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (requestHeader.length == sizeof(FLOCK_REQUEST_QUERY_INFO))
				{
					// Right size, it is normal to process incoming request - right body part size.

					PFLOCK_REQUEST_QUERY_INFO flockInfo = (PFLOCK_REQUEST_QUERY_INFO)requestBody;

					if (FLockStorageIsPresent(flockInfo->uniqueId))
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - requested flock is found.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS)->params.context = FLOCK_STATUS_PRESENT;
					}
					else
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: requested flock was not found.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_NOT_FOUND);
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_REQUEST_BODYSIZE, __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_SIZE);
				}

				// Free request after all actions.
				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_REMOVE:
		//////////////////////////////////////////////////////////////////////////
		if ( FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize) )
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (requestHeader.length == sizeof(FLOCK_REQUEST_QUERY_INFO))
				{
					// Right size, it is normal to process incoming request - right body part size.

					PFLOCK_REQUEST_QUERY_INFO flockInfo = (PFLOCK_REQUEST_QUERY_INFO)requestBody;

					if (FLockStorageRemove(flockInfo->uniqueId))
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - one flock was removed.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
					}
					else
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - a flock was not removed, may be it was removed earlier.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_REQUEST_BODYSIZE, __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_SIZE);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_ADD:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (requestHeader.length == sizeof(FLOCK_STORAGE_ENTRY))
				{
					// Right size, it is normal to process incoming request.
					FLOCK_STORAGE_ENTRY* fse = (FLOCK_STORAGE_ENTRY*) requestBody;
 					FLOCK_STORAGE_ENTRY flockStorageInfo = { 0 };

					//
					// Add flock only if it was not added already.
					//
					if (!FLockStorageLookup(fse->id, &flockStorageInfo))
					{
						if (FLockStorageAdd(fse->id, fse->flockFlag))
						{
							//
							// Success! Flock was added in kernel data base.
							//

							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - new flock added.\n", __FUNCTION__));

							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS);
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - driver could not add the flock.\n", __FUNCTION__));

							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_UNKNOWN_ERROR);
						}

						// Require to flush operation in next request.
						// Please, do not forget about that.
					}
					else
					{
						//
						// Return error if the flock already added.
						//

						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - the flock is already present.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ALREADY_PRESENT);
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_REQUEST_BODYSIZE, __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_SIZE);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////

	case IOCTL_FLOCK_STORAGE_QUERY_ONE:
		//////////////////////////////////////////////////////////////////////////
		if (FLockCanProcessRequest(ioBufferedInputSize, ioBufferedOutputSize))
		{
			if (FLockGetRequestAndBody(ioBufferedInputSize, ioBufferedInput, &requestHeader, &requestBody))
			{
				ioRequestStatus = STATUS_SUCCESS;
				ioWrittenBytes = sizeof(FLOCK_RESPONSE_HEADER);

				if (requestHeader.length == sizeof(FLOCK_REQUEST_QUERY_INFO))
				{
					// Yes - right size, it is normal to process incoming request.

					PFLOCK_REQUEST_QUERY_INFO flockId = (PFLOCK_REQUEST_QUERY_INFO)requestBody;
					FLOCK_STORAGE_ENTRY flockStorageInfo = { 0 };

					if (FLockStorageLookup(flockId->uniqueId, &flockStorageInfo))
					{
						// Yes, we found flock in our kernel storage.
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - flock info is found.\n", __FUNCTION__));

						if (FlockHasPlaceForResponseAndBody(ioBufferedOutputSize, sizeof(FLOCK_RESPONSE_QUERY_INFO)))
						{
							FLOCK_RESPONSE_QUERY_INFO responseBody = { 0 };
							responseBody.info = flockStorageInfo;

							FLockWriteResponseBody(FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SUCCESS), &responseBody, sizeof(responseBody));

							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: success - flock data was copied to output.\n", __FUNCTION__));
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - not enough memory to return data, require %d bytes.\n",
								__FUNCTION__,
								sizeof(FLOCK_RESPONSE_QUERY_INFO)));

							FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_SMALL_BUFFER)->params.requireLength = sizeof(FLOCK_RESPONSE_QUERY_INFO);
						}
					}
					else
					{
						PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - could not find flock info, may be it was removed earlier.\n", __FUNCTION__));
						FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_ERROR);
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_REQUEST_BODYSIZE, __FUNCTION__));
					FLockPrepareResponse(ioBufferedOutput, FLOCK_STATUS_WRONG_SIZE);
				}

				FLockFreeBody(requestBody);
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_INVALID_REQUEST, __FUNCTION__));
				ioRequestStatus = STATUS_BAD_DATA;
				ioWrittenBytes = 0;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, (MSG_WRONG_INPUT_REQUEST, __FUNCTION__));
			ioRequestStatus = STATUS_BUFFER_TOO_SMALL;
			ioWrittenBytes = 0;
		}

		break;
		//////////////////////////////////////////////////////////////////////////


	default:
		break;
	}


	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: %d bytes written to output with status 0x%x\n", __FUNCTION__, ioWrittenBytes, ioRequestStatus));

	//
	// Complete request here.
	//

	_irp->IoStatus.Information = ioWrittenBytes;
	_irp->IoStatus.Status = ioRequestStatus;
	IoCompleteRequest(_irp, IO_NO_INCREMENT);

	return dispatcherFinalStatus;
}
