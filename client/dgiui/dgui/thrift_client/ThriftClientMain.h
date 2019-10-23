// 
// 	Author: 
// 			burluckij@gmail.com
// 			Burlutsky Stanislav
// 
// 

#pragma once

#include <iostream>
#include "ThriftCommon.h"
#include "..\..\..\thrift\cpp\DgiServiceManager.h"

namespace thrift_client
{
	class ClientMain
	{
	public:

		ClientMain(std::string _host, int _port) : m_host(_host), m_port(_port) {

		}

		bool login(std::string _utf8masterpassword, std::string& _outSid);

		bool logout(std::string _sid);

		bool isValidSid(std::string _sid);

		bool isRightPassword(std::string _password);

		bool VerifyIsPasswordSet();

		bool SetMasterPassword(std::wstring _password);

		bool changeMasterPassword(std::string _sid, std::wstring _password, std::wstring _newpassword);

		bool canConnect();

	private:
		std::string m_host;
		int m_port;
	};
}
