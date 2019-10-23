#pragma once

#include <ntifs.h>

NTSTATUS
DgiDosNameToNtNameVolume(
	_In_ PUNICODE_STRING SymName,
	_In_ PUNICODE_STRING VolumeName
	);

NTSTATUS 
DgiSetFileAttribute(
	_In_ PUNICODE_STRING FileName,
	_In_ ULONG	Attribute
	);

NTSTATUS
DgiVolumeDeviceToDosName(
	_In_ PUNICODE_STRING Volume,
	_Out_ PUNICODE_STRING VolumeName
	);

//SecLookupAccountName Вместе с SID возращает мусор.
//SecLookupAccountSid Зависает если вызывать при старте системы, когда у драйвера StartType = 0
//NTSTATUS
//SziConvertSidToUserName(
//	_In_ PSID Sid,
//	_In_ PUNICODE_STRING UserName
//	);

//\??\c:\asdfsdf\sdfsdf\ffff.txt
//Path=\??\c:\asdfsdf\sdfsdf
VOID
DgiDissectPathFromFileName(
	_In_ UNICODE_STRING PathFileName,
	_Out_ PUNICODE_STRING Path
	);

//\??\c:\asdfsdf\sdfsdf\ffff.txt
//Path=\??\c:
NTSTATUS
DgiDissectDiskFromFileName(
	_In_ PUNICODE_STRING PathFileName,
	_Out_ PUNICODE_STRING Disk
	);

//\??\c:\asdf~1
BOOLEAN
DgiIsShortPath(
	_In_ PCUNICODE_STRING PathFileName
	);

NTSTATUS
DgiShortPathToLong(
	_In_ PCUNICODE_STRING ShortPathFileName,
	_Out_ PUNICODE_STRING LongPathFileName,
	_In_ ULONG Tag
	);

//MAX_LENGTH = 256
NTSTATUS
DgiCreateUnicodeStringFromString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ LPCWCH InString,
	_In_ ULONG Tag
	);

NTSTATUS
DgiCreateUnicodeStringFromANSIString(
	_Inout_ PUNICODE_STRING OutString, 
	_In_ PCHAR InString, 
	_In_ ULONG Tag
	);

NTSTATUS
DgiCreateUnicodeStringFromUnicodeString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PUNICODE_STRING InString,
	_In_ ULONG Tag
	);

NTSTATUS
DgiCreateUnicodeStringHex(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PVOID Data,
	_In_ ULONG Size,
	_In_ ULONG Tag
	);

NTSTATUS
DgiConvertSidToUnicodeString(
	_Inout_ PUNICODE_STRING OutString,
	_In_ PSID Sid,
	_In_ ULONG Tag
	);

NTSTATUS
DgiConvertUUIDToUnicodeString(
	_In_ UUID* Uid,
	_Out_ PUNICODE_STRING OutString,
	_In_ ULONG Tag
	);

BOOLEAN
DgiUUIDIsEmpty(
	_In_ UUID* Uid
	);

BOOLEAN
DgiIsValidateData(
	_In_ PVOID Address,
	_In_ ULONG Size
	);

ULONG
DgiGetDeviceTypeToUse(
	_In_ PDEVICE_OBJECT Pdo
	);

NTSTATUS
DgiGetDeviceProperty(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
	_Out_ PUNICODE_STRING Propery,
	_In_ ULONG Tag
);

NTSTATUS
DgiGetDevicePropertyValue(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
	_Out_ PVOID Propery,
	_In_ ULONG ProperySize
);

//Object = PDO Type = BusQueryDeviceSerialNumber
NTSTATUS
DgiSend_IRP_MJ_PNP(
	_In_ PDEVICE_OBJECT Object,
	_In_ BUS_QUERY_ID_TYPE Type,
	_Out_ PUNICODE_STRING String,
	_In_ ULONG Tag
	);

//////////////////////////////////////////////////////////////////////////

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
DgiAcquireResourceExclusive(
	_Inout_ _Acquires_exclusive_lock_(*Resource) PERESOURCE Resource
	) 
{
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) || !ExIsResourceAcquiredSharedLite(Resource));

	KeEnterCriticalRegion();
	(VOID)ExAcquireResourceExclusiveLite(Resource, TRUE);
}

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
DgiAcquireResourceShared(
	_Inout_ _Acquires_shared_lock_(*Resource) PERESOURCE Resource
	) 
{
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	KeEnterCriticalRegion();
	(VOID)ExAcquireResourceSharedLite(Resource, TRUE);
}

FORCEINLINE
VOID
_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(_Global_critical_region_)
DgiReleaseResource(
	_Inout_ _Requires_lock_held_(*Resource) _Releases_lock_(*Resource) PERESOURCE Resource
	) 
{
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) || ExIsResourceAcquiredSharedLite(Resource));

	ExReleaseResourceLite(Resource);
	KeLeaveCriticalRegion();
}

//////////////////////////////////////////////////////////////////////////
