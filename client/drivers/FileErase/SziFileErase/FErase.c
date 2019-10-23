//
//	This code belongs to Data Guard project.
//
//	burluckij@gmail.com
//

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <Ntstrsafe.h>
#include <ntddk.h>

#include "DgiLogging.h"
#include "DgiRegistry.h"
#include "FErase.h"
#include "DriverFileErase.h"
#include "FEraseFltTypes.h"
#include "DgiTools.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER			gFilterHandle = NULL;
PDEVICE_OBJECT		gDrvObject = NULL;
ULONG_PTR			gOperationStatusCtx = 1;
ULONG				gTraceFlags = 0;
USER_CONTEXT_EXT	gUserContext;

/*************************************************************************
    Prototypes
*************************************************************************/

#include "DgiPrototype.h"

VOID
ReDriverEntry(
	_In_ struct _DRIVER_OBJECT *DriverObject,
	_In_ PVOID  Context,
	_In_ ULONG  Count
	);

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FEraseUnloadRoutine)
#pragma alloc_text(PAGE, FEraseFltUnload)
#pragma alloc_text(PAGE, FEraseInstanceQueryTeardown)
#pragma alloc_text(PAGE, FEraseInstanceSetup)
#pragma alloc_text(PAGE, FEraseInstanceTeardownStart)
#pragma alloc_text(PAGE, FEraseInstanceTeardownComplete)
#pragma alloc_text(PAGE, FEraseAllocateContext)
#pragma alloc_text(PAGE, FErasePreSetInfoCallback)
#pragma alloc_text(PAGE, FErasePreCleanupCallback)
#pragma alloc_text(PAGE, FErasePostCleanupCallback)
//#pragma alloc_text(PAGE, FErasePostSetInfoCallback)   //!!! NONPage
//#pragma alloc_text(PAGE, FEraseDeviceControlRoutine)	//!!! NONPage
//#pragma alloc_text(PAGE, FEraseSecureDeleteFile)		//!!! NONPage
//#pragma alloc_text(PAGE, FEraseFltSecureDeleteFile)	//!!! NONPage
#endif

#pragma data_seg("NONPAGE")
// declare your globals here
#pragma data_seg()

CONST FLT_CONTEXT_REGISTRATION Contexts[] = {
	{
		FLT_STREAM_CONTEXT,
		0,
		FEraseStreamContextCleanup,
		sizeof(DF_STREAM_CONTEXT),
		DF_STREAM_CONTEXT_POOL_TAG,
		NULL,
		NULL,
		NULL
	},

	{ FLT_CONTEXT_END }
};

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    {
		IRP_MJ_CREATE,
		0,
		FErasePreCreate,
		FErasePostCreate
	},

	{
		IRP_MJ_SET_INFORMATION,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		FErasePreSetInfoCallback,
		FErasePostSetInfoCallback
	},

	{
		IRP_MJ_CLEANUP,
		0,
		FErasePreCleanupCallback,
		FErasePostCleanupCallback
	},

    { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    Contexts,                           //  Context
    Callbacks,                          //  Operation callbacks

    NULL,					              //  MiniFilterUnload

    FEraseInstanceSetup,                    //  InstanceSetup
    FEraseInstanceQueryTeardown,            //  InstanceQueryTeardown
    FEraseInstanceTeardownStart,            //  InstanceTeardownStart
    FEraseInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL,                               //  NormalizeNameComponent
	NULL,								//  TransactionNotification
#if FLT_MGR_WIN8
	NULL,
#endif
	NULL                                //  NormalizeNameComponentEx
};

#define STRINGIFY(s) XSTRINGIFY(s)
#define XSTRINGIFY(s) #s
#pragma message ("NTDDI_VERSION=" STRINGIFY(NTDDI_VERSION))

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
FEraseCompleteIrp(
	_In_ PIRP Irp,
	_In_ NTSTATUS Status,
	_In_ ULONG Info
	)
{
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
FEraseIrpSuccessHandler(
	PDEVICE_OBJECT Fdo, 
	PIRP Irp
	)
{
	UNREFERENCED_PARAMETER(Fdo);
	return FEraseCompleteIrp(Irp, STATUS_SUCCESS, 0);
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS				status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING			devName = RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING			devSymLink = RTL_CONSTANT_STRING(DEVICE_SYMLINK_NAME);
	UNICODE_STRING			regTrageFlag = RTL_CONSTANT_STRING(REGVALUE_DEBUGFLAGS);

	CommonRegGetValueULong(RegistryPath, &regTrageFlag, &gTraceFlags);

	DriverObject->DriverUnload = NULL;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FEraseDeviceControlRoutine;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = FEraseIrpSuccessHandler;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = FEraseIrpSuccessHandler;
	DriverObject->MajorFunction[IRP_MJ_READ] = FEraseIrpSuccessHandler;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = FEraseIrpSuccessHandler;

#ifdef _DEBUG
	gTraceFlags = PTDBG_TRACE_ROUTINES | PTDBG_TRACE_ERRORS | PTDBG_TRACE_OPERATION_STATUS | PTDBG_TRACE_VALUE | PTDBG_TRACE_FILE;
	//DriverObject->DriverUnload = FEraseUnloadRoutine;
#endif // _DEBUG
	
	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!gTraceFlags: 0x%X\n", gTraceFlags));

	status = DgiCreateUnicodeStringFromUnicodeString(&gUserContext.RegistryPath, RegistryPath, DF_STRING_POOL_TAG);
	if(!NT_SUCCESS(status))
	{
		goto _error;
	}

    status = FltRegisterFilter(DriverObject,  &FilterRegistration, &gFilterHandle);

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status ))
	{
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &gDrvObject);
		if(!NT_SUCCESS(status))
		{
			FltUnregisterFilter(gFilterHandle);
			goto _error;
		}

		status = IoCreateSymbolicLink(&devSymLink, &devName);
		if(!NT_SUCCESS(status))
		{
			IoDeleteDevice(gDrvObject);			
			FltUnregisterFilter(gFilterHandle);
			gDrvObject = NULL;
			goto _error;
		}

        status = FltStartFiltering(gFilterHandle);
        if (!NT_SUCCESS( status ))
		{
            FltUnregisterFilter(gFilterHandle);
			IoDeleteDevice(gDrvObject);	
			gDrvObject = NULL;
        }

		if (NT_SUCCESS(status))
		{
			InterlockedExchange(&gUserContext.AutoErase, 1);
			KIRQL	OldIrql;
			KeInitializeSpinLock(&gUserContext.Lock);
			KeAcquireSpinLock(&gUserContext.Lock, &OldIrql);
			gUserContext.Count = 1;
			gUserContext.Mask = 0xFF;
			KeReleaseSpinLock(&gUserContext.Lock, OldIrql);

			IoRegisterDriverReinitialization(DriverObject, ReDriverEntry, NULL);
		}
    }

_error:
	if(!NT_SUCCESS(status))
	{
		if(gUserContext.RegistryPath.Buffer != NULL)
		{
			ExFreePoolWithTag(gUserContext.RegistryPath.Buffer, DF_STRING_POOL_TAG);
		}
	}

    return status;
}

VOID
ReDriverEntry(
	_In_ struct _DRIVER_OBJECT *DriverObject,
	_In_ PVOID  Context,
	_In_ ULONG  Count
	)
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(DriverObject);

	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s %d\n", __FUNCTION__, Count));

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING		regKey;
	UNICODE_STRING		logFileName;
	LONG				isAutoErase = 0;

	if(Count == 1)
	{
		if(BooleanFlagOn(gTraceFlags, PTDBG_TRACE_FILE))
		{
			RtlInitUnicodeString(&regKey, REGVALUE_LOGFILE);
			status = CommonRegGetValueString(&gUserContext.RegistryPath, &regKey, &logFileName, DF_STRING_POOL_TAG);
			if(NT_SUCCESS(status))
			{
				CommonInitLog(&logFileName, &gUserContext.Log);
				ExFreePoolWithTag(logFileName.Buffer, DF_STRING_POOL_TAG);
			}
		}

		RtlInitUnicodeString(&regKey, REGVALUE_AUTOERASE);
		status = CommonRegGetValueLong(&gUserContext.RegistryPath, &regKey, &isAutoErase);
		if (NT_SUCCESS(status))
		{
			InterlockedExchange(&gUserContext.AutoErase, isAutoErase);
		}		
	}
}

NTSTATUS 
FEraseDeviceControlRoutine(
	PDEVICE_OBJECT Fdo, 
	PIRP Irp
	)
{
	UNREFERENCED_PARAMETER(Fdo);

	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	ULONG						bytesTxd = 0;
	PIO_STACK_LOCATION			irpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG						controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG						method = METHOD_FROM_CTL_CODE(controlCode);
	ULONG						bufferInSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG						bufferOutSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID						buffer = Irp->AssociatedIrp.SystemBuffer;
	PSECURE_WORKITEM_CONTEXT	contextQueue = NULL;

	// Диспетчеризация по IOCTL кодам:
	switch(controlCode)
	{
		case IOCTL_ERASE_FILE:
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Method : IOCTL_ERASE_FILE\n"));

			if (bufferInSize == sizeof(USER_ERASEFILE_CONTEXT) && method == METHOD_BUFFERED)
			{
				contextQueue = (PSECURE_WORKITEM_CONTEXT)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SECURE_WORKITEM_CONTEXT), DF_SECURE_WORKITEM_CONTEXT);
				if (contextQueue == NULL)
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
					goto _error;
				}

				RtlZeroMemory(contextQueue, sizeof(SECURE_WORKITEM_CONTEXT));

				__try
				{
					PUSER_ERASEFILE_CONTEXT userContext = (PUSER_ERASEFILE_CONTEXT)buffer;

					if ((PVOID)userContext->FileName < MmHighestUserAddress)
					{
						ProbeForRead((PVOID)userContext->FileName, (userContext->Length / sizeof(WCHAR)), 1);
					}
					else
					{
						status = STATUS_INVALID_DEVICE_REQUEST;
						goto _error;
					}

					HANDLE		hEvent = userContext->Event;
					PCWSTR		fileName = userContext->FileName;
					SIZE_T		length = (userContext->Length + 1) * sizeof(WCHAR);

					if (hEvent && fileName && length > 0 && length < MAX_PATH)
					{
						status = ObReferenceObjectByHandle(hEvent,
							EVENT_MODIFY_STATE,
							*ExEventObjectType,
							Irp->RequestorMode,
							(PVOID*)&contextQueue->Event,
							NULL);

						if (NT_SUCCESS(status))
						{
							RtlStringCbCopyW(contextQueue->FileName, length, userContext->FileName);
							contextQueue->Length = length;
						}
					}
					else
					{
						status = STATUS_INVALID_DEVICE_REQUEST;
						PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					status = STATUS_INVALID_DEVICE_REQUEST;
					PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
				}

				if (NT_SUCCESS(status))
				{
					contextQueue->WorkItem = IoAllocateWorkItem(Fdo);
					if (contextQueue->WorkItem)
					{
						IoQueueWorkItem(contextQueue->WorkItem, FEraseDiskSecureWorkItem, DelayedWorkQueue, contextQueue);
						status = STATUS_SUCCESS;
					}
					else
					{
						ExFreePoolWithTag(contextQueue, DF_SECURE_WORKITEM_CONTEXT);
						ObDereferenceObject(contextQueue->Event);
						contextQueue = NULL;
						status = STATUS_INSUFFICIENT_RESOURCES;
						PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
						goto _error;
					}
				}
				else
				{
					ExFreePoolWithTag(contextQueue, DF_SECURE_WORKITEM_CONTEXT);
					contextQueue = NULL;
					goto _error;
				}
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}

			break;
		}

		case IOCTL_ERASE_AUTOERASE:
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Method : IOCTL_ERASE_AUTOERASE\n"));

			if(bufferInSize == sizeof(DWORD) && method == METHOD_BUFFERED)
			{				
				__try
				{
					DWORD value = *(DWORD*)buffer;
					if (value == 0)
					{
						InterlockedExchange(&gUserContext.AutoErase, 0);
					}
					else
					{
						InterlockedExchange(&gUserContext.AutoErase, 1);
					}

					status = STATUS_SUCCESS;
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{
					status = STATUS_INVALID_DEVICE_REQUEST;
				}
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}

			break;
		}

		case IOCTL_ERASE_ISAUTOERASE:
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Method : IOCTL_ERASE_ISAUTOERASE\n"));
			if(bufferOutSize == sizeof(DWORD) && method == METHOD_BUFFERED)
			{				
				LONG current = InterlockedCompareExchange(&gUserContext.AutoErase, 0, 0);
				
				__try
				{
					DWORD* value = (DWORD*)Irp->UserBuffer;
					*value = current == 0 ? 0 : 1;
					status = STATUS_SUCCESS;
				}
				__except(EXCEPTION_EXECUTE_HANDLER) {
					status = STATUS_INVALID_DEVICE_REQUEST;
				}
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}

			break;
		}

		case IOCTL_ERASE_LASTSTATUS:
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Method : IOCTL_ERASE_LASTSTATUS\n"));

			if(bufferOutSize == sizeof(NTSTATUS) && method == METHOD_BUFFERED)
			{
				KIRQL	oldIrql;
				KeAcquireSpinLock(&gUserContext.Lock, &oldIrql);
				NTSTATUS lastStatus = gUserContext.LastStatus;
				gUserContext.LastStatus = 0;
				KeReleaseSpinLock(&gUserContext.Lock, oldIrql);

				__try
				{
					NTSTATUS * value = (NTSTATUS *)Irp->UserBuffer;
					*value = lastStatus;
					status = STATUS_SUCCESS;
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{
					status = STATUS_INVALID_DEVICE_REQUEST;
				}				
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}

			break;
		}

		case IOCTL_ERASE_MASK:
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Method : IOCTL_ERASE_MASK\n"));

			if(bufferInSize == sizeof(UCHAR) && method == METHOD_BUFFERED)
			{
				UCHAR mask = 0xFF;

				__try
				{
					mask = *(UCHAR*)buffer;
					status = STATUS_SUCCESS;
				} __except(EXCEPTION_EXECUTE_HANDLER)
				{
					status = STATUS_INVALID_DEVICE_REQUEST;
				}

				if (NT_SUCCESS(status))
				{
					KIRQL	oldIrql;
					KeAcquireSpinLock(&gUserContext.Lock, &oldIrql);
					gUserContext.Mask = mask;
					KeReleaseSpinLock(&gUserContext.Lock, oldIrql);
				}
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}

			break;
		}

		case IOCTL_ERASE_COUNT:
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("Method : IOCTL_ERASE_COUNT\n"));

			if(bufferInSize == sizeof(UINT32) && method == METHOD_BUFFERED)
			{				
				UINT32 count = 1;

				__try
				{
					count = *(UINT32*)buffer;
					status = STATUS_SUCCESS;
				} __except(EXCEPTION_EXECUTE_HANDLER)
				{
					status = STATUS_INVALID_DEVICE_REQUEST;
				}

				if(NT_SUCCESS(status))
				{
					KIRQL	oldIrql;
					KeAcquireSpinLock(&gUserContext.Lock, &oldIrql);
					gUserContext.Count = count;
					KeReleaseSpinLock(&gUserContext.Lock, oldIrql);
				}
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}
			break;
		}

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
		};

_error:
	if (!NT_SUCCESS(status))
	{
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}

	return FEraseCompleteIrp(Irp, status, bytesTxd); // Завершение IRP;
}

VOID 
FEraseUnloadRoutine(
	_In_ PDRIVER_OBJECT pDriverObject
	)
{
	UNREFERENCED_PARAMETER(pDriverObject);

	PAGED_CODE();

	UNICODE_STRING  devSymLink = RTL_CONSTANT_STRING(DEVICE_SYMLINK_NAME);

	IoDeleteSymbolicLink(&devSymLink);
	IoDeleteDevice(gDrvObject);
	CommonFreeLog(gUserContext.Log);
	ExFreePoolWithTag(gUserContext.RegistryPath.Buffer, DF_STRING_POOL_TAG);	
}

NTSTATUS
FEraseFltUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();
	
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("FileErase!%s: Entered\n", __FUNCTION__) );
	FltUnregisterFilter(gFilterHandle);
    return STATUS_SUCCESS;
}

NTSTATUS
FEraseInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
 	NTSTATUS	status = STATUS_FLT_DO_NOT_ATTACH;
	BOOLEAN		isWritable = FALSE;
	
	PAGED_CODE();

	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	
	if(FILE_DEVICE_DISK_FILE_SYSTEM != VolumeDeviceType)
	{
		return status;
	}

	status = FltIsVolumeWritable(FltObjects->Volume, &isWritable);

	if(!NT_SUCCESS(status))
	{
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	if(isWritable)
	{
		switch(VolumeFilesystemType)
		{
			case FLT_FSTYPE_NTFS:
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,  ("FileErase!Attach to NTFS\n"));
				break;

			case FLT_FSTYPE_REFS:
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,  ("FileErase!Attach to REFS\n"));
				break;

			case FLT_FSTYPE_FAT:
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,  ("FileErase!Attach to FAT\n"));
				break;

			case FLT_FSTYPE_EXFAT:
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,  ("FileErase!Attach to EXFAT\n"));
				break;

			default:
				return STATUS_FLT_DO_NOT_ATTACH;
		}
		status = STATUS_SUCCESS;
	}
	else
	{
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	return status;
}

NTSTATUS
FEraseInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    return STATUS_SUCCESS;
}

VOID
FEraseInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();
}

VOID
FEraseInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();
}

FLT_PREOP_CALLBACK_STATUS
FErasePreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
	UNREFERENCED_PARAMETER(FltObjects);
	
    NTSTATUS			status = STATUS_UNSUCCESSFUL;
	PDF_STREAM_CONTEXT	streamContext = NULL;
	
	LONG isAutoErase = InterlockedCompareExchange(&gUserContext.AutoErase, 0, 0);
	if(isAutoErase == 0)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if(FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE))
	{
		status = FEraseAllocateContext(&streamContext);

		if(NT_SUCCESS(status))
		{
			*CompletionContext = (PVOID)streamContext;
			return FLT_PREOP_SYNCHRONIZE;
		}
	}

	*CompletionContext = NULL;
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
FErasePostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
	UNREFERENCED_PARAMETER(Flags);

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("FileErase!%s: Entered\n", __FUNCTION__) );

	ASSERT(NULL != CompletionContext);
	ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

	if(NT_SUCCESS(Data->IoStatus.Status) &&	(STATUS_REPARSE != Data->IoStatus.Status))
	{
		PDF_STREAM_CONTEXT	streamContext = (PDF_STREAM_CONTEXT)CompletionContext;
		NTSTATUS			status = STATUS_UNSUCCESSFUL;

		status = FEraseGetOrSetContext(FltObjects, Data->Iopb->TargetFileObject, &streamContext);

		if (NT_SUCCESS(status))
		{
			streamContext->DeleteOnClose = BooleanFlagOn(Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE);
		}
	}
	
	FltReleaseContext(CompletionContext);	
	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
FErasePreSetInfoCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    NTSTATUS	status = STATUS_SUCCESS;
   
	PAGED_CODE();

	LONG isAutoErase = InterlockedCompareExchange(&gUserContext.AutoErase, 0, 0);
	if(isAutoErase == 0)
	{
		goto _exit;
	}
		
	if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
	{
		if(FltObjects->Transaction != NULL)
		{
			//Стирание файла в транзакции. Оверхед при роллбеке но по другому ни как.
			status = FEraseFltSecureDeleteFile(Data, FltObjects, FALSE);

			Data->IoStatus.Status = status;
			Data->IoStatus.Information = 0;
		}
		else
		{
			PDF_STREAM_CONTEXT	streamContext = NULL;
			status = FEraseGetOrSetContext(FltObjects, Data->Iopb->TargetFileObject, &streamContext);

			if(!NT_SUCCESS(status))
			{
				Data->IoStatus.Status = status;
				Data->IoStatus.Information = 0;
				goto _exit;
			}
			
			PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: PreStatus 0x%X %wZ\n", __FUNCTION__, Data->IoStatus.Status, &Data->Iopb->TargetFileObject->FileName));
			PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: NumOps %d\n", __FUNCTION__, streamContext->NumOps));
			PT_DBG_FILE("%s: Context %p", __FUNCTION__, streamContext);
			PT_DBG_FILE("%s: ContextFile %wZ", __FUNCTION__, &Data->Iopb->TargetFileObject->FileName);

			// Если NumOps > 1 то это папка
			if(!(InterlockedIncrement(&streamContext->NumOps) > 1))
			{
				*CompletionContext = (PVOID)streamContext;
				return FLT_PREOP_SYNCHRONIZE;
			}
			else
			{
				PT_DBG_FILE("%s: FltReleaseContext: NumOps %d Erase %d OnClose %d IsE %d",
					__FUNCTION__,
					streamContext->NumOps,
					streamContext->Erase,
					streamContext->DeleteOnClose,
					streamContext->IsErase);

				FltReleaseContext(streamContext);
			}
		}
	}

_exit:
	if(!NT_SUCCESS(status))
	{
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}

	*CompletionContext = NULL;
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
FErasePostSetInfoCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(FltObjects);

    ASSERT( Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation );
    ASSERT( NULL != CompletionContext );
	ASSERT(FltObjects->Transaction == NULL);

	PDF_STREAM_CONTEXT	streamContext = CompletionContext;
	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: NumOps %d Erase %d\n", __FUNCTION__, streamContext->NumOps, streamContext->Erase));
	PT_DBG_FILE("%s: NumOps %d Erase %d OnClose %d IsE %d", __FUNCTION__, streamContext->NumOps, streamContext->Erase, streamContext->DeleteOnClose, streamContext->IsErase);
	PT_DBG_FILE("%s: Context %p", __FUNCTION__, streamContext);
	PT_DBG_FILE("%s: ContextFile %wZ", __FUNCTION__, &Data->Iopb->TargetFileObject->FileName);

	//Если ОС готова удалить файл помечаем на удаление
	//if(FltObjects->FileObject->DeletePending == TRUE && NT_SUCCESS(Data->IoStatus.Status) && (STATUS_REPARSE != Data->IoStatus.Status)) {

	if(NT_SUCCESS(Data->IoStatus.Status) && (STATUS_REPARSE != Data->IoStatus.Status))
	{
		//InterlockedExchange(&streamContext->Erase, 1);
		InterlockedIncrement(&streamContext->Erase);
	}

	//else {
	//	InterlockedExchange(&streamContext->Erase, 0);
	//}

	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: PostStatus 0x%X %wZ\n", __FUNCTION__, Data->IoStatus.Status, &Data->Iopb->TargetFileObject->FileName));
	
	FltReleaseContext(CompletionContext);
    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FErasePreCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    PAGED_CODE();
	
	if (FltObjects->Transaction == NULL)
	{
		NTSTATUS				status = STATUS_UNSUCCESSFUL;
		PDF_STREAM_CONTEXT		streamContext = NULL;

		status = FltGetStreamContext(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, &streamContext);

		if(!NT_SUCCESS(status))
		{
			goto _exit;
		}

		PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: PreStatus 0x%X %wZ\n", __FUNCTION__, Data->IoStatus.Status, &Data->Iopb->TargetFileObject->FileName));
		PT_DBG_FILE("%s: NumOps %d Erase %d OnClose %d IsE %d", __FUNCTION__, streamContext->NumOps, streamContext->Erase, streamContext->DeleteOnClose, streamContext->IsErase);
		PT_DBG_FILE("%s: Context %p", __FUNCTION__, streamContext);
		PT_DBG_FILE("%s: ContextFile %wZ", __FUNCTION__, &Data->Iopb->TargetFileObject->FileName);

		//Если файл был создан с флагом удаления после закрытия
		if(streamContext->DeleteOnClose || streamContext->Erase == 1)
		{
			status = FEraseFltSecureDeleteFile(Data, FltObjects, streamContext->DeleteOnClose);

			if(streamContext->DeleteOnClose)
			{
				PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: DeleteOnClose 0x%X %wZ\n", __FUNCTION__, status, &Data->Iopb->TargetFileObject->FileName));
			}
			
			if(streamContext->Erase)
			{
				PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s: EraseFile 0x%X %wZ\n", __FUNCTION__, status, &Data->Iopb->TargetFileObject->FileName));
			}

			if (NT_SUCCESS(status))
			{
				InterlockedIncrement(&streamContext->IsErase);
			}

			Data->IoStatus.Status = status;
			Data->IoStatus.Information = 0;
			
			if(!NT_SUCCESS(status))
			{
				FltReleaseContext(streamContext);
				goto _exit;
			}

			*CompletionContext = (PVOID)streamContext;
			return FLT_PREOP_SYNCHRONIZE;
		}

		FltReleaseContext(streamContext);
	}
	
_exit:

	*CompletionContext = NULL;
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
FErasePostCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(Data);

    PAGED_CODE();

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("FileErase!%s: Entered\n", __FUNCTION__) );
	
    ASSERT( !FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) );
    ASSERT( NULL != CompletionContext );
	
	PDF_STREAM_CONTEXT	streamContext = CompletionContext;
	PT_DBG_FILE("%s: NumOps %d Erase %d OnClose %d IsE %d", __FUNCTION__, streamContext->NumOps, streamContext->Erase, streamContext->DeleteOnClose, streamContext->IsErase);
	PT_DBG_FILE("%s: Context %p", __FUNCTION__, streamContext);
	PT_DBG_FILE("%s: ContextFile %wZ", __FUNCTION__, &Data->Iopb->TargetFileObject->FileName);

	FltReleaseContext(CompletionContext);
    return FLT_POSTOP_FINISHED_PROCESSING;
}
