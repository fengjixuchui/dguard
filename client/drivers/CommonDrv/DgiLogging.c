#include "DgiLogging.h"
#include "DgiTools.h"

//#define NTSTRSAFE_MAX_CCH					256
//#define NTSTRSAFE_UNICODE_STRING_MAX_CCH	(256 * sizeof(WCHAR))
#define MAX_MESSAGE							(262 * sizeof(WCHAR))

#define NTSTRSAFE_NO_CB_FUNCTIONS
#include <Ntstrsafe.h>

#define CF_LOGGING_POOL_TAG			'fLfS'			//SfLf
#define CF_RESOURCE_TAG				'rLfS'			//SfLr

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CommonInitLog)
#pragma alloc_text(PAGE, CommonFreeLog)
#pragma alloc_text(PAGE, CommonLogWrite)
#endif

NTSTATUS
CommonInitLog(
	_In_ PUNICODE_STRING FileName,
	_Out_ PSZI_LOGGING* Log
	)
{
	PAGED_CODE();

	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK				ioCreateFile;
	OBJECT_ATTRIBUTES			objAttr;
	FILE_STANDARD_INFORMATION	fileInfo;
	FILE_POSITION_INFORMATION	filePos;
	PSZI_LOGGING				loggin = NULL;
	BOOLEAN						logResInit = FALSE;

	loggin = ExAllocatePoolWithTag(PagedPool, sizeof(SZI_LOGGING), CF_LOGGING_POOL_TAG);
	if (loggin == NULL)	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}

	RtlZeroMemory(loggin, sizeof(SZI_LOGGING));

	loggin->LockWrite = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ERESOURCE), CF_RESOURCE_TAG);
	if(loggin->LockWrite == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}

	status = ExInitializeResourceLite(loggin->LockWrite);
	if(!NT_SUCCESS(status)) {
		goto _error;
	}
	logResInit = TRUE;

	InitializeObjectAttributes(&objAttr, FileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&loggin->File, GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE, &objAttr, &ioCreateFile, NULL, 0, 
						  FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN | FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if(!NT_SUCCESS(status)) {
		loggin->File = NULL;
		goto _error;
	}

	status = ZwQueryInformationFile(loggin->File, &ioCreateFile, &fileInfo, sizeof(fileInfo), FileStandardInformation);
	if(NT_SUCCESS(status)) {
		filePos.CurrentByteOffset = fileInfo.EndOfFile;
		status = ZwSetInformationFile(loggin->File, &ioCreateFile, &filePos, sizeof(filePos), FilePositionInformation);
		if(NT_SUCCESS(status)) {
			*Log = loggin;
		}
	}

_error:
	if (!NT_SUCCESS(status) && loggin != NULL) {
		if(loggin->File != NULL) {
			ZwClose(loggin->File);
		}
		if(logResInit) {
			ExDeleteResourceLite(loggin->LockWrite);
		}
		if(loggin->LockWrite != NULL) {
			ExFreePoolWithTag(loggin->LockWrite, CF_RESOURCE_TAG);
		}
		ExFreePoolWithTag(loggin, CF_LOGGING_POOL_TAG);
	}
	
	return status;
}

VOID
CommonFreeLog(
	_In_ PSZI_LOGGING Log
	)
{
	PAGED_CODE();
	
	if (Log != NULL) {
		ZwClose(Log->File);
		ExDeleteResourceLite(Log->LockWrite);
		ExFreePoolWithTag(Log->LockWrite, CF_RESOURCE_TAG);
		ExFreePoolWithTag(Log, CF_LOGGING_POOL_TAG);
	}
}

NTSTATUS
CommonLogWrite(
	_In_ PSZI_LOGGING Log,
	_In_ LPCTSTR Message,
	_In_ USHORT MessageLength
	)
{
	IO_STATUS_BLOCK		ioWriteFile;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	size_t				length = 0;
	LARGE_INTEGER		sysTime;
	LARGE_INTEGER		localTime;
	TIME_FIELDS			time;
	CHAR				timeBuffer[30] = "\0";

	PAGED_CODE();

	if (Log == NULL) {
		DbgPrint("CommonLog:%s\n", Message);
		return status;
	}
	
	KeQuerySystemTime(&sysTime);
	ExSystemTimeToLocalTime(&sysTime, &localTime);
	RtlTimeToTimeFields(&localTime, &time);
	
	status = RtlStringCchPrintfA(timeBuffer, sizeof(timeBuffer), "%02d.%02d.%d %02d:%02d:%02d:%03d ", time.Day, time.Month, time.Year, time.Hour, time.Minute, time.Second, time.Milliseconds);
	if(!NT_SUCCESS(status)) {
		goto _error;
	}

	status = RtlStringCchLengthA(timeBuffer, sizeof(timeBuffer), &length);
	if(!NT_SUCCESS(status)) {
		goto _error;
	}

	DgiAcquireResourceExclusive(Log->LockWrite);
	status = ZwWriteFile(Log->File, NULL, NULL, NULL, &ioWriteFile, (PVOID)timeBuffer, (ULONG)length, NULL, NULL);
	if(!NT_SUCCESS(status)) {
		goto _error;
	}
	
	status = ZwWriteFile(Log->File, NULL, NULL, NULL, &ioWriteFile, (PVOID)Message, (ULONG)MessageLength, NULL, NULL);
	if(!NT_SUCCESS(status)) {
		goto _error;
	}
	status = ZwWriteFile(Log->File, NULL, NULL, NULL, &ioWriteFile, "\n", 1, NULL, NULL);
	if (NT_SUCCESS(status))	{
		ZwFlushBuffersFile(Log->File, &ioWriteFile);
	}
	DgiReleaseResource(Log->LockWrite);
_error:
	if (!NT_SUCCESS(status)) {
		DbgPrint("CommonLog: %s:%d 0x%X\n", __FILE__, __LINE__, status);
	}
	return status;
}

NTSTATUS
CommonLogWriteU(
	_In_ PSZI_LOGGING Log,
	_In_ PUNICODE_STRING Message
	)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	ANSI_STRING			bufMessage;

	status = RtlUnicodeStringToAnsiString(&bufMessage, Message, TRUE);
	if (NT_SUCCESS(status)) {
		status = CommonLogWrite(Log, bufMessage.Buffer, bufMessage.Length);
		RtlFreeAnsiString(&bufMessage);
	} else {
		DbgPrint("CommonLog: %s:%d 0x%X\n", __FILE__, __LINE__, status);
	}
	return status;
}

NTSTATUS
CommonLogWriteV(
	_In_ PSZI_LOGGING Log,
	_In_ LPCTSTR Format,
	_In_ ...
	)
{
	NTSTATUS		status = STATUS_UNSUCCESSFUL;
	CHAR			bufMessage[MAX_MESSAGE] = "\0";
	size_t			length = 0;

	va_list args;
	va_start(args, Format);
	status = RtlStringCchVPrintfA(bufMessage, sizeof(bufMessage) - sizeof(WCHAR), Format, args);
	if (NT_SUCCESS(status))	{
		status = RtlStringCchLengthA(bufMessage, MAX_MESSAGE, &length);
		if(NT_SUCCESS(status)) {
			status = CommonLogWrite(Log, bufMessage, (USHORT)length);
		} else {
			DbgPrint("CommonLog: %s:%d 0x%X\n", __FILE__, __LINE__, status);
		}
	} else {
		DbgPrint("CommonLog: %s:%d 0x%X\n", __FILE__, __LINE__, status);
	}
	va_end(args);
	return status;
}
