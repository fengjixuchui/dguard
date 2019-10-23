#include "DgiTools.h"

#define NTSTRSAFE_NO_CB_FUNCTIONS
#include <Ntstrsafe.h>

#define MAX_STRING		(262 * sizeof(WCHAR))
#define MAX_LENGTH		(MAX_STRING / sizeof(WCHAR))
#define Add2Ptr(P,I)	((PVOID)((PUCHAR)(P) + (I)))

#define CF_SEARCHBUFFER_TAG					'bEzS'		//SzEl

NTSTATUS DgiDosNameToNtNameVolume(
	_In_ PUNICODE_STRING SymName,
	_In_ PUNICODE_STRING VolumeName
	) 
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	HANDLE		LinkHandle = NULL;
	OBJECT_ATTRIBUTES object_attributes;
	ULONG		rt = 0;

	InitializeObjectAttributes(&object_attributes, SymName, OBJ_KERNEL_HANDLE, NULL, NULL);

	status = ZwOpenSymbolicLinkObject(&LinkHandle, GENERIC_READ, &object_attributes);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	status = ZwQuerySymbolicLinkObject(LinkHandle, VolumeName, &rt);
_error:
	if(LinkHandle) {
		ZwClose(LinkHandle);
	}
	return status;
}

NTSTATUS
DgiVolumeDeviceToDosName(
	_In_ PUNICODE_STRING Volume,
	_Out_ PUNICODE_STRING VolumeName
	) 
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				volume = 0;
	IO_STATUS_BLOCK		ioCreateFile;
	OBJECT_ATTRIBUTES	volAttr;
	PFILE_OBJECT		file = NULL;

	InitializeObjectAttributes(&volAttr, Volume, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	status = ZwOpenFile(&volume, GENERIC_WRITE, &volAttr, &ioCreateFile, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = ObReferenceObjectByHandle(volume, 0, 0, KernelMode, &file, NULL);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = IoVolumeDeviceToDosName(file->DeviceObject, VolumeName);

_error:
	if(volume) {
		ZwClose(volume);
	}
	if(file != NULL) {
		ObDereferenceObject(file);
	}

	return status;
}

NTSTATUS DgiSetFileAttribute(
	_In_ PUNICODE_STRING FileName,
	_In_ ULONG	Attribute
	)
{
	FILE_BASIC_INFORMATION	basicInfoFile;
	HANDLE					hFile = 0;
	IO_STATUS_BLOCK			ioStatus;
	NTSTATUS				status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES		objAttr;

	RtlZeroMemory(&basicInfoFile, sizeof(basicInfoFile));

	basicInfoFile.FileAttributes = Attribute;

	InitializeObjectAttributes(&objAttr, FileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&hFile, FILE_WRITE_ATTRIBUTES, &objAttr, &ioStatus, NULL, 0, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if(NT_SUCCESS(status)) {
		ZwSetInformationFile(hFile, &ioStatus, &basicInfoFile, sizeof(basicInfoFile), FileBasicInformation);
		ZwClose(hFile);
	}

	return status;
}

//NTSTATUS
//SziConvertSidToUserName(
//	_In_ PSID Sid,
//	_In_ PUNICODE_STRING UserName
//	)
//{
//	NTSTATUS		status = STATUS_UNSUCCESSFUL;
//	ULONG			nameSize = 0;	
//	ULONG			domainSize = 0;
//	SID_NAME_USE	puse = 0;
//
//	status = SecLookupAccountSid(Sid, &nameSize, UserName, &domainSize, NULL, &puse);
//	return status;
//}

#pragma warning(disable: 4130)
VOID
DgiDissectPathFromFileName(
	_In_ UNICODE_STRING PathFileName, 
	_Out_ PUNICODE_STRING Path
	)
{
	USHORT newLength = PathFileName.Length;
	for (USHORT index = (PathFileName.Length - 1) / sizeof(WCHAR); index > 0; index--) {
		if (0 != PathFileName.Buffer[index]) {
			newLength = newLength - sizeof(WCHAR);
		}
		if (L'\\' == (WCHAR)PathFileName.Buffer[index]) {
			Path->Buffer = PathFileName.Buffer;
			Path->MaximumLength = Path->Length = newLength;
			break;
		}
	}
}
#pragma warning(default: 4130)

#pragma warning(disable: 4130)
NTSTATUS
DgiDissectDiskFromFileName(
	_In_ PUNICODE_STRING PathFileName,
	_Out_ PUNICODE_STRING Disk
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	for(USHORT index = 0; index < PathFileName->Length / sizeof(WCHAR); index++) {
		if(L':' == (WCHAR)PathFileName->Buffer[index]) {
			Disk->Buffer = PathFileName->Buffer;
			Disk->MaximumLength = Disk->Length = (index * sizeof(WCHAR)) + sizeof(WCHAR);
			status = STATUS_SUCCESS;
			break;
		}
	}
	return status;
}

BOOLEAN
DgiIsShortPath(
	_In_ PCUNICODE_STRING PathFileName
	) 
{
	BOOLEAN isShort = FALSE;
	for(USHORT index = 0; index < PathFileName->Length / sizeof(WCHAR); index++) {
		if(L'~' == (WCHAR)PathFileName->Buffer[index]) {
			isShort = TRUE;
			break;
		}
	}
	return isShort;
}

#pragma warning(disable: 4127)
NTSTATUS
DgiShortPathToLong(
	_In_ PCUNICODE_STRING ShortPathFileName,
	_Out_ PUNICODE_STRING LongPathFileName,
	_In_ ULONG Tag
	)
{
	NTSTATUS		status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING	dirSearch;
	UNICODE_STRING	startSearch;
	USHORT			lastIndexDir = 0;
	
	RtlInitEmptyUnicodeString(LongPathFileName, NULL, 0);

	LongPathFileName->Buffer = ExAllocatePoolWithTag(PagedPool, MAX_STRING, Tag);
	LongPathFileName->Length = 0;
	LongPathFileName->MaximumLength = MAX_STRING;

	if (LongPathFileName->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(LongPathFileName->Buffer, MAX_STRING);

	for (USHORT index = 0; index < ShortPathFileName->Length / sizeof(WCHAR); index++) {
		if (L':' == (WCHAR)ShortPathFileName->Buffer[index]) {
			lastIndexDir = index + 2;
			startSearch.Buffer = ShortPathFileName->Buffer;
			startSearch.MaximumLength = startSearch.Length = (lastIndexDir * sizeof(WCHAR));
			status = STATUS_SUCCESS;
			break;
		}
	}

	if (!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = RtlAppendUnicodeStringToString(LongPathFileName, &startSearch);
	if (!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	do {
		IO_STATUS_BLOCK			ioStatusBlock;
		OBJECT_ATTRIBUTES		objAttr;
		HANDLE					hFile = NULL;

		status = STATUS_UNSUCCESSFUL;

		for (USHORT index = lastIndexDir; index < (ShortPathFileName->Length / sizeof(WCHAR)); index++) {
			if (L'\\' == (WCHAR)ShortPathFileName->Buffer[index]) {
				dirSearch.Buffer = Add2Ptr(ShortPathFileName->Buffer, lastIndexDir * sizeof(WCHAR));
				dirSearch.MaximumLength = dirSearch.Length = (index - lastIndexDir) * sizeof(WCHAR);
				lastIndexDir = index + 1;
				status = STATUS_SUCCESS;
				break;
			}
		}
		//Выход из цикла когда найден последний каталог
		if (!NT_SUCCESS(status)) {
			if (lastIndexDir < (ShortPathFileName->Length / sizeof(WCHAR))) {
				status = RtlUnicodeStringCchCatStringN(LongPathFileName, Add2Ptr(ShortPathFileName->Buffer, lastIndexDir * sizeof(WCHAR)), (ShortPathFileName->Length / sizeof(WCHAR)) - lastIndexDir);
				if (!NT_SUCCESS(status)) {
					DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
					break;
				}
			}
			break;
		}
		
		InitializeObjectAttributes(&objAttr, &startSearch, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenFile(&hFile, FILE_LIST_DIRECTORY | SYNCHRONIZE,
							&objAttr, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
		if (!NT_SUCCESS(status)) {
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
			break;
		}

		PFILE_DIRECTORY_INFORMATION dirInfo = ExAllocatePoolWithTag(PagedPool, sizeof(FILE_DIRECTORY_INFORMATION) + MAX_LENGTH, CF_SEARCHBUFFER_TAG);			
		if (dirInfo == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
			break;
		}
		RtlZeroMemory(dirInfo, sizeof(FILE_DIRECTORY_INFORMATION) + MAX_LENGTH);

		status = ZwQueryDirectoryFile(hFile, NULL, NULL, NULL, &ioStatusBlock, dirInfo, sizeof(FILE_DIRECTORY_INFORMATION) + MAX_LENGTH, FileDirectoryInformation, TRUE, &dirSearch, TRUE);
		if (NT_SUCCESS(status)) {
			status = RtlAppendUnicodeToString(LongPathFileName, dirInfo->FileName);
			if (!NT_SUCCESS(status)) {
				DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
				break;
			}
			status = RtlAppendUnicodeToString(LongPathFileName, L"\\");
			if (!NT_SUCCESS(status)) {
				DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
				break;
			}
		}
		ExFreePoolWithTag(dirInfo, CF_SEARCHBUFFER_TAG);
		ZwClose(hFile);

		if (!NT_SUCCESS(status)) {
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
			break;
		}

		startSearch.MaximumLength = startSearch.Length = startSearch.Length + dirSearch.Length;
	} while (TRUE);

_error:
	if (!NT_SUCCESS(status) && LongPathFileName->Buffer != NULL) {
		ExFreePoolWithTag(LongPathFileName->Buffer, Tag);
	}
	return status;
}
#pragma warning(default: 4127)
#pragma warning(default: 4130)

NTSTATUS
DgiCreateUnicodeStringFromString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ LPCWCH InString,
	_In_ ULONG Tag
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	size_t		strLength = 0;

	if (InString == NULL) {
		status = STATUS_INVALID_ADDRESS;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	status = RtlStringCchLengthW(InString, MAX_LENGTH, &strLength);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	if (strLength == 0)	{
		status = STATUS_SUCCESS;
		goto _error;
	}
	strLength = strLength * sizeof(WCHAR);
	OutString->Buffer = ExAllocatePoolWithTag(PagedPool, strLength, Tag);
	OutString->Length = 0;
	OutString->MaximumLength = (USHORT)strLength;
	if(OutString->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = RtlUnicodeStringCchCopyStringN(OutString, InString, strLength);
_error:
	if(!NT_SUCCESS(status) && OutString->Buffer != NULL) {
		ExFreePoolWithTag(OutString->Buffer, Tag);
		RtlInitEmptyUnicodeString(OutString, NULL, 0);
	}
	return status;
}


NTSTATUS
DgiCreateUnicodeStringFromANSIString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PCHAR InString,
	_In_ ULONG Tag
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	ANSI_STRING aString;

	if(InString == NULL) {
		status = STATUS_INVALID_ADDRESS;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = RtlInitAnsiStringEx(&aString, InString);
	if(!NT_SUCCESS(status))	{
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	if (aString.Length == 0) {
		status = STATUS_UNSUCCESSFUL;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	OutString->Buffer = ExAllocatePoolWithTag(PagedPool, (aString.MaximumLength + 1) * sizeof(WCHAR), Tag);
	OutString->Length = 0;
	OutString->MaximumLength = (aString.MaximumLength + 1) * sizeof(WCHAR);
	if(OutString->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	status = RtlAnsiStringToUnicodeString(OutString, &aString, FALSE);
_error:
	if(!NT_SUCCESS(status) && OutString->Buffer != NULL) {
		ExFreePoolWithTag(OutString->Buffer, Tag);
		RtlInitEmptyUnicodeString(OutString, NULL, 0);
	}
	return status;
}

NTSTATUS
DgiCreateUnicodeStringFromUnicodeString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PUNICODE_STRING InString,
	_In_ ULONG Tag
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	RtlInitEmptyUnicodeString(OutString, NULL, 0);
	if (InString->Length == 0) {
		status = STATUS_SUCCESS;
		goto _error;
	}

	OutString->Buffer = ExAllocatePoolWithTag(PagedPool, InString->Length, Tag);
	OutString->Length = 0;
	OutString->MaximumLength = InString->Length;
	if(OutString->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = RtlUnicodeStringCopy(OutString, InString);
_error:
	if (!NT_SUCCESS(status) && OutString->Buffer != NULL) {
		ExFreePoolWithTag(OutString->Buffer, Tag);
		RtlInitEmptyUnicodeString(OutString, NULL, 0);
	}
	return status;
}

NTSTATUS
DgiCreateUnicodeStringHex(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PVOID Data,
	_In_ ULONG Size,
	_In_ ULONG Tag
	)
{
	UCHAR		hexChars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	CHAR		buffer[255] = "\0";
	USHORT		bufferLength = 0;

	__try {
		for(ULONG i = 0; i < Size; ++i) {
			UCHAR byte = *(PUCHAR)Add2Ptr(Data, i);
			buffer[bufferLength++] = hexChars[(byte & 0xF0) >> 4];
			buffer[bufferLength++] = hexChars[(byte & 0x0F) >> 0];
		}
		if (Size) {
			status = DgiCreateUnicodeStringFromANSIString(OutString, buffer, Tag);
		}
	}__except(1)
	{
		status = STATUS_UNSUCCESSFUL;
	}
	return status;
}

NTSTATUS
DgiConvertSidToUnicodeString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PSID Sid,
	_In_ ULONG Tag
	) 
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	OutString->Buffer = ExAllocatePoolWithTag(PagedPool, SECURITY_MAX_SID_SIZE * sizeof(WCHAR), Tag);
	OutString->Length = 0;
	OutString->MaximumLength = SECURITY_MAX_SID_SIZE * sizeof(WCHAR);

	if(OutString->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = RtlConvertSidToUnicodeString(OutString, Sid, FALSE);

_error:
	if(!NT_SUCCESS(status)) {
		if (OutString->Buffer != NULL) {
			ExFreePoolWithTag(OutString->Buffer, Tag);
		}
		RtlInitEmptyUnicodeString(OutString, NULL, 0);
	}
	return status;
}

NTSTATUS
DgiConvertUUIDToUnicodeString(
	_In_ UUID* Uid,
	_Out_ PUNICODE_STRING OutString,
	_In_ ULONG Tag
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	WCHAR		buffer[128] = L"\0";
	size_t		length = 0;

	RtlInitEmptyUnicodeString(OutString, NULL, 0);
	status = RtlStringCchPrintfW(buffer, sizeof(buffer) / sizeof(WCHAR), L"%x-%x-%x-%x%x%x%x%x%x%x%x", Uid->Data1, Uid->Data2, Uid->Data3, Uid->Data4[0], Uid->Data4[1], Uid->Data4[2], Uid->Data4[3], 
																															Uid->Data4[4],Uid->Data4[5],Uid->Data4[6],Uid->Data4[7]);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	status = RtlStringCchLengthW(buffer, sizeof(buffer), &length);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	OutString->Buffer = ExAllocatePoolWithTag(PagedPool, length * sizeof(WCHAR), Tag);
	OutString->Length = 0;
	OutString->MaximumLength = (USHORT)length * sizeof(WCHAR);
	if(OutString->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	status = RtlUnicodeStringCchCopyStringN(OutString, buffer, length);

_error:
	if (!NT_SUCCESS(status) && OutString->Buffer != NULL) {
		ExFreePoolWithTag(OutString->Buffer, Tag);
	}
	return status;
}

BOOLEAN
DgiUUIDIsEmpty(
	_In_ UUID* Uid
	)
{	
	if (Uid != NULL) {
		return Uid->Data1 == 0 && Uid->Data2 == 0 && Uid->Data3 == 0 && Uid->Data4[0] == 0 && Uid->Data4[1] == 0 && Uid->Data4[2] == 0  && Uid->Data4[3] == 0
																		 && Uid->Data4[4] == 0 && Uid->Data4[5] == 0 && Uid->Data4[6] == 0 && Uid->Data4[7] == 0;
	}
	return TRUE;
}

BOOLEAN
DgiIsValidateData(
	_In_ PVOID Address,
	_In_ ULONG Size
	)
{
	BOOLEAN ret = TRUE;

	if(Address == NULL || Size <= 0) {
		return FALSE;
	}
	
	if(Address > MM_HIGHEST_USER_ADDRESS) {
		// мы имеем дело с kernel mode-адресом
		for(ULONG i = 0; i < Size; i++) {
			if(!MmIsAddressValid((PUCHAR)Address + i)) {
				ret = FALSE;
				break;
			}
		}
	} else {
		// это user mode-адрес
		__try {
			ProbeForRead(Address, Size, 1);
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			// ProbeForRead вызвала исключение
			ret = FALSE;
		}
	}

	return ret;
}

ULONG 
DgiGetDeviceTypeToUse(
	_In_ PDEVICE_OBJECT Pdo
	) 
{
	PDEVICE_OBJECT	ldo = IoGetAttachedDeviceReference(Pdo);
	ULONG			devType = FILE_DEVICE_UNKNOWN;

	if(ldo != NULL) {
		devType = ldo->DeviceType;
		ObDereferenceObject(ldo);
	}
	
	return devType;
}

NTSTATUS
DgiGetDeviceProperty(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
	_Out_ PUNICODE_STRING Propery,
	_In_ ULONG Tag
	) 
{
	ULONG		devDesclen = 0;
	NTSTATUS	status = STATUS_UNSUCCESSFUL;

	RtlInitEmptyUnicodeString(Propery, NULL, 0);
	Propery->Buffer = ExAllocatePoolWithTag(PagedPool, MAX_STRING, Tag);
	Propery->Length = 0;
	Propery->MaximumLength = MAX_STRING;

	if(Propery->Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(Propery->Buffer, MAX_STRING);
	status = IoGetDeviceProperty(DeviceObject, DeviceProperty, Propery->MaximumLength, Propery->Buffer, &devDesclen);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	Propery->Length = (USHORT)devDesclen;
	//if (Propery->MaximumLength >= (USHORT)devDesclen + 1) {
	//	Propery->Length = (USHORT)devDesclen + 1;
	//	Propery->Buffer[Propery->Length] = L'\0';
	//} else {
	//	Propery->Length = (USHORT)devDesclen;
	//}
	
_error:
	if (!NT_SUCCESS(status)) {
		if(Propery->Buffer != NULL) {
			ExFreePoolWithTag(Propery->Buffer, Tag);
		}
		RtlInitEmptyUnicodeString(Propery, NULL, 0);
	}
	return status;
}

NTSTATUS
DgiGetDevicePropertyValue(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
	_Out_ PVOID Propery,
	_In_ ULONG ProperySize
	)
{
	ULONG		devDesclen = 0;
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	
	RtlZeroMemory(Propery, ProperySize);
	status = IoGetDeviceProperty(DeviceObject, DeviceProperty, ProperySize, Propery, &devDesclen);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
	}
	return status;
}

NTSTATUS
DgiSend_IRP_MJ_PNP(
	_In_ PDEVICE_OBJECT Object,
	_In_ BUS_QUERY_ID_TYPE Type,
	_Out_ PUNICODE_STRING String,
	_In_ ULONG Tag
	)
{
	KEVENT				kEvent;
	IO_STATUS_BLOCK		ioStatus;
	PIRP				irp = NULL;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION  stack;

	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);
	
	irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, Object, NULL, 0, NULL, &kEvent, &ioStatus);
	if (irp == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}

	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	stack = IoGetNextIrpStackLocation(irp);
	stack->MinorFunction = IRP_MN_QUERY_ID;
	stack->Parameters.QueryId.IdType = Type;

	status = IoCallDriver(Object, irp);
	if(status == STATUS_PENDING) {
		KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, NULL);
	}
	
	if(NT_SUCCESS(status)) {
		WCHAR *instanceId = (WCHAR*)ioStatus.Information;
		status = DgiCreateUnicodeStringFromString(String, instanceId, Tag);
		//DbgPrint("InstanceId = %ws\n", pInstanceId); // tested.
		ExFreePool(instanceId); // IRP_MN_QUERY_ID require this
	}

_error:

	return status;
}