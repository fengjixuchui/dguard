//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../DgiService/stdafx.h"

void DataGuardConsole();

void initConsoleClient();

std::string getServiceIp();

int getMainServicePort();
int getEraseServicePort();
int getFlockServicePort();
int getCardsServicePort();
int getEncryptionPort();

std::string readline();
std::wstring readlineW();


struct ServiceInfo
{
	std::string m_ip;
	int m_portMain;
	int m_portErase;
	int m_portCards;
	int m_portFlock;
	int m_portEncryption;
};

