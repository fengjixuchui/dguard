//
// Author:
//		Burlutsky Stas
//
//		burluckij@gmail.com
//

#include "../flock.h"
#include "../FLockStorage.h"
#include "../FLockCache.h"
#include "../FLockMd5.h"



#define FLOCK_FIELD_READ(_base, _offset, _fieldType)				(   *( (_fieldType*)( ((PUCHAR)_base) + _offset) )   )
#define FLOCK_FIELD_PTR(_base, _offset, _fieldType)					(   ( (_fieldType*)( ((PUCHAR)_base) + _offset) )   )
#define FLOCK_WRITE_FIELD(_base, _offset, _fieldType, _value)		(   *((_fieldType*)(((PUCHAR)_base) + _offset)) = _value  )



extern ULONG gTraceFlags;
extern FLOCK_DEVICE_DATA g_flockData;



NTSTATUS
FLockHandleFileBothDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileFullDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileIdBothDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileIdFullDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileNamesInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileObjectIdInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);


NTSTATUS
FLockHandleFileReparsePointInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
);



FLT_PREOP_CALLBACK_STATUS FLockPreDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA    Data,
	_In_    PCFLT_RELATED_OBJECTS FltObjects,
	_Out_   PVOID                 *CompletionContext
	)
{
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);

	NTSTATUS status;
	BOOLEAN hasFlockMeta = FALSE, requireReadMeta = TRUE;
	UNICODE_STRING path = { 0 }, filepath = { 0 };
	FLOCK_META fm = { 0 };

	if (FLT_IS_FASTIO_OPERATION(Data))
	{
		return FLT_PREOP_DISALLOW_FASTIO;
	}

	if (BooleanFlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (BooleanFlagOn(FltObjects->FileObject->Flags, FO_VOLUME_OPEN))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: info - this is a volume open operation.", __FUNCTION__));
	}

	if (!FLockStorageHasHiddenUserObjects())
	{
		// There is no one object which should be hidden.
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// 	if (IoGetTopLevelIrp() == FSRTL_FSP_TOP_LEVEL_IRP)
	// 	{
	// 		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	// 	}

	if (Data->Iopb->TargetFileObject)
	{
		if (FsRtlIsPagingFile(Data->Iopb->TargetFileObject))
		{
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}
	}
	else
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: warning it's no FILE_OBJECT.\n", __FUNCTION__));
	}

	if (!FLT_IS_IRP_OPERATION(Data))
	{
		//PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: it is not IRP based operation.\n", __FUNCTION__));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: ignore request which is not (IRP_MN_QUERY_DIRECTORY), MinorFunction is 0x%x.\n", __FUNCTION__, Data->Iopb->MinorFunction));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	//
	// If cache enabled, we should handle this request through cache for improving hole system performance.
	//
	if ( FLockCacheIsEnable() )
	{
		UCHAR hashByFilePath[16];		
		BOOLEAN hasFilePath = FLockFltGetPath(g_flockData.filterHandle, Data->Iopb->TargetInstance, Data, &path, &status);
		
		if (hasFilePath)
		{
			// Calculate hash if we have file path.
			//
			if (path.Length)
			{
				FLockMd5Calc(path.Buffer, path.Length, hashByFilePath);

#pragma region WORK_WITH_CACHE

				FLockCacheLock();

				ULONG stepsRequired = 0;
				FLOCK_CACHE_ENTRY fce = { 0 };
				if (FLockCacheLookup(hashByFilePath, &fce, &stepsRequired))
				{
					if (fce.presentMeta == FALSE)
					{
						PT_DBG_PRINT(PTGBG_FLOCK_CACHE, ("FLock!%s: cache_strike. Ignore reading EAs. Cached entry found, required to do %d steps.\n", __FUNCTION__, stepsRequired));

						// It is not require to read FLock-meta information form disk.
						//
						requireReadMeta = FALSE;
					}
					else
					{
						PT_DBG_PRINT(PTGBG_FLOCK_CACHE, ("FLock!%s: cache_strike. Cache entry found - read EAs! It took %d steps.\n", __FUNCTION__, stepsRequired));
					}
				}

				FLockCacheUnlock();

#pragma endregion WORK_WITH_CACHE

			}

			ExFreePool(path.Buffer);
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - couldn't find file path for the opening file.\n", __FUNCTION__));
		}
	}

	if (requireReadMeta)
	{
		//
		// Path is c:\work\dir
		// Нужно ли нам скрывать что-то для этого каталога? 
		// Он может быть родительским для целевого, скрываемого нами каталога - c:\work\dir\hidden
		//

		if (Data->Iopb->TargetFileObject)
		{
			//
			// Read flock-meta avoiding opening file operation using earlier created FILE_OBJECT.
			//

			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: call for Data->Iopb->TargetFileObject.\n", __FUNCTION__));

			hasFlockMeta = FLockFltReadFirstMeta(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, &fm, &status);
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: open and read info.\n", __FUNCTION__));

			hasFlockMeta = FLockFltReadFirstMetaWithGetFilePath(
				g_flockData.filterHandle,
				Data->Iopb->TargetInstance,
				Data,
				&fm,
				&filepath,
				&status);
		}

		if (hasFlockMeta)
		{
			FLockPrintMeta(&fm);

			//
			// Requested directory has something to protect.
			//
			if (fm.flags & FLOCK_FLAG_HAS_FLOCKS)
			{
				if (filepath.Length)
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: there is something to protect in %wZ\n", __FUNCTION__, &filepath));

					ExFreePool(filepath.Buffer);
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: there is something to protect in the directory.\n", __FUNCTION__));
				}

				//
				// Hide target objects in post-callback.
				//
				return FLT_PREOP_SYNCHRONIZE;
			}
		}
		else
		{
			PT_DBG_PRINT(PTDBG_TRACE_ERRORS, ("FLock!%s: error - can't read flock-meta, status is 0x%x\n", __FUNCTION__, status));
		}
	}
	
	//
	// There is nothing to hide in requested directory.
	//
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS FLockPostDirectoryControl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (!NT_SUCCESS(Data->IoStatus.Status) || Data->IoStatus.Status == STATUS_REPARSE)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (IoGetTopLevelIrp() == FSRTL_FSP_TOP_LEVEL_IRP)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (FsRtlIsPagingFile(Data->Iopb->TargetFileObject))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("%s it is page file opening. Ignore.\n", __FUNCTION__));
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: (IRP_MN_QUERY_DIRECTORY is 0x01), MinorFunction is 0x%x.\n", __FUNCTION__, Data->Iopb->MinorFunction));

	if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: ignore request which is not (IRP_MN_QUERY_DIRECTORY = 0x01), MinorFunction is 0x%x.\n", __FUNCTION__, Data->Iopb->MinorFunction));
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: ready to process the request.\n", __FUNCTION__));

	//
	// Lookup flock-meta information.
	//
	
	UNICODE_STRING requestedDirPath = { 0 };
	BOOLEAN result = FLockFltGetPath(g_flockData.filterHandle, Data->Iopb->TargetInstance, Data, &requestedDirPath, &status);

	if (!result)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: error - can't get requested directory path.\n", __FUNCTION__));
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	FILE_INFORMATION_CLASS infoClass = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
	
	switch (infoClass)
	{

	case FileDirectoryInformation:
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: requested 'FileDirectoryInformation'\n", __FUNCTION__));
		status = FLockHandleFileDirectoryInformation(Data, FltObjects, &requestedDirPath);
		break;

	case FileIdFullDirectoryInformation:
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: requested 'FileIdFullDirectoryInformation'\n", __FUNCTION__));
		status = FLockHandleFileIdFullDirectoryInformation(Data, FltObjects, &requestedDirPath);
		break;

	case FileNamesInformation:
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: requested 'FileNamesInformation'\n", __FUNCTION__));
		status = FLockHandleFileNamesInformation(Data, FltObjects, &requestedDirPath);
		break;

	case FileObjectIdInformation:
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: requested 'FileObjectIdInformation'\n", __FUNCTION__));
		break;

	case FileReparsePointInformation:
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FLock!%s: requested 'FileReparsePointInformation'\n", __FUNCTION__));
		break;

	default:
		status = STATUS_UNSUCCESSFUL;
		break;
	}

	//
	// Проверить extended attributes и удалить объект из списка, если требуется.
	//

	if (requestedDirPath.Buffer){
		ExFreePool(requestedDirPath.Buffer);
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}


NTSTATUS
FLockHandleByPath2(
_Inout_ PFLT_CALLBACK_DATA Data,
__in PUNICODE_STRING _requestedDir,
__in ULONG _offsetNextEntry,
__in ULONG _offsetFileNameLength,
__in ULONG _offsetFileName,
__in ULONG _sizeOfStruct,
__in ULONG _offsetShortFileName // could be or couldn't be - depends from structure.
)
{
	return STATUS_SUCCESS;
}

//
// All functions placed below - does the same like FLockHandleFileIdBothDirectoryInformation(..)
//

//
// I'll change all that code to something common and will move it somewhere.
//

NTSTATUS
FLockHandleFileIdBothDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);

	const ULONG offsetNextEntry = OFFSET_OF(FILE_ID_BOTH_DIR_INFORMATION, NextEntryOffset);
	const ULONG offsetFileNameLength = OFFSET_OF(FILE_ID_BOTH_DIR_INFORMATION, FileNameLength);
	const ULONG offsetFileName = OFFSET_OF(FILE_ID_BOTH_DIR_INFORMATION, FileName);
	const ULONG sizeOfStructure = sizeof(FILE_ID_BOTH_DIR_INFORMATION);
	const ULONG offsetShortName = OFFSET_OF(FILE_ID_BOTH_DIR_INFORMATION, ShortName);

	return FLockHandleByPath2(Data, _requestedDir, offsetNextEntry, offsetFileNameLength, offsetFileName, sizeOfStructure, offsetShortName);
}

NTSTATUS
FLockHandleFileBothDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);

	const ULONG offsetNextEntry = OFFSET_OF(FILE_BOTH_DIR_INFORMATION, NextEntryOffset);
	const ULONG offsetFileNameLength = OFFSET_OF(FILE_BOTH_DIR_INFORMATION, FileNameLength);
	const ULONG offsetFileName = OFFSET_OF(FILE_BOTH_DIR_INFORMATION, FileName);
	const ULONG sizeOfStructure = sizeof(FILE_BOTH_DIR_INFORMATION);
	const ULONG offsetShortName = OFFSET_OF(FILE_BOTH_DIR_INFORMATION, ShortName);

	return FLockHandleByPath2(Data, _requestedDir, offsetNextEntry, offsetFileNameLength, offsetFileName, sizeOfStructure, offsetShortName);
}


NTSTATUS
FLockHandleFileDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);

	const ULONG offsetNextEntry = OFFSET_OF(FILE_DIRECTORY_INFORMATION, NextEntryOffset);
	const ULONG offsetFileNameLength = OFFSET_OF(FILE_DIRECTORY_INFORMATION, FileNameLength);
	const ULONG offsetFileName = OFFSET_OF(FILE_DIRECTORY_INFORMATION, FileName);
	const ULONG sizeOfStructure = sizeof(FILE_DIRECTORY_INFORMATION);

	return FLockHandleByPath2(Data, _requestedDir, offsetNextEntry, offsetFileNameLength, offsetFileName, sizeOfStructure, 0);
}


NTSTATUS
FLockHandleFileFullDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);

	const ULONG offsetNextEntry = OFFSET_OF(FILE_FULL_DIR_INFORMATION, NextEntryOffset);
	const ULONG offsetFileNameLength = OFFSET_OF(FILE_FULL_DIR_INFORMATION, FileNameLength);
	const ULONG offsetFileName = OFFSET_OF(FILE_FULL_DIR_INFORMATION, FileName);
	const ULONG sizeOfStructure = sizeof(FILE_FULL_DIR_INFORMATION);

	return FLockHandleByPath2(Data, _requestedDir, offsetNextEntry, offsetFileNameLength, offsetFileName, sizeOfStructure, 0);
}


NTSTATUS
FLockHandleFileIdFullDirectoryInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);

	const ULONG offsetNextEntry = OFFSET_OF(FILE_ID_FULL_DIR_INFORMATION, NextEntryOffset);
	const ULONG offsetFileNameLength = OFFSET_OF(FILE_ID_FULL_DIR_INFORMATION, FileNameLength);
	const ULONG offsetFileName = OFFSET_OF(FILE_ID_FULL_DIR_INFORMATION, FileName);
	const ULONG sizeOfStructure = sizeof(FILE_ID_FULL_DIR_INFORMATION);

	return FLockHandleByPath2(Data, _requestedDir, offsetNextEntry, offsetFileNameLength, offsetFileName, sizeOfStructure, 0);
}


NTSTATUS
FLockHandleFileNamesInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);

	const ULONG offsetNextEntry = OFFSET_OF(FILE_NAMES_INFORMATION, NextEntryOffset);
	const ULONG offsetFileNameLength = OFFSET_OF(FILE_NAMES_INFORMATION, FileNameLength);
	const ULONG offsetFileName = OFFSET_OF(FILE_NAMES_INFORMATION, FileName);
	const ULONG sizeOfStructure = sizeof(FILE_NAMES_INFORMATION);

	return FLockHandleByPath2(Data, _requestedDir, offsetNextEntry, offsetFileNameLength, offsetFileName, sizeOfStructure, 0);
}

NTSTATUS
FLockHandleFileObjectIdInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(_requestedDir);

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
FLockHandleFileReparsePointInformation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
__in PUNICODE_STRING _requestedDir
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(_requestedDir);

	return STATUS_NOT_IMPLEMENTED;
}
