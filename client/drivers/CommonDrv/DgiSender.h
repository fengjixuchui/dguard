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
DgiSenderInit();

BOOLEAN
DgiSenderIsInit();

VOID
DgiSenderDestroy();

NTSTATUS
DgiSenderConnect(
	_In_ USHORT Port,
	_In_ PWSK_SOCKET * Socket
	);

VOID
DgiSenderDisconnect(
	_In_ PWSK_SOCKET Socket
	);

NTSTATUS
DgiSenderWrite(
	_In_ PWSK_SOCKET Socket,
	_In_ PUCHAR Data,
	_In_ SIZE_T Length
	);

NTSTATUS
DgiBase64Encode(
	_In_ UCHAR const* BytesEncode,
	_In_ size_t SizeEncode,
	_Inout_ PUCHAR ByteOut,
	_In_ size_t SizeOut,
	_Out_ size_t* Size
	);

//////////////////////////////////////////////////////////////////////////

NTSTATUS
DgiSocketInitContext(
	_Inout_ PSZI_SOCKET_CONTEXT * Context,
	_In_ ULONG	Timeout
	);

VOID
DgiSocketDestroyContext(
	_In_ PSZI_SOCKET_CONTEXT Context
	);

NTSTATUS
DgiSocketConnect(
	_In_ IN_ADDR Ip,
	_In_ USHORT Port,
	_Inout_ PSZI_SOCKET_CONTEXT Context
	);

VOID
DgiSocketDisconnect(
	_In_ PSZI_SOCKET_CONTEXT Context
	);

NTSTATUS
DgiSocketWrite(
	_In_ PSZI_SOCKET_CONTEXT Context,
	_In_ PVOID Data,
	_In_ SIZE_T Size
	);

NTSTATUS
DgiSocketRead(
	_In_ PSZI_SOCKET_CONTEXT Context,
	_Out_ PVOID Data,
	_In_ SIZE_T Length,
	_Out_ PSIZE_T ReadSize
	);
