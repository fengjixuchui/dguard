#pragma once

#include "SziLogging.h"

typedef union _DF_FILE_REFERENCE {

	struct {
		ULONGLONG   Value;          //  The 64-bit file ID lives here.
		ULONGLONG   UpperZeroes;    //  In a 64-bit file ID this will be 0.
	} FileId64;

	UCHAR           FileId128[16];  //  The 128-bit file ID lives here.

} DF_FILE_REFERENCE, *PDF_FILE_REFERENCE;

typedef struct _DF_INSTANCE_CONTEXT {
	UNICODE_STRING VolumeGuidName;
} DF_INSTANCE_CONTEXT, *PDF_INSTANCE_CONTEXT;

typedef struct _DF_STREAM_CONTEXT {
	volatile LONG               NumOps;
	volatile LONG               Erase;
	volatile LONG               IsErase;
	BOOLEAN                     DeleteOnClose;
} DF_STREAM_CONTEXT, *PDF_STREAM_CONTEXT;

typedef struct _DF_DELETE_NOTIFY {
	LIST_ENTRY Links;
	PDF_STREAM_CONTEXT StreamContext;
} DF_DELETE_NOTIFY, *PDF_DELETE_NOTIFY;

typedef struct _FLT_CONTEXT_ERASEFILE {
	FILE_NETWORK_OPEN_INFORMATION	InfoFile;
	PFLT_INSTANCE					Instance;
	PFILE_OBJECT					FObject;
} FLT_CONTEXT_ERASEFILE, *PFLT_CONTEXT_ERASEFILE;

#define MAX_PATH		((260 + 5) * sizeof(WCHAR))			//5 для UNI имени

typedef struct _SECURE_WORKITEM_CONTEXT {
	WCHAR			FileName[MAX_PATH];
	SIZE_T			Length;
	PKEVENT			Event;
	PIO_WORKITEM	WorkItem;
} SECURE_WORKITEM_CONTEXT, *PSECURE_WORKITEM_CONTEXT;

#define MAX_VOLUMNNAME	8

typedef struct _CONTEXT_ERASEFILE {
	FILE_NETWORK_OPEN_INFORMATION	NetInfoFile;
	UNICODE_STRING		FileName;
	UNICODE_STRING		Volume;
} CONTEXT_ERASEFILE, *PCONTEXT_ERASEFILE;

typedef struct _USER_CONTEXT_EXT  {
	volatile LONG	AutoErase;
	UINT32			Count;
	UCHAR			Mask;
	NTSTATUS		LastStatus;
	KSPIN_LOCK		Lock;
	PSZI_LOGGING	Log;
	UNICODE_STRING	RegistryPath;
} USER_CONTEXT_EXT, *PUSER_CONTEXT_EXT;
