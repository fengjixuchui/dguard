//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "..\ServiceClient.h"
#include "..\..\..\thrift\cpp\DgiSecureErase.h"

namespace thrift_client
{
	class ClientShredder
	{
	public:

		ClientShredder(std::string _host, int _port) : m_host(_host), m_port(_port) {

		}

		bool EraseFile(std::wstring _path);

		bool EraseDirectory(std::wstring _path, dgi::AsyncResponse& _response);

		bool GetAsyncState(dgi::AsyncId _id, dgi::DgiResult& _result);

	private:
		std::string m_host;
		int m_port;
	};
}

