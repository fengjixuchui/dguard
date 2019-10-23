//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "ServiceClient.h"
#include "thrift-client/ClientMain.h"
#include "thrift-client/ClientCards.h"
#include "thrift-client/ClientShredder.h"
#include "..\DgiService\helpers\internal\log.h"

void test();

int main()
{
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
	{
		printf("\nWSAStartup failed.\n");
	}

	//test();

	DataGuardConsole();

	getchar();

	return 0;
}

void test()
{
	logfile log("client_test.log");
	thrift_client::ClientMain dataGuard("127.0.0.1", THRIFT_MAIN_PORT);

	if (!dataGuard.canConnect())
	{
		log.print(std::string(__FUNCTION__) + " error - could not connect to Data Guard service!\n");
		return;
	}

	if (dataGuard.VerifyIsPasswordSet())
	{
		log.print(std::string(__FUNCTION__) + " master password is set.");
	}
	else
	{
		log.print(std::string(__FUNCTION__) + " master password was not set.");

		if ( dataGuard.SetMasterPassword(L"password") )
		{
			log.print(std::string(__FUNCTION__) + " password was successfully set.");
		}
		else
		{
			log.print(std::string(__FUNCTION__) + " error - could not set master password.");
		}
	}

	// Shredder.

	thrift_client::ClientShredder shredder("127.0.0.1", THRIFT_ERASING_PORT);

	if (shredder.EraseFile(L"D:\\work\\test\\file.txt"))
	{
		log.print(std::string(__FUNCTION__) + ": success - file was removed!");
	}
	else
	{
		log.print(std::string(__FUNCTION__) + " error - we could not remove the file.");
	}
}

