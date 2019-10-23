#pragma once

#include <ntifs.h>
#include <DriverEvents.h>

typedef struct _EVENTSECURITY {	
	UNICODE_STRING	Param1;
	UNICODE_STRING	Object;				//FileName, ExeName, Other
	UNICODE_STRING	Sid;
	LARGE_INTEGER	Time;
	USHORT			TypeEvent;			//EVENT_EVENT_
	USHORT			Source;				//DRIVER_??
} EVENTSECURITY, *PEVENTSECURITY;

typedef struct _EVENTSLIST{	
	LIST_ENTRY		EventList;
	PEVENTSECURITY	Event;
} EVENTSLIST, *PEVENTSLIST;

typedef struct _EVENTCONTEXT {
	LIST_ENTRY		HeadEventsList;
	KSPIN_LOCK		EventListLock;
	PETHREAD		ThreadObject;
	KSEMAPHORE		QueueSemaphore;
	USHORT			SocketPort;
	LARGE_INTEGER	SentTimeOut;
	volatile LONG	ThreadStop;
} EVENTCONTEXT, *PEVENTCONTEXT;

NTSTATUS
DgiEventInit(
	_In_ USHORT Port,
	_In_ LONGLONG Timeout
	);

VOID
DgiEventDestoy();

NTSTATUS
DgiEventMake(
	_In_ USHORT Source,
	_In_ USHORT TypeEvent,
	_In_ PCUNICODE_STRING Param1,
	_In_opt_ PCUNICODE_STRING Object,
	_In_ PSID Sid,
	_Outptr_ PEVENTSECURITY * Event
	);

VOID
DgiEventFree(
	_In_ PEVENTSECURITY Event
	);

VOID
DgiEventAsyncSend(
	_In_ PEVENTSECURITY Event
	);

BOOLEAN
DgiEventIsEmpty();
