//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "ServiceClient.h"
#include "thrift-client/ClientMain.h"
#include "thrift-client/ClientShredder.h"
#include "thrift-client/ClientCards.h"
#include "..\DgiService\helpers\internal\log.h"


ServiceInfo g_serviceInfo;



std::string readline()
{
	std::string line;

	while (line.empty())
	{
		std::getline(std::cin, line);
	}

	return line;
}

std::wstring readlineW()
{
	std::wstring line;

	while (line.empty())
	{
		std::getline(std::wcin, line);
	}

	return line;
}

std::string getServiceIp()
{
	if (g_serviceInfo.m_ip.empty())
	{
		return "127.0.0.1";
	}

	return g_serviceInfo.m_ip;
}

int getMainServicePort()
{
	return g_serviceInfo.m_portMain;
}

int getCardsServicePort()
{
	return g_serviceInfo.m_portCards;
}

int getEraseServicePort()
{
	return g_serviceInfo.m_portErase;
}

int getEncryptionPort()
{
	return g_serviceInfo.m_portEncryption;
}

int getFlockServicePort()
{
	return g_serviceInfo.m_portFlock;
}



void initConsoleClient()
{
	g_serviceInfo.m_ip = "127.0.0.1";

	g_serviceInfo.m_portMain = THRIFT_MAIN_PORT;
	g_serviceInfo.m_portCards = THRIFT_BANKING_PORT;
	g_serviceInfo.m_portFlock = THRIFT_FLOCK_PORT;
	g_serviceInfo.m_portErase = THRIFT_ERASING_PORT;
	g_serviceInfo.m_portEncryption = THRIFT_ENCRYPTION_PORT;
}
