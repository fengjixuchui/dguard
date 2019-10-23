#include "DgiEvent.h"
#include "DriverEvents.h"
#include "DgiSender.h"
#include "DgiTools.h"

#define NTSTRSAFE_NO_CB_FUNCTIONS
#include <Ntstrsafe.h>

#define CF_EVENTSECURETY_TAG	'SEzS'		//SzES
#define CF_EVENTLIST_TAG		'lEzS'		//SzEl
#define CF_EVENTBASE64_TAG		'bEzS'		//SzEb
#define CF_EVENTBUFFER_TAG		'BEzS'		//SzEB
#define CF_EVENTSTRING_TAG		'sEzS'		//SzEs

#define BUFFER_PP				(0x25 * sizeof(WCHAR))

EVENTCONTEXT			gEventContext;
volatile LONG			gIsEventInit = 0;

KSTART_ROUTINE SziEventThreadRoute;

NTSTATUS
SziEventPushRecord(
	_In_ PEVENTSECURITY Event
	);

NTSTATUS
SziEventPopRecord(
	_In_ PEVENTSECURITY * Event
	);

NTSTATUS
SziEventSend(
	_In_ PWSK_SOCKET Socket,
	_In_ PEVENTSECURITY Event
	);

NTSTATUS
SziEventSerialize(
	_In_ PEVENTSECURITY Event,
	_Out_ WCHAR** Buffer,
	_Out_ size_t* Size
	);

NTSTATUS
DgiEventInit(
	_In_ USHORT Port,
	_In_ LONGLONG Timeout
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	HANDLE	 thread = NULL;

	if (InterlockedCompareExchange(&gIsEventInit, 0, 0)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(&gEventContext, sizeof(gEventContext));
	InitializeListHead(&gEventContext.HeadEventsList);
	KeInitializeSpinLock(&gEventContext.EventListLock);
	InterlockedExchange(&gEventContext.ThreadStop, 0);
	KeInitializeSemaphore(&gEventContext.QueueSemaphore, 0, MAXLONG);
	
	gEventContext.SocketPort = Port;// 14900;
	gEventContext.SentTimeOut.QuadPart = Timeout * -10000;// -1 * 10 * 1000 * 1000;		//1 sec

	status = PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, NULL, NULL, NULL, SziEventThreadRoute, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	status = ObReferenceObjectByHandle(thread, THREAD_ALL_ACCESS, NULL, KernelMode, &gEventContext.ThreadObject, NULL);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	
_error:
	if (NT_SUCCESS(status)) {
		InterlockedExchange(&gIsEventInit, 1);
	}
	if (thread) {
		ZwClose(thread);
	}

	return status;
}

VOID
DgiEventDestoy()
{		
	if (InterlockedCompareExchange(&gIsEventInit, 0, 0)) {
		KIRQL oldIrql;
		InterlockedExchange(&gEventContext.ThreadStop, 1);
		KeReleaseSemaphore(&gEventContext.QueueSemaphore, LOW_PRIORITY, 1, TRUE);
		KeWaitForSingleObject(gEventContext.ThreadObject, Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(gEventContext.ThreadObject);

		KeAcquireSpinLock(&gEventContext.EventListLock, &oldIrql);
		while (!IsListEmpty(&gEventContext.HeadEventsList)) {
			PLIST_ENTRY pList = RemoveHeadList(&gEventContext.HeadEventsList);
			PEVENTSLIST	itemList = CONTAINING_RECORD(pList, EVENTSLIST, EventList);
			KeReleaseSpinLock(&gEventContext.EventListLock, oldIrql);

			DgiEventFree(itemList->Event);
			ExFreePoolWithTag(itemList, CF_EVENTLIST_TAG);

			KeAcquireSpinLock(&gEventContext.EventListLock, &oldIrql);
		}
		KeReleaseSpinLock(&gEventContext.EventListLock, oldIrql);

		DgiSenderDestroy();
	} else
	{
		DbgPrint("SziEvent not init. File %s:%d\n", __FILE__, __LINE__);
	}
}

NTSTATUS
DgiEventMake(
	_In_ USHORT Source,
	_In_ USHORT TypeEvent,
	_In_ PCUNICODE_STRING Param1,
	_In_opt_ PCUNICODE_STRING Object,
	_In_ PSID Sid,
	_Outptr_ PEVENTSECURITY * Event
	)
{
	NTSTATUS		status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER	sysTime;
	PEVENTSECURITY	newEvent = NULL;

	if (InterlockedCompareExchange(&gIsEventInit, 0, 0) == 0) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		return status;
	}

	KeQuerySystemTime(&sysTime);

	newEvent  = ExAllocatePoolWithTag(PagedPool, sizeof(EVENTSECURITY), CF_EVENTSECURETY_TAG);
	if (newEvent == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _exit;
	}
	RtlZeroMemory(newEvent, sizeof(EVENTSECURITY));
	ExSystemTimeToLocalTime(&sysTime, &newEvent->Time);

	if (Object != NULL && Object->Buffer != NULL && Object->Length > 0) {
		newEvent->Object.Buffer = ExAllocatePoolWithTag(PagedPool, Object->Length, CF_EVENTSTRING_TAG);
		if (newEvent->Object.Buffer == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
			goto _exit;
		}
		newEvent->Object.Length = newEvent->Object.MaximumLength = Object->Length;
		RtlCopyUnicodeString(&newEvent->Object, Object);
	} else {
		RtlInitEmptyUnicodeString(&newEvent->Object, NULL, 0);
	}
	
	if (Sid != NULL) {
		status = DgiConvertSidToUnicodeString(&newEvent->Sid, Sid, CF_EVENTSTRING_TAG);
		if (!NT_SUCCESS(status)) {
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
			goto _exit;
		}
	}

	if(Param1 != NULL && Param1->Buffer != NULL && Param1->Length > 0) {
		newEvent->Param1.Buffer = ExAllocatePoolWithTag(PagedPool, Param1->Length, CF_EVENTSTRING_TAG);
		if(newEvent->Param1.Buffer == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
			goto _exit;
		}
		newEvent->Param1.Length = newEvent->Param1.MaximumLength = Param1->Length;
		RtlCopyUnicodeString(&newEvent->Param1, Param1);
	} else {
		RtlInitEmptyUnicodeString(&newEvent->Param1, NULL, 0);
	}
		
	newEvent->TypeEvent = TypeEvent;
	newEvent->Source = Source;

	*Event = newEvent;
	status = STATUS_SUCCESS;

_exit:
	if (!NT_SUCCESS(status) && newEvent != NULL) {
		if(newEvent->Object.Buffer != NULL) {
			ExFreePoolWithTag(newEvent->Object.Buffer, CF_EVENTSTRING_TAG);
		}
		if(newEvent->Param1.Buffer != NULL) {
			ExFreePoolWithTag(newEvent->Param1.Buffer, CF_EVENTSTRING_TAG);
		}
		if (newEvent->Sid.Buffer != NULL) {
			ExFreePoolWithTag(newEvent->Sid.Buffer, CF_EVENTSTRING_TAG);
		}
		ExFreePoolWithTag(newEvent, CF_EVENTSECURETY_TAG);
	}

	return status;
}

VOID
DgiEventFree(
	_In_ PEVENTSECURITY Event
	)
{
	if(Event->Object.Buffer != NULL) {
		ExFreePoolWithTag(Event->Object.Buffer, CF_EVENTSTRING_TAG);
	}
	if(Event->Param1.Buffer != NULL) {
		ExFreePoolWithTag(Event->Param1.Buffer, CF_EVENTSTRING_TAG);
	}
	if(Event->Sid.Buffer != NULL) {
		ExFreePoolWithTag(Event->Sid.Buffer, CF_EVENTSTRING_TAG);
	}
	ExFreePoolWithTag(Event, CF_EVENTSECURETY_TAG);
}

NTSTATUS
SziEventPushRecord(
	_In_ PEVENTSECURITY Event
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	PEVENTSLIST eventList = NULL;
	KIRQL		oldIrql;

	if (InterlockedCompareExchange(&gIsEventInit, 0, 0) == 0) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		return status;
	}

	eventList = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(EVENTSLIST), CF_EVENTLIST_TAG);	
	if (eventList == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto _error;
	}
	RtlZeroMemory(eventList, sizeof(EVENTSLIST));
	eventList->Event = Event;
	
	KeAcquireSpinLock(&gEventContext.EventListLock, &oldIrql);
	InsertTailList(&gEventContext.HeadEventsList, &eventList->EventList);
	KeReleaseSpinLock(&gEventContext.EventListLock, oldIrql);

	status = STATUS_SUCCESS;
_error:

	return status;
}

NTSTATUS
SziEventPopRecord(
	_In_ PEVENTSECURITY * Event
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	KIRQL		oldIrql;
	PEVENTSLIST itemList = NULL;

	if (InterlockedCompareExchange(&gIsEventInit, 0, 0) == 0) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		return status;
	}

	KeAcquireSpinLock(&gEventContext.EventListLock, &oldIrql);
	if(!IsListEmpty(&gEventContext.HeadEventsList)) {
		PLIST_ENTRY pList = RemoveHeadList(&gEventContext.HeadEventsList);
		itemList = CONTAINING_RECORD(pList, EVENTSLIST, EventList);
	}
	KeReleaseSpinLock(&gEventContext.EventListLock, oldIrql);

	if (itemList != NULL) {
		*Event = itemList->Event;
		status = STATUS_SUCCESS;
		ExFreePoolWithTag(itemList, CF_EVENTLIST_TAG);
	}
	return status;
}

BOOLEAN
DgiEventIsEmpty()
{
	BOOLEAN isEmpty = FALSE;
	KIRQL	oldIrql;

	if (InterlockedCompareExchange(&gIsEventInit, 0, 0)) {
		KeAcquireSpinLock(&gEventContext.EventListLock, &oldIrql);
		isEmpty = IsListEmpty(&gEventContext.HeadEventsList);
		KeReleaseSpinLock(&gEventContext.EventListLock, oldIrql);
	} else {
		DbgPrint("SziEvent not init. File %s:%d\n", __FILE__, __LINE__);
	}
	
	return isEmpty;
}

VOID
DgiEventAsyncSend(
	_In_ PEVENTSECURITY Event
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	status = SziEventPushRecord(Event);
	if (NT_SUCCESS(status))	{
		KeReleaseSemaphore(&gEventContext.QueueSemaphore, 0, 1, FALSE);
	} else {
		DgiEventFree(Event);
	}	
}

VOID
SziEventThreadRoute(
	_In_ PVOID Context
	)
{
	UNREFERENCED_PARAMETER(Context);

	NTSTATUS		status = STATUS_UNSUCCESSFUL;
	PWSK_SOCKET		wskSocket = NULL;
	
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	
	for(;;)	{		
		status = KeWaitForSingleObject(&gEventContext.QueueSemaphore, Executive, KernelMode, FALSE, &gEventContext.SentTimeOut);
		if(status == STATUS_TIMEOUT) {
			if(wskSocket != NULL) {
				DgiSenderDisconnect(wskSocket);
				wskSocket = NULL;
			}
		}

		BOOLEAN isInit = DgiSenderIsInit();
		if (isInit == FALSE) {
			status = DgiSenderInit();
			isInit = NT_SUCCESS(status);
			DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		}

		if(isInit && !DgiEventIsEmpty()) {
			PEVENTSECURITY sEvent = NULL;

			if (wskSocket == NULL) {
				status = DgiSenderConnect(gEventContext.SocketPort, &wskSocket);				
			} else {
				status = STATUS_SUCCESS;
			}			

			if (NT_SUCCESS(status) && wskSocket != NULL) {
				status = SziEventPopRecord(&sEvent);
				if(NT_SUCCESS(status)) {
					status = SziEventSend(wskSocket, sEvent);
					if(NT_SUCCESS(status)) {
						DgiEventFree(sEvent);
					} else {
						DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);

						status = SziEventPushRecord(sEvent);
						if(!NT_SUCCESS(status)) {
							DgiEventFree(sEvent);							
						}
						
						DgiSenderDisconnect(wskSocket);
						wskSocket = NULL;
					}
				}
			}
		}
		//Выход из треда
		if(InterlockedCompareExchange(&gEventContext.ThreadStop, 0, 0)) {
			break;
		}
	}
	if(wskSocket != NULL) {
		DgiSenderDisconnect(wskSocket);
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS
SziEventSend(
	_In_ PWSK_SOCKET Socket,
	_In_ PEVENTSECURITY Event
	)
{
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	PUCHAR		bufferBase64 = NULL;
	size_t		sizeBase64 = 0;
	WCHAR*		bufferEvent = NULL;
	size_t		sizeBase64Max = 0;
	size_t		sizeEvent = 0;
	PHEAD_EVENT  headEvent = NULL;
	
	status = SziEventSerialize(Event, &bufferEvent, &sizeEvent);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	sizeBase64Max = sizeEvent * 2;

	bufferBase64 = ExAllocatePoolWithTag(NonPagedPool, sizeBase64Max, CF_EVENTBASE64_TAG);
	if(bufferBase64 == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(bufferBase64, sizeBase64Max);

	status = DgiBase64Encode((UCHAR*)bufferEvent, sizeEvent, bufferBase64, sizeBase64Max, &sizeBase64);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	headEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(HEAD_EVENT), CF_EVENTBASE64_TAG);
	if(headEvent == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}
	RtlZeroMemory(headEvent, sizeof(HEAD_EVENT));

	headEvent->Mask = EVENT_MASK;
	headEvent->Version = EVENT_VERSION;
	headEvent->Size = (unsigned short)sizeBase64;
	
	status = DgiSenderWrite(Socket, (PUCHAR)headEvent, sizeof(HEAD_EVENT));
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = DgiSenderWrite(Socket, bufferBase64, sizeBase64);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

_error:
	if(bufferBase64 != NULL) {
		ExFreePoolWithTag(bufferBase64, CF_EVENTBASE64_TAG);
	}
	if (bufferEvent != NULL) {
		ExFreePoolWithTag(bufferEvent, CF_EVENTBUFFER_TAG);
	}
	if(headEvent != NULL) {
		ExFreePoolWithTag(headEvent, CF_EVENTBASE64_TAG);
	}
	return status;
}

NTSTATUS
SziEventSerialize(
	_In_ PEVENTSECURITY Event,
	_Out_ WCHAR** Buffer,
	_Out_ size_t* Size
	)
{	
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	//Максимально возможная длинна
	size_t	sizeEventMax =  (SECURITY_MAX_SID_SIZE * sizeof(WCHAR)) + 
							Event->Object.Length + 
							Event->Param1.Length + 
							(18 * sizeof(WCHAR)) +		//Event->Source
							(18 * sizeof(WCHAR)) +		//Event->TypeEvent
							(18 * sizeof(WCHAR)) +		//Event->Time
							BUFFER_PP;					//| |

	WCHAR*	bufferEvent = NULL;	
	size_t	sizeEvent = 0;
	UNICODE_STRING nullString = RTL_CONSTANT_STRING(L"");

	bufferEvent = ExAllocatePoolWithTag(PagedPool, sizeEventMax, CF_EVENTBUFFER_TAG);
	if(bufferEvent == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("NT_SUCCESS 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	//Source | Type | Time | Object | Param1 | SID |
	RtlZeroMemory(bufferEvent, sizeEventMax);
	status = RtlStringCchPrintfW(bufferEvent, sizeEventMax / sizeof(WCHAR), L"%d|%d|%lld|%wZ|%wZ|%wZ|",
								Event->Source,
								Event->TypeEvent,
								Event->Time.QuadPart,
								Event->Object.Length ? &Event->Object : &nullString,
								Event->Param1.Length ? &Event->Param1 : &nullString,
								Event->Sid.Length ? &Event->Sid : &nullString);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	status = RtlStringCchLengthW(bufferEvent, sizeEventMax / sizeof(WCHAR), &sizeEvent);
	if(!NT_SUCCESS(status)) {
		DbgPrint("status 0x%X File %s:%d\n", status, __FILE__, __LINE__);
		goto _error;
	}

	*Buffer = bufferEvent;
	*Size = sizeEvent * sizeof(WCHAR);
	status = STATUS_SUCCESS;
	goto _exit;
_error:
	if (bufferEvent != NULL) {
		ExFreePoolWithTag(bufferEvent, CF_EVENTBUFFER_TAG);
	}
_exit:

	return status;
}