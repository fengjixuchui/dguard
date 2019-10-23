//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "..\ServiceClient.h"
#include "..\..\..\thrift\cpp\DgiEncryption.h"

namespace thrift_client
{
	class ClientEncryption
	{
	public:

		ClientEncryption(std::string _host, int _port) : m_host(_host), m_port(_port) {

		}

		bool isEncoded(std::wstring _path, bool& isEncrypted);

		bool getFileInfo(std::wstring _path, dgi::ResponseFileInfo& _info);

		bool encryptFile(std::wstring _filepath, std::string _key, bool _useMasterPassword, dgi::EncryptionAlgType::type _algtype);

		bool decryptFile(std::wstring _filepath, std::string _key, bool _useMasterPassword, bool& _compromisedIntegrity);

		bool encryptFileAsync(std::wstring _filepath, std::string _key, bool _useMasterPassword, dgi::EncryptionAlgType::type _algtype, dgi::AsyncResponse& _response);

		bool decryptFileAsync(std::wstring _filepath, std::string _key, bool _useMasterPassword, dgi::AsyncResponse& _response /*, bool& _compromisedIntegrity*/);

		bool getAsyncState(dgi::AsyncId _id, dgi::DgiResult& _result);

	private:
		std::string m_host;
		int m_port;
	};
}

