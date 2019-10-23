#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#include "SziFltTypes.h"
#include "SziFileErase.h"
#include "SziPrototype.h"

extern PFLT_FILTER		gFilterHandle;
extern ULONG			gTraceFlags;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SziFileEraseAllocateContext)
#pragma alloc_text(PAGE, SziFileEraseGetContext)
#pragma alloc_text(PAGE, SziFileEraseGetOrSetContext)
#pragma alloc_text(PAGE, SziFileEraseInstanceContextCleanup)
#pragma alloc_text(PAGE, SziFileEraseSetContext)
#pragma alloc_text(PAGE, SziFileEraseStreamContextCleanup)
#endif

NTSTATUS
SziFileEraseAllocateContext (
    _Outptr_ PFLT_CONTEXT *Context
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	PAGED_CODE();

	status = FltAllocateContext( gFilterHandle, FLT_STREAM_CONTEXT, sizeof(DF_STREAM_CONTEXT), PagedPool, Context );
	if (NT_SUCCESS( status )) {
	    RtlZeroMemory( *Context, sizeof(DF_STREAM_CONTEXT) );
	}
	return status;
}

NTSTATUS
SziFileEraseSetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext
    )
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    status = FltSetStreamContext( FltObjects->Instance, (PFILE_OBJECT)Target, FLT_SET_CONTEXT_KEEP_IF_EXISTS, NewContext, OldContext );
    return status;
}

NTSTATUS
SziFileEraseGetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _Outptr_ PFLT_CONTEXT *Context
    )
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();
    
	status = FltGetStreamContext( FltObjects->Instance, (PFILE_OBJECT)Target, Context );
    return status;
}

NTSTATUS
SziFileEraseGetOrSetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _Outptr_ _Pre_valid_ PFLT_CONTEXT *Context    
    )
{
    NTSTATUS		status = STATUS_UNSUCCESSFUL;
    PFLT_CONTEXT	newContext = NULL;
    PFLT_CONTEXT	oldContext = NULL;

    PAGED_CODE();

    ASSERT( NULL != Context );

    newContext = *Context;

    status = SziFileEraseGetContext( FltObjects, Target, &oldContext );
    if (STATUS_NOT_FOUND == status) {
        if (NULL == newContext) {
            status = SziFileEraseAllocateContext( &newContext );
            if (!NT_SUCCESS( status )) {				
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileErase! status 0x%X %s:%d \n", status, __FILE__, __LINE__));				
                return status;
            }
        }
    } else if (!NT_SUCCESS( status )) {
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileErase! status 0x%X %s:%d \n", status, __FILE__, __LINE__));
        return status;
    } else {

        ASSERT( newContext != oldContext );
        if (NULL != newContext) {
            FltReleaseContext( newContext );
        }

		if(!NT_SUCCESS(status)) {
			PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileErase! status 0x%X %s:%d \n", status, __FILE__, __LINE__));
		}
        *Context = oldContext;
        return status;
    }

    status = SziFileEraseSetContext( FltObjects, Target, newContext, &oldContext );
    if (!NT_SUCCESS( status )) {
        FltReleaseContext( newContext );

        if (STATUS_FLT_CONTEXT_ALREADY_DEFINED == status) {
            *Context = oldContext;
            return STATUS_SUCCESS;
        } else {
			if(!NT_SUCCESS(status)) {
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileErase! status 0x%X %s:%d \n", status, __FILE__, __LINE__));
			}
            *Context = NULL;
            return status;
        }
    }
	if(!NT_SUCCESS(status)) {
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileErase! status 0x%X %s:%d \n", status, __FILE__, __LINE__));
	}
    *Context = newContext;
    return status;
}

VOID
SziFileEraseStreamContextCleanup (
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
{
    UNREFERENCED_PARAMETER(ContextType);
	UNREFERENCED_PARAMETER(StreamContext);

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("FileErase!%s: Entered\n", __FUNCTION__) );

    PAGED_CODE();

	if(NULL != StreamContext) {
		FltDeleteContext(StreamContext);
	}
}