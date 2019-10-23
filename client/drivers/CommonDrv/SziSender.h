#pragma once

#include <ntifs.h>
#include <Wsk.h>

#define NT_SUCCESS_SOCKET(_x) (NT_SUCCESS(_x) && STATUS_TIMEOUT != _x)

typedef struct _SZI_SOCKET_CONTEXT {
	//KIRQL					LockSocket;
	PWSK_SOCKET				Socket;
	ULONG					TimeOut;
	WSK_CLIENT_NPI			SocketClientNpi;
	WSK_REGISTRATION		SocketRegistration;
	WSK_CLIENT_DISPATCH		SocketDispatch;
	WSK_PROVIDER_NPI		SocketProviderNpi;
} SZI_SOCKET_CONTEXT, *PSZI_SOCKET_CONTEXT;

NTSTATUS
SziSenderInit();

BOOLEAN
SziSenderIsInit();

VOID
SziSenderDestroy();

NTSTATUS
SziSenderConnect(
	_In_ USHORT Port,
	_In_ PWSK_SOCKET * Socket
	);

VOID
SziSenderDisconnect(
	_In_ PWSK_SOCKET Socket
	);

NTSTATUS
SziSenderWrite(
	_In_ PWSK_SOCKET Socket,
	_In_ PUCHAR Data,
	_In_ SIZE_T Length
	);

NTSTATUS
SziBase64Encode(
	_In_ UCHAR const* BytesEncode,
	_In_ size_t SizeEncode,
	_Inout_ PUCHAR ByteOut,
	_In_ size_t SizeOut,
	_Out_ size_t* Size
	);

//////////////////////////////////////////////////////////////////////////

NTSTATUS
SziSocketInitContext(
	_Inout_ PSZI_SOCKET_CONTEXT * Context,
	_In_ ULONG	Timeout
	);

VOID
SziSocketDestroyContext(
	_In_ PSZI_SOCKET_CONTEXT Context
	);

NTSTATUS
SziSocketConnect(
	_In_ IN_ADDR Ip,
	_In_ USHORT Port,
	_Inout_ PSZI_SOCKET_CONTEXT Context
	);

VOID
SziSocketDisconnect(
	_In_ PSZI_SOCKET_CONTEXT Context
	);

NTSTATUS
SziSocketWrite(
	_In_ PSZI_SOCKET_CONTEXT Context,
	_In_ PVOID Data,
	_In_ SIZE_T Size
	);

NTSTATUS
SziSocketRead(
	_In_ PSZI_SOCKET_CONTEXT Context,
	_Out_ PVOID Data,
	_In_ SIZE_T Length,
	_Out_ PSIZE_T ReadSize
	);