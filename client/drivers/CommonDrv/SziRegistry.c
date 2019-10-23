#include "SziRegistry.h"
#include "SziTools.h"

#define MAX_REGSTRING		(255 * sizeof(WCHAR))

NTSTATUS
CommonRegGetValueString(
	_In_ PUNICODE_STRING RegistryPath,
	_In_ PUNICODE_STRING KeyName,
	_Out_ PUNICODE_STRING Value,
	_In_ ULONG	Tag
	)
{
	HANDLE				regKey = NULL;
	OBJECT_ATTRIBUTES	attributes;
	UCHAR				buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_REGSTRING] = "\0";
	ULONG				valueLength = sizeof(buffer);	
	PKEY_VALUE_PARTIAL_INFORMATION valueReg = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
	ULONG				resultLength = 0;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	
	RtlInitEmptyUnicodeString(Value, NULL, 0);
	InitializeObjectAttributes(&attributes, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&regKey, KEY_READ, &attributes);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = ZwQueryValueKey(regKey, KeyName, KeyValuePartialInformation, valueReg, valueLength, &resultLength);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	
	if(REG_SZ == valueReg->Type) {
		__try {
			status = SziCreateUnicodeStringFromString(Value, (WCHAR*)valueReg->Data, Tag);
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			status = STATUS_UNSUCCESSFUL;
		}
	} else {
		status = STATUS_INVALID_PARAMETER;
	}

_error:
	if (regKey)	{
		ZwClose(regKey);
	}
	
	return status;
}

NTSTATUS
CommonRegGetValueULong(
	_In_ PUNICODE_STRING RegistryPath,
	_In_ PUNICODE_STRING KeyName,
	_Out_ ULONG* Value
	)
{
	HANDLE				regKey = NULL;
	OBJECT_ATTRIBUTES	attributes;
	UCHAR				buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_REGSTRING] = "\0";
	ULONG				valueLength = sizeof(buffer);	
	ULONG				resultLength = 0;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;

	__try {
		InitializeObjectAttributes(&attributes, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&regKey, KEY_READ, &attributes);
		if(NT_SUCCESS(status)) {
			PKEY_VALUE_PARTIAL_INFORMATION valueReg = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

			status = ZwQueryValueKey(regKey, KeyName, KeyValuePartialInformation, valueReg, valueLength, &resultLength);
			if(NT_SUCCESS(status)) {
				if(REG_DWORD == valueReg->Type) {
					*Value = *(ULONG*)valueReg->Data;
				} else {
					status = STATUS_UNSUCCESSFUL;
				}
			}
		}
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		status = STATUS_UNSUCCESSFUL;
	}

	if(regKey != NULL) {
		ZwClose(regKey);
	}

	return status;
}

NTSTATUS
CommonRegGetValueLong(
	_In_ PUNICODE_STRING RegistryPath,
	_In_ PUNICODE_STRING KeyName,
	_Out_ LONG* Value
	) 
{
	HANDLE				regKey = NULL;
	OBJECT_ATTRIBUTES	attributes;
	UCHAR				buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_REGSTRING] = "\0";
	ULONG				valueLength = sizeof(buffer);
	ULONG				resultLength = 0;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;

	__try {
		InitializeObjectAttributes(&attributes, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&regKey, KEY_READ, &attributes);
		if(NT_SUCCESS(status)) {
			PKEY_VALUE_PARTIAL_INFORMATION valueReg = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

			status = ZwQueryValueKey(regKey, KeyName, KeyValuePartialInformation, valueReg, valueLength, &resultLength);
			if(NT_SUCCESS(status)) {
				if(REG_DWORD == valueReg->Type) {
					*Value = *(LONG*)valueReg->Data;
				} else {
					status = STATUS_UNSUCCESSFUL;
				}
			}
		}
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		status = STATUS_UNSUCCESSFUL;
	}

	if(regKey != NULL) {
		ZwClose(regKey);
	}

	return status;
}