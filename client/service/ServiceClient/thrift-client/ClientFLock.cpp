//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "ClientFLock.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include "../../DgiService/helpers/internal/helpers.h"

namespace thrift_client
{
	using namespace ::apache::thrift;
	using namespace ::apache::thrift::protocol;
	using namespace ::apache::thrift::transport;

	bool ClientFLock::add(const std::string& _sid, const FLockObject& _newFlock)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		::dgi::DgiStatus::type response;
		::dgi::FLockInfo info;

		try
		{
			info.obj.flockId = _newFlock.uniqueId;
			info.obj.path = strings::ws_s(_newFlock.filePath, CP_UTF8);

			// We should be careful with that two conversions.
			info.obj.type = (::dgi::FLockObjectType::type) _newFlock.type;
			info.state = (::dgi::FLockState::type) _newFlock.state;

			transport->open();
			response = client.add(_sid, info);
			transport->close();

			return (response == ::dgi::DgiStatus::Success);
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::getFlocks(const std::string& _sid, FLocks& _outFlocks)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		::dgi::FLockListResponse response;

		try
		{
			transport->open();
			client.getFlocks(response, _sid);
			transport->close();

			if (response.result.status == ::dgi::DgiStatus::Success)
			{
				for (auto thriftFLock : response.flocks)
				{
					FLockObject flock;
					flock.filePath = strings::s_ws( thriftFLock.obj.path );
					flock.uniqueId = thriftFLock.obj.flockId;

					// (!) This type conversion is dangerous and works only if types have same definition.
					flock.type = (FLockObjectType) thriftFLock.obj.type;
					flock.state = (FLockState) thriftFLock.state;
					
					// Save converted types.
					_outFlocks.push_back(flock);
				}

				return true;
			}
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::getState(const std::string& _sid, const std::wstring& _filepath, FLockState& _outState)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		::dgi::FLockStateResponse response;

		try
		{
			transport->open();
			client.getState(response, _sid, strings::ws_s(_filepath, CP_UTF8) );
			transport->close();

			// Return this result only we have no other specific errors.
			if (response.result.status == ::dgi::DgiStatus::Success)
			{
                //
				//  We take only FLock's state.
				//  (!) This type conversion is dangerous and works only if 'FLockState' is strong equal to thrift defined enum - '::dgi::FLockState::type'.
				//

                _outState = (FLockState)(response.flinf.state);
				return true;
			}
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::setState(const std::string& _sid, const std::wstring& _filepath, const FLockState& _newState)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		::dgi::DgiResult response;

		try
		{
			// (!) This type conversion is dangerous and works only if 'FLockState' is strong equal to thrift defined enum - '::dgi::FLockState::type'.
			::dgi::FLockState::type newState = (::dgi::FLockState::type) _newState;

			transport->open();
			client.setState(response, _sid, strings::ws_s(_filepath, CP_UTF8), newState);
			transport->close();

			return response.status == ::dgi::DgiStatus::Success;
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::present(const std::string& _sid, std::wstring _filepath)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		dgi::BoolResponse response;

		try
		{
			transport->open();
			client.present(response, _sid, strings::ws_s(_filepath, CP_UTF8));
			transport->close();

			if (response.errorResult.status == ::dgi::DgiStatus::Success)
			{
				// Return this result only we have no other specific errors.
				return response.bool_result;
			}
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::presentId(const std::string& _sid, std::string _id)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		dgi::BoolResponse response;

		try
		{
			transport->open();
			client.presentById(response, _sid, _id);
			transport->close();

			if (response.errorResult.status == ::dgi::DgiStatus::Success)
			{
				// Return this result only we have no other specific errors.
				return response.bool_result;
			}
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::remove(const std::string& _sid, std::wstring _filepath)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		dgi::DgiResult response;

		try
		{
			transport->open();
			client.remove(response, _sid, strings::ws_s(_filepath, CP_UTF8) );
			transport->close();

			return response.status == ::dgi::DgiStatus::Success;
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

	bool ClientFLock::removeAll(const std::string& _sid)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiFolderLockClient client(protocol);
		dgi::DgiResult response;

		try
		{
			transport->open();
			client.removeAll(response, _sid);
			transport->close();

			return response.status == ::dgi::DgiStatus::Success;
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return false;
	}

    bool ClientFLock::isSupportedFs(std::wstring _path, bool& _supported)
    {
        std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
        std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

        dgi::DgiFolderLockClient client(protocol);
        dgi::BoolResponse response;

        try
        {
            auto path = strings::ws_s(_path, CP_UTF8);

            transport->open();
            client.isSupportedFs(response, path);
            transport->close();

            if (::dgi::DgiStatus::Success == response.errorResult.status)
            {
                _supported = response.bool_result;
                return (true);
            }
        }
        catch (TTransportException& e)
        {
            std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
            return false;
        }

        return false;
    }

}
