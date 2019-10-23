#pragma once

#include <ntifs.h>

NTSTATUS
CommonRegGetValueString(
	_In_ PUNICODE_STRING RegistryPath,
	_In_ PUNICODE_STRING Key,
	_Out_ PUNICODE_STRING Value,
	_In_ ULONG	Tag
	);

NTSTATUS
CommonRegGetValueULong(
	_In_ PUNICODE_STRING RegistryPath,
	_In_ PUNICODE_STRING Key,
	_Out_ ULONG* Value
	);

NTSTATUS
CommonRegGetValueLong(
	_In_ PUNICODE_STRING RegistryPath,
	_In_ PUNICODE_STRING Key,
	_Out_ LONG* Value
	);