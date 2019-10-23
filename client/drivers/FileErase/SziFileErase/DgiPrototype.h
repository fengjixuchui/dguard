//
//	This code belongs to Data Guard project.
//
//	burluckij@gmail.com
//

#pragma once

DRIVER_INITIALIZE	DriverEntry;
DRIVER_UNLOAD		FEraseUnloadRoutine;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH		FEraseDeviceControlRoutine;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
_Dispatch_type_(IRP_MJ_READ)
_Dispatch_type_(IRP_MJ_WRITE)
DRIVER_DISPATCH		FEraseIrpSuccessHandler;

IO_WORKITEM_ROUTINE FEraseDiskSecureWorkItem;

NTSTATUS
FEraseInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
	);

VOID
FEraseInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);

VOID
FEraseInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);

NTSTATUS
FEraseFltUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
	);

NTSTATUS
FEraseInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
FErasePreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

VOID
FEraseOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
	);

FLT_POSTOP_CALLBACK_STATUS
FErasePostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
FErasePreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

BOOLEAN
FEraseDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
	);

FLT_PREOP_CALLBACK_STATUS
FErasePreSetInfoCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
FErasePostCleanupCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
FErasePreCleanupCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
FErasePostSetInfoCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

//Context

NTSTATUS
FEraseAllocateContext(
	_Outptr_ PFLT_CONTEXT *Context
	);

NTSTATUS
FEraseSetContext(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_When_(ContextType == FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType != FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
	_In_ PFLT_CONTEXT NewContext,
	_Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext
	);

NTSTATUS
FEraseGetContext(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_When_(ContextType == FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType != FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
	_Outptr_ PFLT_CONTEXT *Context
	);

NTSTATUS
FEraseGetOrSetContext(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_When_(ContextType == FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType != FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
	_Outptr_ _Pre_valid_ PFLT_CONTEXT *Context	
	);

VOID
FEraseInstanceContextCleanup(
	_In_ PDF_INSTANCE_CONTEXT InstanceContext
	);

VOID
FEraseStreamContextCleanup(
	_In_ PDF_STREAM_CONTEXT StreamContext,
	_In_ FLT_CONTEXT_TYPE ContextType
	);

NTSTATUS
FEraseProcessDelete(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PDF_STREAM_CONTEXT    StreamContext
	);

NTSTATUS
FEraseIsFileDeleted(
	_In_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PDF_STREAM_CONTEXT StreamContext,
	_In_ BOOLEAN IsTransaction
	);

NTSTATUS
FEraseGetFileClusters(
	_In_ HANDLE fHandle,
	_Out_ PRETRIEVAL_POINTERS_BUFFER* vcnList
	);

NTSTATUS
FEraseFltGetFileClusters(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_Out_ PRETRIEVAL_POINTERS_BUFFER* VcnList
	);

NTSTATUS
FEraseFltGetFileNameInformation(
	_In_ PFLT_CALLBACK_DATA Context,
	_Inout_ PDF_STREAM_CONTEXT StreamContext
	);

NTSTATUS
FEraseFltEraseFile(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
FEraseFltEraseFileByBlocks(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
FEraseFltSecureOverwrite(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ LARGE_INTEGER offset,
	_In_ ULONG Length,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
FEraseEraseFileByBlocks(	
	_In_ PCONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
FEraseEraseFile(	
	_In_ PCONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
FEraseSecureOverwrite(
	_In_ HANDLE Handle,
	_In_ PFILE_POSITION_INFORMATION positon,
	_In_ PUCHAR Buffer,
	_In_ ULONG Length,
	_In_ UINT32 Count	
	);

NTSTATUS
FEraseSecureDeleteFile(
	_In_ PCWSTR Filename
	);

NTSTATUS
FEraseFltUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
	);

NTSTATUS
FEraseFltSecureDeleteFile(
	_In_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FileObject,
	_In_ BOOLEAN OnClose
	);

NTSTATUS
FEraseDeleteEmptyDir(
	_In_ POBJECT_ATTRIBUTES
	);

VOID
FEraseContextFree(
	_In_ PCONTEXT_ERASEFILE Context
	);
