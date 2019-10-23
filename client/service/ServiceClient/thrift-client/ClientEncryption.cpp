//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "ClientEncryption.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include "../../DgiService/helpers/internal/helpers.h"

namespace thrift_client
{
	using namespace ::apache::thrift;
	using namespace ::apache::thrift::protocol;
	using namespace ::apache::thrift::transport;


	bool ClientEncryption::isEncoded(std::wstring _path, bool& isEncrypted)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);
		dgi::BoolResponse response;

		try
		{
			transport->open();
			client.isFileEncoded( response, strings::ws_s(_path, CP_UTF8) );
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (response.errorResult.status == dgi::DgiStatus::Success)
		{
			isEncrypted = response.bool_result;
			return true;
		}

		return false;
	}

	bool ClientEncryption::getFileInfo(std::wstring _path, dgi::ResponseFileInfo& _info)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);

		try
		{
			transport->open();
			client.getFileInfo(_info, strings::ws_s(_path, CP_UTF8));
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (_info.result.status == dgi::DgiStatus::Success)
		{
			return true;
		}

		return false;
	}

	bool ClientEncryption::encryptFile( std::wstring _filepath, std::string _key, bool _useMasterPassword, dgi::EncryptionAlgType::type _algtype)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);
		dgi::RequestEncryptFile request;
		dgi::DgiResult result;

		request.filePath = strings::ws_s(_filepath, CP_UTF8);
		request.encryptionKey = _key;
		request.useMasterPassword = _useMasterPassword;
		request.encodingAlgorithm = _algtype;

		try
		{
			transport->open();
			client.encryptFile(result, request);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (result.status != dgi::DgiStatus::Success)
		{
			printf("\n%s: error - dgiStatus code is %d \n", __FUNCTION__, result.status);
		}

		return result.status == dgi::DgiStatus::Success;
	}

	bool ClientEncryption::decryptFile(std::wstring _filepath, std::string _key, bool _useMasterPassword, bool& _compromisedIntegrity)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);
		dgi::RequestDecodeFile request;
		dgi::ResponseDecodeFile result;

		request.filePath = strings::ws_s(_filepath, CP_UTF8);
		request.encryptionKey = _key;
		request.useMasterPassword = _useMasterPassword;

		try
		{
			transport->open();
			client.decodeFile(result, request);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (result.result.status != dgi::DgiStatus::Success)
		{
			printf("\n%s: error - dgiStatus code is %d \n", __FUNCTION__, result.result.status);
		}
		else
		{
			_compromisedIntegrity = result.integrityCompromised;
		}

		return  result.result.status == dgi::DgiStatus::Success;
	}

	bool ClientEncryption::encryptFileAsync(std::wstring _filepath, std::string _key, bool _useMasterPassword, dgi::EncryptionAlgType::type _algtype, dgi::AsyncResponse& _response)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);
		dgi::RequestEncryptFile request;

		request.filePath = strings::ws_s(_filepath, CP_UTF8);
		request.encryptionKey = _key;
		request.useMasterPassword = _useMasterPassword;
		request.encodingAlgorithm = _algtype;

		try
		{
			transport->open();
			client.encryptFileAsync(_response, request);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (_response.result.status != dgi::DgiStatus::Success)
		{
			printf("\n%s: error - dgiStatus code is %d \n", __FUNCTION__, _response.result.status );
		}

		return _response.result.status == dgi::DgiStatus::Success;
	}

	bool ClientEncryption::decryptFileAsync(std::wstring _filepath, std::string _key, bool _useMasterPassword, dgi::AsyncResponse& _response /*, bool& _compromisedIntegrity*/)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);
		dgi::RequestDecodeFile request;
		dgi::ResponseDecodeFile result;

		request.filePath = strings::ws_s(_filepath, CP_UTF8);
		request.encryptionKey = _key;
		request.useMasterPassword = _useMasterPassword;

		try
		{
			transport->open();
			client.decodeFileAsync(_response, request);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (result.result.status != dgi::DgiStatus::Success)
		{
			printf("\n%s: error - dgiStatus code is %d \n", __FUNCTION__, result.result.status);
		}

		return  result.result.status == dgi::DgiStatus::Success;
	}

	bool ClientEncryption::getAsyncState(dgi::AsyncId _id, dgi::DgiResult& _result)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiEncryptionClient client(protocol);

		try
		{
			transport->open();
			client.getState(_result, _id);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return true;
	}

}
