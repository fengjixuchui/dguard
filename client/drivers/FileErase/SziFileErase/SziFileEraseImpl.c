#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#include "SziFltTypes.h"
#include "SziFileErase.h"
#include "SziPrototype.h"
#include <SziTools.h>

extern ULONG			gTraceFlags;
extern PFLT_FILTER		gFilterHandle;
extern USER_CONTEXT_EXT	gUserContext;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SziFileEraseFltSecureOverwrite)
//#pragma alloc_text(PAGE, SziFileEraseSecureDeleteFile) //!!! NONPage
#endif

NTSTATUS
SziFileEraseFltGetFileClusters(
	_In_ PFLT_CONTEXT_ERASEFILE context, 
	_Out_ PRETRIEVAL_POINTERS_BUFFER* VcnList
	)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	PRETRIEVAL_POINTERS_BUFFER	pbub = NULL;
	STARTING_VCN_INPUT_BUFFER	pinbuf;	
	ULONG						countPointers = 200;
	ULONG						sizeBuffer = sizeof(RETRIEVAL_POINTERS_BUFFER) * countPointers;
	
	pinbuf.StartingVcn.QuadPart = 0;
	pbub = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(NonPagedPool, sizeBuffer, DF_PRETRIEVAL_POINTERS_BUFFER);
	if(pbub == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
		goto _error;
	}

	while((status = FltFsControlFile(context->Instance, context->FObject,
									FSCTL_GET_RETRIEVAL_POINTERS,
									&pinbuf, sizeof(STARTING_VCN_INPUT_BUFFER),
									pbub, sizeBuffer,
									NULL)) == STATUS_BUFFER_OVERFLOW || status == STATUS_PENDING)
	{
		if(status == STATUS_PENDING) {
			PT_DBG_FILE("FileErase!STATUS_PENDING %s:%d", __FILE__, __LINE__);
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileErase!%s:%d STATUS_PENDING\n", __FILE__, __LINE__));

			goto _error;
		}
		countPointers += 200;
		sizeBuffer = sizeof(RETRIEVAL_POINTERS_BUFFER) * countPointers;
		ExFreePoolWithTag(pbub, DF_PRETRIEVAL_POINTERS_BUFFER);
		pbub = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(NonPagedPool, sizeBuffer, DF_PRETRIEVAL_POINTERS_BUFFER);
		if(pbub == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto _error;
		}
	}
	
_error:	
	if(!NT_SUCCESS(status) && pbub != NULL) {
		ExFreePoolWithTag(pbub, DF_PRETRIEVAL_POINTERS_BUFFER);
		*VcnList = NULL;
	} else {
		*VcnList = pbub;
	}

	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseFltEraseFile(
	_In_ PFLT_CONTEXT_ERASEFILE Context, 
	_In_ UINT32 Count, 
	_In_ UCHAR Mask)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER				offset;
	PFILE_OBJECT				volFileObject = NULL;
	FILE_FS_SIZE_INFORMATION	fsInfo;
	ULONG						clusterSize = 0;
	HANDLE						volHandle = 0;

	//Получаем размер кластера
	status = FltOpenVolume(Context->Instance, &volHandle, &volFileObject);
	if (!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}

	status = FltQueryVolumeInformationFile(Context->Instance, volFileObject, &fsInfo, sizeof(fsInfo), FileFsSizeInformation, NULL);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	
	clusterSize = fsInfo.BytesPerSector * fsInfo.SectorsPerAllocationUnit;
	PT_DBG_FILE("%s ClusterSize %d", __FUNCTION__, clusterSize);
	PT_DBG_FILE("%s AllocationSize %d", __FUNCTION__, Context->InfoFile.AllocationSize.QuadPart);

	//Info: Если флаг FO_NO_INTERMEDIATE_BUFFERING то писать можно только по кластерно
	//offset = Context->InfoFile.AllocationSize;
	//if(offset.LowPart > 0) {
	//	offset.LowPart = offset.LowPart - sizeof(UCHAR);
	//} else if(offset.HighPart > 0) {
	//	offset.HighPart = offset.HighPart - sizeof(UCHAR);
	//} else {
	//	status = STATUS_INVALID_PARAMETER;
	//	PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
	//	goto _error;
	//}
	//// Пишем только один байт в конец
	//status = SziFileEraseFltSecureOverwrite(Context, offset, sizeof(UCHAR), Count, Mask);
	//if(!NT_SUCCESS(status)) {
	//	PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
	//	goto _error;
	//}

	// Устанавливаем на начало
	offset.QuadPart = 0;
	LONG bytesToWrite = 0;
	while(offset.QuadPart < Context->InfoFile.AllocationSize.QuadPart) {
		bytesToWrite = (LONG)min(Context->InfoFile.AllocationSize.QuadPart - offset.QuadPart, clusterSize);
		status = SziFileEraseFltSecureOverwrite(Context, offset, bytesToWrite, Count, Mask);
		if(!NT_SUCCESS(status)) {
			break;
		}
		offset.QuadPart += bytesToWrite;
	}
	
_error:
	if(volHandle != 0) {
		FltClose(volHandle);
	}
	if (volFileObject != NULL) {
		ObDereferenceObject(volFileObject);
	}
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseFltEraseFileByBlocks(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask) 
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	PRETRIEVAL_POINTERS_BUFFER	vcnList = NULL;
	HANDLE						volHandle = NULL;	
	FILE_FS_SIZE_INFORMATION	fsInfo;
	ULONG						LengthRet = 0;
	ULONG						clusterSize = 0;
	PUCHAR						buffer = NULL;
	KEVENT						event;
	PFILE_OBJECT				volObject = NULL;
		
	status = FltOpenVolume(Context->Instance, &volHandle, &volObject);
	if (!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}

	status = FltQueryVolumeInformationFile(Context->Instance, volObject, &fsInfo, sizeof(fsInfo), FileFsSizeInformation, &LengthRet);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	
	clusterSize = fsInfo.BytesPerSector * fsInfo.SectorsPerAllocationUnit;
	PT_DBG_FILE("%s ClusterSize %d", __FUNCTION__, clusterSize);
	PT_DBG_FILE("%s AllocationSize %d", __FUNCTION__, Context->InfoFile.AllocationSize.QuadPart);

	status = SziFileEraseFltGetFileClusters(Context, &vcnList);
	if(!NT_SUCCESS(status) || vcnList == NULL) {
		goto _error;
	}	

	PT_DBG_FILE("%s VCN Count %d", __FUNCTION__, vcnList->ExtentCount);

	buffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, clusterSize, DF_ERASEBUFFER_POOL_TAG);
	if(buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlFillMemory(buffer, clusterSize, Mask);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	LARGE_INTEGER		lcn, startVcn = vcnList->StartingVcn;
	LARGE_INTEGER		position;
	IO_STATUS_BLOCK		ioStatus;
	for(ULONG index = 0; index < vcnList->ExtentCount; ++index) {
		lcn = vcnList->Extents[index].Lcn;
		// On NT 4.0, a compressed virtual run (0-filled) is identified with a cluster offset of -1
		// On the NTFS file system, the value (LONGLONG) –1 indicates either a compression unit that is partially allocated, or an unallocated region of a sparse file.
		if(lcn.QuadPart != LLINVALID) {
			//Список VCN каждая запись смещение относительно начала логического диска. В одной VCN может быть N реальных кластеров. 
			//Сколько кластеров в VCN указывает следующая запись.
			position.QuadPart = lcn.QuadPart * clusterSize;
			for(ULONG clusterCount = 0; clusterCount < (ULONG)(vcnList->Extents[index].NextVcn.QuadPart - startVcn.QuadPart); ++clusterCount) {
				// N раз посылаем IRP пакет для стирания размером с кластер
				for(UINT32 passes = 1; passes <= Count; ++passes) {
					PIRP IrpFsd = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, volObject->DeviceObject, buffer, clusterSize, &position, &event, &ioStatus);
					if(!IrpFsd) {
						status = STATUS_INSUFFICIENT_RESOURCES;
						PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
						PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
						goto _error;
					}
					IrpFsd->IoStatus.Status = STATUS_SUCCESS;
					status = IoCallDriver(volObject->DeviceObject, IrpFsd);
					if(status == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						status = ioStatus.Status;
					}
					KeClearEvent(&event);
					
					if(!NT_SUCCESS(status)) {
						PT_DBG_FILE("%s Status 0x%X", __FUNCTION__, status);
						PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
						goto _error;
					}
				}
				position.QuadPart += clusterSize;
			}
		}
		startVcn = vcnList->Extents[index].NextVcn;
	}

_error:
	if (volHandle != NULL) {
		FltClose(volHandle);
	}
	if (volObject != NULL) {
		ObDereferenceObject(volObject);
	}
	if (vcnList != NULL) {
		ExFreePoolWithTag(vcnList, DF_PRETRIEVAL_POINTERS_BUFFER);
	}
	if (buffer) {
		ExFreePoolWithTag(buffer, DF_ERASEBUFFER_POOL_TAG);
	}

	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS 
SziFileEraseFltSecureOverwrite(
	_In_ PFLT_CONTEXT_ERASEFILE Context,
	_In_ LARGE_INTEGER Offset,
	_In_ ULONG Length,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	PUCHAR						buffer = NULL;
	ULONG						lengthReturn = 0;
	
    PAGED_CODE();
	
	buffer = (PUCHAR)FltAllocatePoolAlignedWithTag(Context->Instance, PagedPool, Length, DF_ERASEBUFFER_POOL_TAG);
	if(buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlFillMemory(buffer, Length, Mask);

	for(UINT32 passes = 1; passes <= Count; ++passes) {
		status = FltWriteFile(Context->Instance, Context->FObject, &Offset, Length, buffer,
								FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &lengthReturn, NULL, NULL);
		if(!NT_SUCCESS(status)) {
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _error;
		}		
	}
	FltFlushBuffers(Context->Instance, Context->FObject);
_error:
	if(buffer != NULL) {
		ExFreePoolWithTag(buffer, DF_ERASEBUFFER_POOL_TAG);
	}

	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

VOID 
SziFileEraseDiskSecureWorkItem(
	PDEVICE_OBJECT Fdo,
	PVOID Context
	)
{
	UNREFERENCED_PARAMETER(Fdo);

	PSECURE_WORKITEM_CONTEXT	WorkContext = (PSECURE_WORKITEM_CONTEXT)Context;	
	KIRQL						oldIrql;	
	NTSTATUS					status = STATUS_UNSUCCESSFUL;

	if (WorkContext) {
		status = SziFileEraseSecureDeleteFile(WorkContext->FileName);
		if(!NT_SUCCESS(status)) {
			PT_DBG_FILE("LastStatus 0x%X", status);
		}
		KeAcquireSpinLock(&gUserContext.Lock, &oldIrql);
		gUserContext.LastStatus = status;
		KeReleaseSpinLock(&gUserContext.Lock, oldIrql);

		KeSetEvent(WorkContext->Event, EVENT_INCREMENT, FALSE);
		IoFreeWorkItem(WorkContext->WorkItem);
		ObDereferenceObject(WorkContext->Event);
		ExFreePoolWithTag(WorkContext, DF_SECURE_WORKITEM_CONTEXT);
	}	
}

NTSTATUS
SziFileEraseContextInit(
	_In_ PUNICODE_STRING FileName,
	_In_ PUNICODE_STRING Volume,
	_Out_ PCONTEXT_ERASEFILE Context
	)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES	objAttr;

	RtlZeroMemory(Context, sizeof(CONTEXT_ERASEFILE));
	
	if (!RtlCreateUnicodeString(&Context->FileName, FileName->Buffer)) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _exit;
	}

	if(!RtlCreateUnicodeString(&Context->Volume, Volume->Buffer)) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error1;
	}

	InitializeObjectAttributes(&objAttr, &Context->FileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	//Проверяем какой файл
	status = ZwQueryFullAttributesFile(&objAttr, &Context->NetInfoFile);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X File %wZ %s:%d", status, &Context->FileName, __FILE__, __LINE__);
		goto _error2;
	}

	status = STATUS_SUCCESS;
	goto _exit;
_error2:
	RtlFreeUnicodeString(&Context->Volume);

_error1:
	RtlFreeUnicodeString(&Context->FileName);

_exit:

	return status;
}

VOID
SziFileEraseContextFree(
	_In_ PCONTEXT_ERASEFILE Context
	)
{
	RtlFreeUnicodeString(&Context->FileName);
	RtlFreeUnicodeString(&Context->Volume);
}

NTSTATUS 
SziFileEraseSecureDeleteFile(
	_In_ PCWSTR Filename
	)
{
	KIRQL					oldIrql;
	UINT32					count = 1;
	UCHAR					mask = 0xFF;
	OBJECT_ATTRIBUTES		objAttr;
	CONTEXT_ERASEFILE		context;
	WCHAR*					uniNameBuffer = NULL;
	WCHAR*					volumeBuffer = NULL;
	NTSTATUS				status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING			uniFileName;
	UNICODE_STRING			volume;

	KeAcquireSpinLock(&gUserContext.Lock, &oldIrql);
	count = gUserContext.Count;
	mask = gUserContext.Mask;
	KeReleaseSpinLock(&gUserContext.Lock, oldIrql);

	volumeBuffer = ExAllocatePoolWithTag(PagedPool, MAX_PATH, DF_STRING_POOL_TAG);
	if(volumeBuffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(volumeBuffer, MAX_PATH);

	uniNameBuffer = ExAllocatePoolWithTag(PagedPool, MAX_PATH, DF_STRING_POOL_TAG);
	if(uniNameBuffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(uniNameBuffer, MAX_PATH);

	//Получаем литеру тома
	if(Filename[1] == L':' && Filename[2] == L'\\') {
		WCHAR volumLiter[4] = L"\0";
		volumLiter[0] = Filename[0];
		volumLiter[1] = Filename[1];
		
		RtlInitEmptyUnicodeString(&volume, volumeBuffer, MAX_PATH);
		status = RtlAppendUnicodeToString(&volume, L"\\??\\");
		if(!NT_SUCCESS(status)) {
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _error;
		}
		status = RtlAppendUnicodeToString(&volume, volumLiter);
		if(!NT_SUCCESS(status)) {
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _error;
		}
		//Дос имя
		RtlInitEmptyUnicodeString(&uniFileName, uniNameBuffer, MAX_PATH);
		status = RtlAppendUnicodeToString(&uniFileName, L"\\??\\");
		if(!NT_SUCCESS(status)) {
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _error;
		}
		
		status = RtlAppendUnicodeToString(&uniFileName, Filename);
		if(!NT_SUCCESS(status)) {
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _error;
		}
		
		PT_DBG_FILE("%s %wZ", __FUNCTION__, &uniFileName);
	} else {
		status = STATUS_INVALID_PARAMETER;
		goto _error;
	}

	status = SziFileEraseContextInit(&uniFileName, &volume, &context);
	if (!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}

	LONG isAutoErase = InterlockedCompareExchange(&gUserContext.AutoErase, 0, 0);
	if(isAutoErase == 0) {
		PT_DBG_FILE("FileErase!%s AutoErase %d", __FUNCTION__, isAutoErase);
		
		SziSetFileAttribute(&uniFileName, FILE_ATTRIBUTE_NORMAL);

		if(context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_OFFLINE ||
			context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ||
			context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_DEVICE) 
		{
			PT_DBG_FILE("FileErase!%s FILE_ATTRIBUTE_OFFLINE", __FUNCTION__);
			status = STATUS_NOT_IMPLEMENTED;
		} else if(context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_COMPRESSED ||
				 context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED ||
				 context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
		{
			status = SziFileEraseEraseFileByBlocks(&context, count, mask);
			if(status == STATUS_END_OF_FILE) {				
				status = SziFileEraseEraseFile(&context, count, mask);
			}
		} else if((context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
			status = SziFileEraseEraseFile(&context, count, mask);
		} else if((context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
			status = STATUS_SUCCESS;
		} else {
			PT_DBG_FILE("FileErase!%s NOVALID FLAGS", __FUNCTION__);
			status = STATUS_NOT_IMPLEMENTED;
		}
	}
	
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error1;
	}
	
	InitializeObjectAttributes(&objAttr, &uniFileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	if((context.NetInfoFile.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		status = SziFileEraseDeleteEmptyDir(&objAttr);
		if (!NT_SUCCESS(status)) {
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s SziFileEraseDeleteEmptyDir 0x%X\n", __FUNCTION__, status));
		}		
	} else {
		if (isAutoErase) {
			SziSetFileAttribute(&uniFileName, FILE_ATTRIBUTE_NORMAL);
		}
		
		status = ZwDeleteFile(&objAttr);
		if (!NT_SUCCESS(status)) {
			PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s ZwDeleteFile 0x%X\n", __FUNCTION__, status));
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		}
	}

_error1:
	SziFileEraseContextFree(&context);

_error:
	if(uniNameBuffer != NULL) {
		ExFreePoolWithTag(uniNameBuffer, DF_STRING_POOL_TAG);
	}
	if(volumeBuffer != NULL) {
		ExFreePoolWithTag(volumeBuffer, DF_STRING_POOL_TAG);
	}
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseSecureOverwrite(
	_In_ HANDLE Handle,
	_In_ PFILE_POSITION_INFORMATION positon,
	_In_ PUCHAR Buffer,
	_In_ ULONG Length, 
	_In_ UINT32 Count
	)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK				ioCreateFile;

	for(UINT32 passes = 1; passes <= Count; ++passes) {
		if(passes != 1) {
			status = ZwSetInformationFile(Handle, &ioCreateFile, positon, sizeof(*positon), FilePositionInformation);
			if(!NT_SUCCESS(status)) {
				goto error;
			}
		}

		status = ZwWriteFile(Handle, NULL, NULL, NULL, &ioCreateFile, Buffer, Length, NULL, NULL);
		if(!NT_SUCCESS(status)) {
			goto error;
		}
	}
	status = ZwFlushBuffersFile(Handle, &ioCreateFile);
error:
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseGetFileClusters(
	_In_ HANDLE fHandle, 
	_Out_ PRETRIEVAL_POINTERS_BUFFER* vcnList
	)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK		ioCreateFile;
	STARTING_VCN_INPUT_BUFFER	pinbuf;
	PRETRIEVAL_POINTERS_BUFFER	pbub = NULL;
	ULONG		countPointers = 200;
	SIZE_T		sizeBuffer = sizeof(RETRIEVAL_POINTERS_BUFFER)* countPointers;
	pinbuf.StartingVcn.QuadPart = 0;

	pbub = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(NonPagedPool, sizeBuffer, DF_PRETRIEVAL_POINTERS_BUFFER);
	if(pbub == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto error;
	}

	//TODO: Оптимизировать 200 
	while((status = ZwFsControlFile(fHandle, NULL, NULL, 0, &ioCreateFile, FSCTL_GET_RETRIEVAL_POINTERS,
		&pinbuf, sizeof(STARTING_VCN_INPUT_BUFFER), pbub, (ULONG)sizeBuffer)) == STATUS_BUFFER_OVERFLOW || status == STATUS_PENDING) {
		if(status == STATUS_PENDING) {
			PT_DBG_FILE("%s ZwFsControlFile STATUS_PENDING", __FUNCTION__);
			status = ZwWaitForSingleObject(fHandle, FALSE, NULL);
			if(!NT_SUCCESS(status)) {
				goto error;
			}

			if(NT_SUCCESS(ioCreateFile.Status)) {
				break;
			}
			if(ioCreateFile.Status != STATUS_BUFFER_OVERFLOW) {
				goto error;
			}
		}

		countPointers += 200;
		sizeBuffer = sizeof(RETRIEVAL_POINTERS_BUFFER)* countPointers;
		ExFreePoolWithTag(pbub, DF_PRETRIEVAL_POINTERS_BUFFER);
		pbub = (PRETRIEVAL_POINTERS_BUFFER)ExAllocatePoolWithTag(NonPagedPool, sizeBuffer, DF_PRETRIEVAL_POINTERS_BUFFER);
		if(pbub == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto error;
		}
	}

error:
	if(!NT_SUCCESS(status) && pbub != NULL) {
		ExFreePoolWithTag(pbub, DF_PRETRIEVAL_POINTERS_BUFFER);
		*vcnList = NULL;
	} else {
		*vcnList = pbub;
	}

	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS 
SziFileEraseEraseFile(
	_In_ PCONTEXT_ERASEFILE Context,
	_In_ UINT32 Count, 
	_In_ UCHAR Mask
	)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	HANDLE						fHandle = 0, volHandle = 0;
	IO_STATUS_BLOCK				ioCreateFile;
	OBJECT_ATTRIBUTES			objAttr, volAttr;
	FILE_STANDARD_INFORMATION	hlInfo;
	FILE_FS_SIZE_INFORMATION	fsInfo;
	PUCHAR						buffer = NULL;
	ULONG						clusterSize = 0;
	FILE_POSITION_INFORMATION	position;
	
	// MSDN: If the file was opened or created with the FILE_NO_INTERMEDIATE_BUFFERING option, the value of 
	// CurrentByteOffset must be an integral multiple of the sector size of the underlying device.
	InitializeObjectAttributes(&objAttr, &Context->FileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&fHandle, GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE, &objAttr, &ioCreateFile, NULL, 0, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if(!NT_SUCCESS(status)) {
		PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	
	//Место да диске под файл не выделенно
	if(Context->NetInfoFile.AllocationSize.QuadPart == 0) {
		status = STATUS_SUCCESS;
		goto _exit;
	}
	//Info: Если флаг FO_NO_INTERMEDIATE_BUFFERING то писать можно только по кластерно
	//position.CurrentByteOffset = Context->NetInfoFile.AllocationSize;
	//if(position.CurrentByteOffset.LowPart > 0) {
	//	position.CurrentByteOffset.LowPart = position.CurrentByteOffset.LowPart - sizeof(UCHAR);
	//} else if(position.CurrentByteOffset.HighPart > 0) {
	//	position.CurrentByteOffset.HighPart = position.CurrentByteOffset.HighPart - sizeof(UCHAR);
	//} else {
	//	status = STATUS_INVALID_PARAMETER;
	//	PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
	//	goto _error;
	//}

	status = ZwQueryInformationFile(fHandle, &ioCreateFile, &hlInfo, sizeof(hlInfo), FileStandardInformation);
	if(!NT_SUCCESS(status)) {
		PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status));
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	
	if(hlInfo.NumberOfLinks > 1 || hlInfo.DeletePending == TRUE) {
		status = STATUS_SUCCESS;
		PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FileErase!%s link %d pend %d\n", __FUNCTION__, hlInfo.NumberOfLinks, hlInfo.DeletePending));
		goto _exit;
	}

	InitializeObjectAttributes(&volAttr, &Context->Volume, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenFile(&volHandle, GENERIC_WRITE, &volAttr, &ioCreateFile, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}

	//Получаем размер кластера
	status = ZwQueryVolumeInformationFile(volHandle, &ioCreateFile, &fsInfo, sizeof(fsInfo), FileFsSizeInformation);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}

	clusterSize = fsInfo.BytesPerSector * fsInfo.SectorsPerAllocationUnit;
	PT_DBG_FILE("%s ClusterSize %d", __FUNCTION__, clusterSize);
	PT_DBG_FILE("%s AllocationSize %d", __FUNCTION__, Context->NetInfoFile.AllocationSize.QuadPart);
				
	//status = ZwSetInformationFile(fHandle, &ioCreateFile, &position, sizeof(position), FilePositionInformation);
	//if(!NT_SUCCESS(status)) {
	//	PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
	//	goto _error;
	//}
	//
	//// Пишем только один байт в конец
	//status = SziFileEraseSecureOverwrite(fHandle, &position, &Mask, sizeof(UCHAR), Count);
	//if(!NT_SUCCESS(status)) {
	//	PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
	//	goto _error;
	//}

	// Устанавливаем на начало
	position.CurrentByteOffset.QuadPart = 0;
	status = ZwSetInformationFile(fHandle, &ioCreateFile, &position, sizeof(position), FilePositionInformation);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	
	buffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, clusterSize, DF_ERASEBUFFER_POOL_TAG);
	if(buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlFillMemory(buffer, clusterSize, Mask);
	
	ULONG bytesToWrite = 0, bytesWritten = 0;
	while(position.CurrentByteOffset.QuadPart < Context->NetInfoFile.AllocationSize.QuadPart) {
		bytesToWrite = (ULONG)min(Context->NetInfoFile.AllocationSize.QuadPart - bytesWritten, clusterSize);
		status = SziFileEraseSecureOverwrite(fHandle, &position, buffer, bytesToWrite, Count);
		if(!NT_SUCCESS(status)) {
			break;
		}
		position.CurrentByteOffset.QuadPart += bytesToWrite;
	}

_exit:

_error:
	if(buffer != NULL) {
		ExFreePoolWithTag(buffer, DF_ERASEBUFFER_POOL_TAG);
	}
	if(fHandle != NULL) {
		ZwClose(fHandle);
	}
	if(volHandle != NULL) {
		ZwClose(volHandle);
	}
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseEraseFileByBlocks(
	_In_ PCONTEXT_ERASEFILE Context,
	_In_ UINT32 Count,
	_In_ UCHAR Mask
	)
{
	Count;
	HANDLE						fHandle = 0, volHandle = 0;
	IO_STATUS_BLOCK				ioCreateFile;
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES			objAttr, volAttr;
	FILE_FS_SIZE_INFORMATION	fsInfo;
	PRETRIEVAL_POINTERS_BUFFER	vcnList = NULL;
	ULONG						clusterSize = 0;
	PFILE_OBJECT				fileObj = NULL;
	KEVENT						event;
	PUCHAR						buffer = NULL;
	FILE_STANDARD_INFORMATION	hlInfo;

	InitializeObjectAttributes(&objAttr, &Context->FileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&fHandle, GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE, &objAttr, &ioCreateFile,
							NULL, 0, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto error;
	}

	status = ZwQueryInformationFile(fHandle, &ioCreateFile, &hlInfo, sizeof(hlInfo), FileStandardInformation);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto error;
	}

	if(hlInfo.NumberOfLinks > 1 || hlInfo.DeletePending == TRUE) {
		status = STATUS_SUCCESS;
		PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("ZwQueryInformationFile link %d pend %d\n", hlInfo.NumberOfLinks, hlInfo.DeletePending));
		goto error;
	}

	InitializeObjectAttributes(&volAttr, &Context->Volume, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenFile(&volHandle, GENERIC_WRITE, &volAttr, &ioCreateFile, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
	if(!NT_SUCCESS(status)) {		
		goto error;
	}

	//Получаем размер кластера
	status = ZwQueryVolumeInformationFile(volHandle, &ioCreateFile, &fsInfo, sizeof(fsInfo), FileFsSizeInformation);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto error;
	}

	clusterSize = fsInfo.BytesPerSector * fsInfo.SectorsPerAllocationUnit;
	PT_DBG_FILE("%s ClusterSize %d", __FUNCTION__, clusterSize);
	PT_DBG_FILE("%s AllocationSize %d", __FUNCTION__, Context->NetInfoFile.AllocationSize.QuadPart);

	status = SziFileEraseGetFileClusters(fHandle, &vcnList);
	if(!NT_SUCCESS(status) || vcnList == NULL) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto error;
	}

	buffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, clusterSize, DF_ERASEBUFFER_POOL_TAG);
	if(buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto error;
	}
	RtlFillMemory(buffer, clusterSize, Mask);

	status = ObReferenceObjectByHandle(volHandle, FILE_ALL_ACCESS, *IoFileObjectType, KernelMode, (PVOID*)&fileObj, 0);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto error;
	}

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	LARGE_INTEGER	lcn, startVcn = vcnList->StartingVcn;
	LARGE_INTEGER	position;
	for(ULONG index = 0; index < vcnList->ExtentCount; ++index) {
		lcn = vcnList->Extents[index].Lcn;
		// On NT 4.0, a compressed virtual run (0-filled) is identified with a cluster offset of -1
		// On the NTFS file system, the value (LONGLONG) –1 indicates either a compression unit that is partially allocated, or an unallocated region of a sparse file.
		if(lcn.QuadPart != LLINVALID) {
			//Список VCN каждая запись смещение относительно начала логического диска. В одной VCN может быть N реальных кластеров. 
			//Сколько кластеров в VCN указывает следующая запись.
			position.QuadPart = lcn.QuadPart * clusterSize;
			for(ULONG clusterCount = 0; clusterCount < (ULONG)(vcnList->Extents[index].NextVcn.QuadPart - startVcn.QuadPart); ++clusterCount) {
				// N раз посылаем IRP пакет для стирания размером с кластер
				for(UINT32 passes = 1; passes <= Count; ++passes) {
					PIRP IrpFsd = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, fileObj->DeviceObject, buffer, clusterSize, &position, &event, &ioCreateFile);
					if(!IrpFsd) {
						status = STATUS_INSUFFICIENT_RESOURCES;
						goto error;
					}

					IrpFsd->IoStatus.Status = STATUS_SUCCESS;
					status = IoCallDriver(fileObj->DeviceObject, IrpFsd);
					if(status == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);								
						status = ioCreateFile.Status;
					}
					KeClearEvent(&event);
					
					if(!NT_SUCCESS(status)) {
						PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
						goto error;
					}
				}
				position.QuadPart += clusterSize;
			}
		}
		startVcn = vcnList->Extents[index].NextVcn;
	}

error:
	if(fHandle) {
		ZwClose(fHandle);
	}
	if(volHandle) { 
		ZwClose(volHandle);
	}
	if(vcnList) {
		ExFreePoolWithTag(vcnList, DF_PRETRIEVAL_POINTERS_BUFFER);
	}
	if(fileObj) {
		ObDereferenceObject(fileObj);
	}
	if(buffer) {
		ExFreePoolWithTag(buffer, DF_ERASEBUFFER_POOL_TAG);
	}
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseFltSecureDeleteFile(
	_In_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObject,
	_In_ BOOLEAN OnClose
	)
{
	KIRQL						oldIrql;	
	UINT32						count = 0;
	UCHAR						mask = 0;
	FLT_CONTEXT_ERASEFILE		contextErase;	
	NTSTATUS					status = STATUS_UNSUCCESSFUL;	
	FLT_FILESYSTEM_TYPE			fsType = FLT_FSTYPE_UNKNOWN;
	BOOLEAN						isErase = FALSE;
	FILE_STANDARD_INFORMATION	hlInfo;

	KeAcquireSpinLock(&gUserContext.Lock, &oldIrql);
	count = gUserContext.Count;
	mask = gUserContext.Mask;
	KeReleaseSpinLock(&gUserContext.Lock, oldIrql);

	//Когда удаляем файл через Shift+Del или из консоли hlInfo.DeletePending == TRUE 
	//Для NTFS Если количество ссылок на файл >1
	//Для NTFS Если количество ссылок на файл == 0 Файл еще не создан. Хз Почему но иногда удаляется с NumberOfLinks = 0 AllocateSize = 0
	//На Fat32 Всегда NumberOfLinks = 1	
	status = FltQueryInformationFile(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, &hlInfo, sizeof(hlInfo), FileStandardInformation, NULL);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _exit;
	}

	status = FltGetFileSystemType(FltObject->Volume, &fsType);
	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
		goto _exit;
	}
	
	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s NumberOfLinks %d DeletePending %d Directory %d\n", __FUNCTION__, hlInfo.NumberOfLinks, hlInfo.DeletePending, hlInfo.Directory));
	PT_DBG_FILE("%s NumberOfLinks %d DeletePending %d Directory %d", __FUNCTION__, hlInfo.NumberOfLinks, hlInfo.DeletePending, hlInfo.Directory);

	if (fsType == FLT_FSTYPE_NTFS) {
		PT_DBG_FILE("%s FLT_FSTYPE_NTFS", __FUNCTION__);
		isErase = hlInfo.Directory ? FALSE : hlInfo.NumberOfLinks == 0 || OnClose;
	} else {
		PT_DBG_FILE("%s FLT_FSTYPE_FAT", __FUNCTION__);
		isErase	= hlInfo.Directory ? FALSE : TRUE;
	}

	PT_DBG_FILE("%s %wZ", __FUNCTION__, &Data->Iopb->TargetFileObject->FileName);
	PT_DBG_FILE("%s Erase %d", __FUNCTION__, isErase);
	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s %wZ\n",__FUNCTION__, &Data->Iopb->TargetFileObject->FileName));
	PT_DBG_PRINT(PTDBG_TRACE_VALUE, ("FileErase!%s Erase %d\n", __FUNCTION__, isErase));

	if(isErase) {
		contextErase.Instance = Data->Iopb->TargetInstance;
		contextErase.FObject = Data->Iopb->TargetFileObject;
	
		status = FltQueryInformationFile(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, &contextErase.InfoFile, sizeof(contextErase.InfoFile), FileNetworkOpenInformation, NULL);
		if(!NT_SUCCESS(status)) {
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _exit;
		}
		//Место да диске под файл не выделенно
		if (contextErase.InfoFile.AllocationSize.QuadPart == 0)	{
			status = STATUS_UNSUCCESSFUL;
			PT_DBG_FILE("Status 0x%X %s:%d", status, __FILE__, __LINE__);
			goto _exit;
		}
	
		if(contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_OFFLINE ||
			contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ||
			contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_DEVICE ) 
		{
			PT_DBG_FILE("FileErase!%s:%d Status 0x%X\n", __FILE__, __LINE__, status);
			status = STATUS_SUCCESS;
		} else if(contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			status = STATUS_SUCCESS;
		} else if(contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_COMPRESSED ||
				 contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED ||
				 contextErase.InfoFile.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) 
		{
			status = SziFileEraseFltEraseFileByBlocks(&contextErase, count, mask);
			if(status == STATUS_END_OF_FILE) {
				status = SziFileEraseFltEraseFile(&contextErase, count, mask);
			}
		} else {
			status = SziFileEraseFltEraseFile(&contextErase, count, mask);
		}
	}

_exit:
	if(!NT_SUCCESS(status)) {
		PT_DBG_PRINT( PTDBG_TRACE_VALUE, ("FileErase!%s: status 0x%X\n", __FUNCTION__, status) );
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}

NTSTATUS
SziFileEraseDeleteEmptyDir(
	_In_ POBJECT_ATTRIBUTES ObjAttr
	)
{
	FILE_DISPOSITION_INFORMATION	disInfo;
	IO_STATUS_BLOCK					ioCreateFile;
	HANDLE							fHandle = 0;
	disInfo.DeleteFile = TRUE;

	NTSTATUS status = ZwCreateFile(&fHandle, GENERIC_WRITE, ObjAttr, &ioCreateFile, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_DIRECTORY_FILE, NULL, 0);
	if(NT_SUCCESS(status)) {
		status = ZwSetInformationFile(fHandle, &ioCreateFile, &disInfo, sizeof(disInfo), FileDispositionInformation);
		ZwClose(fHandle);
	}

	if(!NT_SUCCESS(status)) {
		PT_DBG_FILE("%s Status:0x%X", __FUNCTION__, status);
	}
	return status;
}
