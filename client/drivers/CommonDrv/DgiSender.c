#include <DgiSender.h>

#define WSK_CAPTURE_WAIT_TIMEOUT_MSEC	1000		//1 sec
#define CF_SENDERCONTEXT_TAG			'lEzS'		//SzEl

CONST UCHAR gBase64Chars[] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

typedef struct _DGI_SENDER_CONTEXT
{
	KEVENT		Event;
	PIRP		Irp;	
} DGI_SENDER_CONTEXT, *PDGI_SENDER_CONTEXT;


typedef struct _DGI_IRP_REQUEST
{
	KEVENT			Event;
	PIRP			Irp;
} DGI_IRP_REQUEST, *PDGI_IRP_REQUEST;


NTSTATUS
DgiInitOperationContext();

VOID
DgiFreeOperationContext();

VOID
DgiReuseOperationContext();

NTSTATUS
DgiWaitSocketOperation(
	_In_ NTSTATUS
	);

VOID
DgiReuseForCloseOperationContext();

IO_COMPLETION_ROUTINE DgiSyncIrpCompletionRoutine;
IO_COMPLETION_ROUTINE DgiSyncIrpCompletionCloseRoutine;
	

WSK_CLIENT_NPI				gClientNpi = {0};
WSK_REGISTRATION			gWskRegistration;
WSK_CLIENT_DISPATCH			gWskDispatch = {MAKE_WSK_VERSION(1,0), 0, NULL};
WSK_PROVIDER_NPI			gWskProvider;
PDGI_SENDER_CONTEXT			gContext = NULL;
BOOLEAN						gIsInit = FALSE;

NTSTATUS
DgiSenderInit() 
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (gIsInit == FALSE) {
		gClientNpi.ClientContext = NULL;
		gClientNpi.Dispatch = &gWskDispatch;

		RtlZeroMemory(&gWskRegistration, sizeof(gWskRegistration));
		RtlZeroMemory(&gWskProvider, sizeof(gWskProvider));

		status = WskRegister(&gClientNpi, &gWskRegistration);
		if(!NT_SUCCESS(status)) {
			DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			goto _error;
		}

		status = WskCaptureProviderNPI(&gWskRegistration, WSK_CAPTURE_WAIT_TIMEOUT_MSEC, &gWskProvider);
		if(!NT_SUCCESS(status)) {
			DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			WskDeregister(&gWskRegistration);
			goto _error;
		}

		status = DgiInitOperationContext();
		if(!NT_SUCCESS(status)) {
			DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			WskDeregister(&gWskRegistration);
			goto _error;
		}
		gIsInit = TRUE;
	} else {
		status = STATUS_SUCCESS;
	}

_error:

	return status;
}

VOID
DgiSenderDestroy()
{
	if (gIsInit) {
		WskReleaseProviderNPI(&gWskRegistration);
		DgiFreeOperationContext();
		WskDeregister(&gWskRegistration);
		gIsInit = FALSE;
	}	
}

BOOLEAN
DgiSenderIsInit()
{
	return gIsInit;
}

NTSTATUS
DgiSenderConnect(
	_In_ USHORT Port,
	_In_ PWSK_SOCKET * Socket
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	SOCKADDR_IN localAddress = { AF_INET, 0, IN4ADDR_ANY_INIT };
	SOCKADDR_IN remoteAddress = { AF_INET, 0, IN4ADDR_LOOPBACK_INIT };
	remoteAddress.sin_port = RtlUshortByteSwap(Port);

	DgiReuseOperationContext();
	status = gWskProvider.Dispatch->WskSocketConnect(gWskProvider.Client, SOCK_STREAM, IPPROTO_TCP, (PSOCKADDR)&localAddress, (PSOCKADDR)&remoteAddress,
													 WSK_FLAG_CONNECTION_SOCKET, NULL, NULL, NULL, NULL, NULL, gContext->Irp);
	status = DgiWaitSocketOperation(status);
	if(!NT_SUCCESS(status)) {
		DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	*Socket = (PWSK_SOCKET)gContext->Irp->IoStatus.Information;

_error:

	return status;
}

VOID
DgiSenderDisconnect(
	_In_ PWSK_SOCKET Socket
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (Socket != NULL) {
		PWSK_PROVIDER_CONNECTION_DISPATCH socketDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)Socket->Dispatch;

		//SziReuseOperationContext();
		//status = socketDispatch->WskSend(Socket, NULL, WSK_FLAG_NODELAY, gContext->Irp);
		//status = SziWaitSocketOperation(status);
		//
		//if(!NT_SUCCESS(status)) {
		//	DbgPrint("%s:%d 0x%X\n", __FILE__, __LINE__, status);
		//}
		//
		//TODO: WskDisconnect вызывет BSOD если делать только конект и не посылать данные.

		//SziReuseForCloseOperationContext();
		DgiReuseOperationContext();
		status = socketDispatch->WskDisconnect(Socket, NULL, WSK_FLAG_ABORTIVE, gContext->Irp);
		status = DgiWaitSocketOperation(status);
		if(!NT_SUCCESS(status)) {
			DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		}

		DgiReuseForCloseOperationContext();
		status = socketDispatch->WskCloseSocket(Socket, gContext->Irp);
		status = DgiWaitSocketOperation(status);
		if(!NT_SUCCESS(status)) {
			DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		}
	}	
}

NTSTATUS
DgiSenderWrite(
	_In_ PWSK_SOCKET Socket,
	_In_ PUCHAR Data,
	_In_ SIZE_T Length
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	PWSK_PROVIDER_CONNECTION_DISPATCH socketDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)Socket->Dispatch;
	WSK_BUF		bufferWsk;

	bufferWsk.Offset = 0;
	bufferWsk.Length = Length;
	bufferWsk.Mdl = IoAllocateMdl(Data, (ULONG)Length, FALSE, FALSE, NULL);
		
	if(bufferWsk.Mdl != NULL) {
		MmBuildMdlForNonPagedPool(bufferWsk.Mdl);
		//MmProbeAndLockPages
		for(;;)
		{
			DgiReuseOperationContext();
			status = socketDispatch->WskSend(Socket, &bufferWsk, WSK_FLAG_NODELAY, gContext->Irp);
			status = DgiWaitSocketOperation(status);

			if(!NT_SUCCESS(status)) {
				IoFreeMdl(bufferWsk.Mdl);
				goto _error;
			}
			
			ULONG byteSent = (ULONG)gContext->Irp->IoStatus.Information;
			if(byteSent < bufferWsk.Length) {
				bufferWsk.Offset += byteSent;
				continue;
			} else
				break;
		}
		
		IoFreeMdl(bufferWsk.Mdl);
	} else {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}	

_error:
		
	return status;
}

//NTSTATUS
//SziSenderRead(
//	_In_ PWSK_SOCKET Socket,
//	_In_ PLARGE_INTEGER TimeOut,
//	_Out_ PUCHAR Data,
//	_In_ SIZE_T Length
//	)
//{
//	NTSTATUS	status = STATUS_UNSUCCESSFUL;
//	PWSK_PROVIDER_CONNECTION_DISPATCH socketDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)Socket->Dispatch;
//	WSK_BUF		bufferWsk;
//
//	bufferWsk.Offset = 0;
//	bufferWsk.Length = Length;
//	bufferWsk.Mdl = IoAllocateMdl(Data, (ULONG)Length, FALSE, FALSE, NULL);
//
//	if(bufferWsk.Mdl != NULL) {
//		MmBuildMdlForNonPagedPool(bufferWsk.Mdl);
//		//MmProbeAndLockPages
//		for(;;) {
//			SziReuseOperationContext();
//			status = socketDispatch->WskReceive(Socket, &bufferWsk, 0, gContext->Irp);
//			status = SziWaitSocketOperationTimeOut(status, TimeOut);
//
//			if(!NT_SUCCESS(status)) {
//				break;
//			}
//
//			ULONG byteRead = (ULONG)gContext->Irp->IoStatus.Information;
//			if(byteRead < bufferWsk.Length) {
//				bufferWsk.Offset += byteRead;
//				continue;
//			} else
//				break;
//		}
//
//		IoFreeMdl(bufferWsk.Mdl);
//	} else {
//		status = STATUS_INSUFFICIENT_RESOURCES;
//	}
//
//	return status;
//}

NTSTATUS
DgiSyncIrpCompletionRoutine(
	__in PDEVICE_OBJECT Reserved,
	__in PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
	) 
{
	UNREFERENCED_PARAMETER(Reserved);
	UNREFERENCED_PARAMETER(Irp);

	ASSERT(Context != NULL);

	PKEVENT pEvent = (PKEVENT)Context;
	if (pEvent != NULL) {
		KeSetEvent(pEvent, 2, FALSE);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
DgiSyncIrpCompletionCloseRoutine(
	__in PDEVICE_OBJECT Reserved,
	__in PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
	) 
{
	UNREFERENCED_PARAMETER(Reserved);
	UNREFERENCED_PARAMETER(Irp);

	ASSERT(Context != NULL);
	
	PKEVENT pEvent = (PKEVENT)Context;
	if (pEvent != NULL)	{
		KeSetEvent(pEvent, 0, FALSE);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	return STATUS_SUCCESS;
}

NTSTATUS 
DgiInitOperationContext(VOID)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (gContext != NULL) {
		goto _error;
	}

	gContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(DGI_SENDER_CONTEXT), CF_SENDERCONTEXT_TAG);
	if (gContext == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}

	gContext->Irp = IoAllocateIrp(1, FALSE);
	if(gContext->Irp == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}

	KeInitializeEvent(&gContext->Event, SynchronizationEvent, FALSE);
	status = STATUS_SUCCESS;

	goto _exit;
_error:
	if (gContext != NULL) {
		ExFreePoolWithTag(gContext, CF_SENDERCONTEXT_TAG);
		gContext = NULL;
	}
_exit:

	return status;
}

VOID 
DgiFreeOperationContext(VOID)
{
	if(gContext != NULL) {
		IoFreeIrp(gContext->Irp);
		ExFreePoolWithTag(gContext, CF_SENDERCONTEXT_TAG);
		gContext = NULL;
	}	
}

VOID 
DgiReuseOperationContext()
{
	IoReuseIrp(gContext->Irp, STATUS_UNSUCCESSFUL);
	IoSetCompletionRoutine(gContext->Irp, DgiSyncIrpCompletionRoutine, &gContext->Event, TRUE, TRUE, TRUE);
	KeResetEvent(&gContext->Event);
}

VOID
DgiReuseForCloseOperationContext() 
{
	IoReuseIrp(gContext->Irp, STATUS_UNSUCCESSFUL);
	IoSetCompletionRoutine(gContext->Irp, DgiSyncIrpCompletionCloseRoutine, &gContext->Event, TRUE, TRUE, TRUE);
	KeResetEvent(&gContext->Event);
}

NTSTATUS
DgiWaitSocketOperation(
	_In_ NTSTATUS status
	)
{
	if(status == STATUS_PENDING) {
		KeWaitForSingleObject(&gContext->Event, Executive, KernelMode, FALSE, NULL);
		status = gContext->Irp->IoStatus.Status;
	}
	return status;
}

//NTSTATUS
//SziWaitSocketOperationTimeOut(
//	_In_ NTSTATUS Status,
//	_In_ PLARGE_INTEGER TimeOut
//	) 
//{
//	NTSTATUS status = STATUS_UNSUCCESSFUL;
//
//	if(Status == STATUS_PENDING) {
//		status = KeWaitForSingleObject(&gContext->Event, Executive, KernelMode, FALSE, TimeOut);
//		if (STATUS_TIMEOUT == status) {
//			IoCancelIrp(gContext->Irp);
//			status = KeWaitForSingleObject(&gContext->Event, Executive, KernelMode, FALSE, NULL);
//			if (!NT_SUCCESS(status)) {
//				DbgPrint("DgiSender!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
//			}
//			//return STATUS_TIMEOUT;
//		}
//		Status = gContext->Irp->IoStatus.Status;
//	}
//	return Status;
//}

NTSTATUS
DgiBase64Encode(
	_In_ UCHAR const* BytesEncode,
	_In_ size_t SizeEncode,
	_Inout_ PUCHAR ByteOut,
	_In_ size_t SizeOut,
	_Out_ size_t* Size
	)
{	
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	size_t offset = 0;

	while(SizeEncode--) {
		char_array_3[i++] = *(BytesEncode++);
		if(i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i < 4); i++) {
				ByteOut[offset] = gBase64Chars[char_array_4[i]];
				offset++;
				if (SizeOut < offset) {
					return STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			i = 0;
		}
	}

	if(i) {
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for(j = 0; (j < i + 1); j++) {
			ByteOut[offset] = gBase64Chars[char_array_4[j]];
			offset++;
			if(SizeOut < offset) {
				return STATUS_INSUFFICIENT_RESOURCES;
			}
		}
		
		while((i++ < 3)) {
			ByteOut[offset] = '=';
			offset++;
			if(SizeOut < offset) {
				return STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	}
	*Size = offset;
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

VOID DgiSocketSyncIrpCancelRoutine(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP Irp
	);

NTSTATUS
DgiSocketSyncIrpCompletionRoutine(
	__in PDEVICE_OBJECT Reserved,
	__in PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
	);

NTSTATUS DgiSocketDisconnectEvent(
	_In_opt_ PVOID SocketContext,
	_In_     ULONG Flags
	);

const WSK_CLIENT_CONNECTION_DISPATCH DgiSocketConnectDispatch = {
	NULL,
	DgiSocketDisconnectEvent,
	NULL,
};

NTSTATUS
DgiSocketInitIrp(
	_In_ PDGI_IRP_REQUEST* Request,
	_In_ BOOLEAN Cancel
	)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	PDGI_IRP_REQUEST	request = NULL;
	KIRQL				irql;

	request = ExAllocatePoolWithTag(NonPagedPool, sizeof(DGI_IRP_REQUEST), CF_SENDERCONTEXT_TAG);
	if(request == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}
	RtlZeroMemory(request, sizeof(DGI_IRP_REQUEST));

	request->Irp = IoAllocateIrp(1, FALSE);
	if(request->Irp == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}
	
	KeInitializeEvent(&request->Event, SynchronizationEvent, FALSE);
	IoSetCompletionRoutine(request->Irp, DgiSocketSyncIrpCompletionRoutine, &request->Event, TRUE, TRUE, TRUE);
	
	if (Cancel)	{
		IoAcquireCancelSpinLock(&irql);
		IoSetCancelRoutine(request->Irp, DgiSocketSyncIrpCancelRoutine);
		IoReleaseCancelSpinLock(irql);
	}
	
	*Request = request;
	status = STATUS_SUCCESS;
_error:
	if (!NT_SUCCESS(status) && request != NULL) {
		ExFreePoolWithTag(request, CF_SENDERCONTEXT_TAG);
	}
	return status;
}

VOID
DgiSocketReuseIrp(
	_In_ PDGI_IRP_REQUEST Request
	)
{
	KIRQL	irql;

	KeResetEvent(&Request->Event);
	IoReuseIrp(Request->Irp, STATUS_UNSUCCESSFUL);
	IoSetCompletionRoutine(Request->Irp, DgiSocketSyncIrpCompletionRoutine, &Request->Event, TRUE, TRUE, TRUE);

	IoAcquireCancelSpinLock(&irql);
	if (Request->Irp->CancelRoutine == NULL) {
		IoSetCancelRoutine(Request->Irp, DgiSocketSyncIrpCancelRoutine);
	}
	IoReleaseCancelSpinLock(irql);
}

VOID
DgiSocketFreeIrp(
	_In_ PDGI_IRP_REQUEST Request
	)
{
	if (Request != NULL) {
		IoFreeIrp(Request->Irp);
		ExFreePoolWithTag(Request, CF_SENDERCONTEXT_TAG);
	}
}

NTSTATUS
DgiSocketInitContext(
	_Inout_ PSZI_SOCKET_CONTEXT * Context,
	_In_ ULONG Timeout
	)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	PSZI_SOCKET_CONTEXT context = NULL;
	BOOLEAN				wskReg = FALSE;
	WSK_EVENT_CALLBACK_CONTROL callbackControl = {(PNPIID)&NPI_WSK_INTERFACE_ID, WSK_EVENT_DISCONNECT};

	context = ExAllocatePoolWithTag(NonPagedPool, sizeof(SZI_SOCKET_CONTEXT), CF_SENDERCONTEXT_TAG);
	if(context == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(context, sizeof(SZI_SOCKET_CONTEXT));
	
	context->SocketDispatch.Version = MAKE_WSK_VERSION(1, 0);
	context->SocketClientNpi.Dispatch = &context->SocketDispatch;
	context->TimeOut = Timeout;

	status = WskRegister(&context->SocketClientNpi, &context->SocketRegistration);
	if(!NT_SUCCESS(status)) {
		DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	wskReg = TRUE;

	status = WskCaptureProviderNPI(&context->SocketRegistration, WSK_CAPTURE_WAIT_TIMEOUT_MSEC, &context->SocketProviderNpi);
	if(!NT_SUCCESS(status)) {
		DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);		
		goto _error;
	}
	
	status = context->SocketProviderNpi.Dispatch->WskControlClient(context->SocketProviderNpi.Client, WSK_SET_STATIC_EVENT_CALLBACKS,
																	sizeof(callbackControl), &callbackControl, 0, NULL,	NULL, NULL);
	if(!NT_SUCCESS(status)) {
		DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);	
		goto _error;
	}

	*Context = context;
	status = STATUS_SUCCESS;

_error:
	if(!NT_SUCCESS(status) && wskReg == TRUE) {
		WskDeregister(&context->SocketRegistration);
	}
	if(!NT_SUCCESS(status) && context != NULL) {
		ExFreePoolWithTag(context, CF_SENDERCONTEXT_TAG);
	}
	
	return status;
}

NTSTATUS
DgiSocketSyncIrpCompletionRoutine(
	__in PDEVICE_OBJECT Reserved,
	__in PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
	) 
{
	UNREFERENCED_PARAMETER(Reserved);
	UNREFERENCED_PARAMETER(Irp);

	if(Context != NULL) {
		KeSetEvent((PKEVENT)Context, IO_NETWORK_INCREMENT, FALSE);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	return STATUS_SUCCESS;
}

VOID
DgiSocketDestroyContext(
	_In_ PSZI_SOCKET_CONTEXT Context
	) 
{
	if(Context != NULL) {
		WskReleaseProviderNPI(&Context->SocketRegistration);		
		WskDeregister(&Context->SocketRegistration);
		ExFreePoolWithTag(Context, CF_SENDERCONTEXT_TAG);
	}
}

VOID DgiSocketSyncIrpCancelRoutine(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP Irp
	) 
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	IoReleaseCancelSpinLock(Irp->CancelIrql);
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
SziSocketWaitSocketOperation(
	_In_ NTSTATUS Status,
	_In_ PDGI_IRP_REQUEST Request,
	_In_ PSZI_SOCKET_CONTEXT Context,
	_In_ BOOLEAN Closing
	)
{
	NTSTATUS		statusRet = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER	waitTime;
	waitTime.QuadPart = (-1 * 10000 * (LONG)Context->TimeOut);

	if(STATUS_PENDING == Status) {
		statusRet = KeWaitForSingleObject(&Request->Event, Executive, KernelMode, FALSE, &waitTime);
		if(STATUS_TIMEOUT == statusRet && Closing == TRUE) {
			IoCancelIrp(Request->Irp);
			KeWaitForSingleObject(&Request->Event, Executive, KernelMode, FALSE, NULL);			
		}
		statusRet = Request->Irp->IoStatus.Status;
	} else {
		statusRet = Status;
	}
	return statusRet;
}

NTSTATUS
DgiSocketConnect(
	_In_ IN_ADDR Ip,
	_In_ USHORT Port,
	_Inout_ PSZI_SOCKET_CONTEXT Context
	) 
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	SOCKADDR_IN			localAddress = {AF_INET, 0, IN4ADDR_ANY_INIT};
	SOCKADDR_IN			remoteAddress = {AF_INET, 0, IN4ADDR_LOOPBACK_INIT};
	PDGI_IRP_REQUEST	request = NULL;

	remoteAddress.sin_port = RtlUshortByteSwap(Port);
	remoteAddress.sin_addr = Ip;
	
	status = DgiSocketInitIrp(&request, TRUE);
	if(!NT_SUCCESS(status)) {
		DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	 
	status = Context->SocketProviderNpi.Dispatch->WskSocketConnect(Context->SocketProviderNpi.Client, SOCK_STREAM, IPPROTO_TCP, (PSOCKADDR)&localAddress, (PSOCKADDR)&remoteAddress,
																WSK_FLAG_CONNECTION_SOCKET, Context, &DgiSocketConnectDispatch, NULL, NULL, NULL, request->Irp);
	status = SziSocketWaitSocketOperation(status, request, Context, TRUE);
	if(!NT_SUCCESS_SOCKET(status)) {
		DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	Context->Socket = (PWSK_SOCKET)request->Irp->IoStatus.Information;
_error:
	DgiSocketFreeIrp(request);

	return status;
}

VOID
DgiSocketDisconnect(
	_In_ PSZI_SOCKET_CONTEXT Context
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	if(Context != NULL && Context->Socket != NULL) {
		PWSK_PROVIDER_CONNECTION_DISPATCH	socketDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)Context->Socket->Dispatch;
		{
			PDGI_IRP_REQUEST	request = NULL;
			status = DgiSocketInitIrp(&request, TRUE);
			if(NT_SUCCESS(status)) {
				status = socketDispatch->WskDisconnect(Context->Socket, NULL, 0, request->Irp); //WSK_FLAG_ABORTIVE
				status = SziSocketWaitSocketOperation(status, request, Context, TRUE);
				if(!NT_SUCCESS_SOCKET(status)) {
					DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
				}
				DgiSocketFreeIrp(request);
			}
		}
		{
			PDGI_IRP_REQUEST	request = NULL;
			status = DgiSocketInitIrp(&request, FALSE);
			if(NT_SUCCESS(status)) {
				status = socketDispatch->WskCloseSocket(Context->Socket, request->Irp);
				SziSocketWaitSocketOperation(status, request, Context, FALSE);
				Context->Socket = NULL;
				DgiSocketFreeIrp(request);
			}
		}
	}
}

NTSTATUS
DgiSocketWrite(
	_In_ PSZI_SOCKET_CONTEXT Context,
	_In_ PVOID Data,
	_In_ SIZE_T Size
	)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	WSK_BUF				bufferWsk;
	PDGI_IRP_REQUEST	request = NULL;

	RtlZeroMemory(&bufferWsk, sizeof(bufferWsk));

	if (Context != NULL && Context->Socket != NULL)
	{
		PWSK_PROVIDER_CONNECTION_DISPATCH socketDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)Context->Socket->Dispatch;
		
		bufferWsk.Offset = 0;
		bufferWsk.Length = Size;
		bufferWsk.Mdl = IoAllocateMdl(Data, (ULONG)Size, FALSE, FALSE, NULL);
		if(bufferWsk.Mdl == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			goto _error;
		}
		
		status = DgiSocketInitIrp(&request, TRUE);

		if(!NT_SUCCESS(status))
		{
			DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			goto _error;
		}

		MmBuildMdlForNonPagedPool(bufferWsk.Mdl);
		//MmProbeAndLockPages
		for(;;)
		{
			DgiSocketReuseIrp(request);
			status = socketDispatch->WskSend(Context->Socket, &bufferWsk, 0, request->Irp);		//WSK_FLAG_NODELAY
			status = SziSocketWaitSocketOperation(status, request, Context, TRUE);

			if(!NT_SUCCESS_SOCKET(status))
			{
				DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
				break;
			}

			ULONG byteSent = (ULONG)request->Irp->IoStatus.Information;
			if(byteSent < bufferWsk.Length) {
				bufferWsk.Offset += byteSent;
				continue;
			} else {
				status = STATUS_SUCCESS;
				break;
			}
		}

		DgiSocketFreeIrp(request);
	}

_error:
	if (bufferWsk.Mdl != NULL) {
		IoFreeMdl(bufferWsk.Mdl);
	}
	
	return status;
}


NTSTATUS
DgiSocketRead(
	_In_ PSZI_SOCKET_CONTEXT Context,
	_Out_ PVOID Data,
	_In_ SIZE_T Size,
	_Out_ PSIZE_T ReadSize
	) 
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;	
	WSK_BUF				bufferWsk;
	PDGI_IRP_REQUEST	request = NULL;

	RtlZeroMemory(&bufferWsk, sizeof(bufferWsk));

	if (Context != NULL && Context->Socket != NULL) {
		PWSK_PROVIDER_CONNECTION_DISPATCH socketDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)Context->Socket->Dispatch;

		bufferWsk.Offset = 0;
		bufferWsk.Length = Size;
		bufferWsk.Mdl = IoAllocateMdl(Data, (ULONG)Size, FALSE, FALSE, NULL);

		if(bufferWsk.Mdl == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			goto _error;
		}
		
		status = DgiSocketInitIrp(&request, TRUE);
		if(!NT_SUCCESS(status)) {
			DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			goto _error;
		}
		MmBuildMdlForNonPagedPool(bufferWsk.Mdl);

		status = socketDispatch->WskReceive(Context->Socket, &bufferWsk, 0, request->Irp);
		status = SziSocketWaitSocketOperation(status, request, Context, TRUE);
		if(!NT_SUCCESS_SOCKET(status)) {
			DbgPrint("DgiSocket!Status 0x%X File:%s:%d\n", status, __FILE__, __LINE__);
			goto _error;
		}

		*ReadSize = (SIZE_T)request->Irp->IoStatus.Information;
		status = STATUS_SUCCESS;
	} 
_error:
	if(bufferWsk.Mdl != NULL) {
		IoFreeMdl(bufferWsk.Mdl);
	}
	DgiSocketFreeIrp(request);
	return status;
}

NTSTATUS 
DgiSocketDisconnectEvent(
	_In_opt_ PVOID SocketContext,
	_In_     ULONG Flags
	)
{
	PSZI_SOCKET_CONTEXT context = SocketContext;
	context;

	DbgPrint("DgiSocket!DgiSocketDisconnectEvent 0x%X File:%s:%d\n", Flags, __FILE__, __LINE__);
	//SziSocketDisconnect(streamContext->SocketContext);
	return STATUS_SUCCESS;
}