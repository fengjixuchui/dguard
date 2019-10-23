#pragma once

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
#define PTDBG_TRACE_ERRORS				0x00000004
#define PTDBG_TRACE_VALUE				0x00000008
#define PTDBG_TRACE_FILE				0x00000010

#define PT_DBG_PRINT( _dbgLevel, _string )          \
	(FlagOn(gTraceFlags, (_dbgLevel)) ? \
	DbgPrint _string : \
	((int)0))

#define PT_DBG_FILE(_string, ...)				\
	(FlagOn(gTraceFlags, PTDBG_TRACE_FILE) ? \
	CommonLogWriteV(gUserContext.Log, _string, __VA_ARGS__) : \
	((int)0))

#define FlagOnAll( F, T )                           \
	(FlagOn(F, T) == T)

#define LIST_FOR_EACH(_Link, _Head)	\
	for(_Link = _Head.Flink;		\
		_Link != &_Head;			\
		_Link = _Link->Flink)		\

#define DF_INSTANCE_CONTEXT_POOL_TAG    'nIfS'			//SfIn
#define DF_STREAM_CONTEXT_POOL_TAG      'xSfS'			//SfSx
#define DF_TRANSACTION_CONTEXT_POOL_TAG 'xTfS'			//SfTx
#define DF_ERESOURCE_POOL_TAG           'sRfS'			//SfRs
#define DF_DELETE_NOTIFY_POOL_TAG       'nDfS'			//SfDn
#define DF_STRING_POOL_TAG              'rSfS'			//SfSr
#define DF_ERASEBUFFER_POOL_TAG         'eFfS'			//SfFe
#define DF_SECURE_WORKITEM_CONTEXT		'cwfS'			//Sfwc
#define DF_PRETRIEVAL_POINTERS_BUFFER   'bpfS'			//Sfpb

#define DF_VOLUME_GUID_NAME_SIZE        48

#define SziFileEraseSizeofFileId(FID) (               \
	((FID).FileId64.UpperZeroes == 0ll) ? \
	sizeof((FID).FileId64.Value) : \
	sizeof((FID).FileId128)             \
	)

#define LLINVALID		((LONGLONG) -1)

#define REGVALUE_FILERIGHTS	 L"FileRights"
#define REGVALUE_DEBUGFLAGS	 L"DebugFlags"
#define REGVALUE_LOGFILE	 L"FileLog"
#define REGVALUE_AUTOERASE	 L"AutoErase"