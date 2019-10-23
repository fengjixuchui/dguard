#pragma once

DRIVER_INITIALIZE	DriverEntry;
DRIVER_UNLOAD		SziFileEraseUnloadRoutine;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH		SziFileEraseDeviceControlRoutine;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
_Dispatch_type_(IRP_MJ_READ)
_Dispatch_type_(IRP_MJ_WRITE)
DRIVER_DISPATCH		SziFileEraseIrpSuccessHandler;

IO_WORKITEM_ROUTINE SziFileEraseDiskSecureWorkItem;

NTSTATUS
SziFileEraseInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
	);

VOID
SziFileEraseInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);

VOID
SziFileEraseInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);

NTSTATUS
SziFileEraseFltUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
	);

NTSTATUS
SziFileEraseInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
SziFileErasePreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

VOID
SziFileEraseOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
	);

FLT_POSTOP_CALLBACK_STATUS
SziFileErasePostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
SziFileErasePreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

BOOLEAN
SziFileEraseDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
	);

FLT_PREOP_CALLBACK_STATUS
SziFileErasePreSetInfoCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
SziFileErasePostCleanupCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
SziFileErasePreCleanupCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
SziFileErasePostSetInfoCallback(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

//Context

NTSTATUS
SziFileEraseAllocateContext(
	_Outptr_ PFLT_CONTEXT *Context
	);

NTSTATUS
SziFileEraseSetContext(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_When_(ContextType == FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType != FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
	_In_ PFLT_CONTEXT NewContext,
	_Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext
	);

NTSTATUS
SziFileEraseGetContext(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_When_(ContextType == FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType != FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
	_Outptr_ PFLT_CONTEXT *Context
	);

NTSTATUS
SziFileEraseGetOrSetContext(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_When_(ContextType == FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType != FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
	_Outptr_ _Pre_valid_ PFLT_CONTEXT *Context	
	);

VOID
SziFileEraseInstanceContextCleanup(
	_In_ PDF_INSTANCE_CONTEXT InstanceContext
	);

VOID
SziFileEraseStreamContextCleanup(
	_In_ PDF_STREAM_CONTEXT StreamContext,
	_In_ FLT_CONTEXT_TYPE ContextType
	);

NTSTATUS
SziFileEraseProcessDelete(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PDF_STREAM_CONTEXT    StreamContext
	);

NTSTATUS
SziFileEraseIsFileDeleted(
	_In_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PDF_STREAM_CONTEXT StreamContext,
	_In_ BOOLEAN IsTransaction
	);

NTSTATUS
SziFileEraseGetFileClusters(
	_In_ HANDLE fHandle,
	_Out_ PRETRIEVAL_POINTERS_BUFFER* vcnList
	);

NTSTATUS
SziFileEraseFltGetFileClusters(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_Out_ PRETRIEVAL_POINTERS_BUFFER* VcnList
	);

NTSTATUS
SziFileEraseFltGetFileNameInformation(
	_In_ PFLT_CALLBACK_DATA Context,
	_Inout_ PDF_STREAM_CONTEXT StreamContext
	);

NTSTATUS
SziFileEraseFltEraseFile(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
SziFileEraseFltEraseFileByBlocks(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
SziFileEraseFltSecureOverwrite(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ LARGE_INTEGER offset,
	_In_ ULONG Length,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
SziFileEraseEraseFileByBlocks(	
	_In_ PCONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
SziFileEraseEraseFile(	
	_In_ PCONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	);

NTSTATUS
SziFileEraseSecureOverwrite(
	_In_ HANDLE Handle,
	_In_ PFILE_POSITION_INFORMATION positon,
	_In_ PUCHAR Buffer,
	_In_ ULONG Length,
	_In_ UINT32 Count	
	);

NTSTATUS
SziFileEraseSecureDeleteFile(
	_In_ PCWSTR Filename
	);

NTSTATUS
SziFileEraseFltUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
	);

NTSTATUS
SziFileEraseFltSecureDeleteFile(
	_In_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FileObject,
	_In_ BOOLEAN OnClose
	);

NTSTATUS
SziFileEraseDeleteEmptyDir(
	_In_ POBJECT_ATTRIBUTES
	);

VOID
SziFileEraseContextFree(
	_In_ PCONTEXT_ERASEFILE Context
	);
