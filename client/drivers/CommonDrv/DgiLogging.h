#pragma once

#include <ntifs.h>

typedef struct _SZI_LOGGING {
	HANDLE		File;
	PERESOURCE	LockWrite;
} SZI_LOGGING, *PSZI_LOGGING;

NTSTATUS
CommonInitLog(
	_In_ PUNICODE_STRING FileName,
	_Out_ PSZI_LOGGING* Log
);

VOID
CommonFreeLog(
	_In_ PSZI_LOGGING Log
);

NTSTATUS
CommonLogWrite(
	_In_ PSZI_LOGGING Log,
	_In_ LPCTSTR Message,
	_In_ USHORT Length
	);

NTSTATUS
CommonLogWriteU(
	_In_ PSZI_LOGGING Log,
	_In_ PUNICODE_STRING Message
	);

NTSTATUS
CommonLogWriteV(
	_In_ PSZI_LOGGING Log,
	_In_ LPCTSTR Format,
	_In_ ...
	);