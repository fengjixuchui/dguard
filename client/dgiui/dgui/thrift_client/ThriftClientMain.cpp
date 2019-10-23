// 
// 	Author: 
// 			burluckij@gmail.com
// 			Burlutsky Stanislav
//

#include "ThriftClientMain.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include "../../../../service/DgiService/helpers/internal/helpers.h"

namespace thrift_client
{
	using namespace ::apache::thrift;
	using namespace ::apache::thrift::protocol;
	using namespace ::apache::thrift::transport;

	bool ClientMain::login(std::string _utf8masterpassword, std::string& _outSid)
	{
		dgi::AuthResponse result;

		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.login(result, _utf8masterpassword);
			transport->close();

			if (result.result.status == dgi::DgiStatus::Success)
			{
				_outSid = result.sessionId;
			}
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.result.status == dgi::DgiStatus::Success);
	}

	bool ClientMain::logout(std::string _sid)
	{
		dgi::DgiResult result;

		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.logout(result, _sid);
			transport->close();
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.status == dgi::DgiStatus::Success);
	}

	bool ClientMain::isValidSid(std::string _sid)
	{
		dgi::DgiResult result;

		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.isValidSession(result, _sid);
			transport->close();
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.status == dgi::DgiStatus::Success);
	}

	bool ClientMain::isRightPassword(std::string _password)
	{
		dgi::DgiResult result;

		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.isRightPassword(result, _password);
			transport->close();
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.status == dgi::DgiStatus::Success);
	}

	bool ClientMain::VerifyIsPasswordSet()
	{
		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::BoolResponse result;
			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.isPasswordSet(result);
			transport->close();

			if (result.errorResult.status == dgi::DgiStatus::Success)
			{
				return (result.bool_result == true);
			}
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientMain::SetMasterPassword(std::wstring _password)
	{
		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::DgiResult result;
			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.setPassword(result, strings::ws_s(_password, CP_UTF8) );
			transport->close();

			return (result.status == dgi::DgiStatus::Success);
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientMain::changeMasterPassword(std::string _sid, std::wstring _password, std::wstring _newpassword)
	{
		dgi::DgiResult result;

		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.changePassword(result, _sid, strings::ws_s(_password, CP_UTF8), strings::ws_s(_newpassword, CP_UTF8) );
			transport->close();
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.status == dgi::DgiStatus::Success);
	}

	bool ClientMain::canConnect()
	{
		try
		{
			std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
			std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			dgi::BoolResponse result;
			dgi::DgiServiceManagerClient client(protocol);

			transport->open();
			client.isPasswordSet(result);
			transport->close();

			return (result.errorResult.status == dgi::DgiStatus::Success);
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

}
