//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "ClientCards.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include "../../DgiService/helpers/internal/helpers.h"

namespace thrift_client
{
	using namespace ::apache::thrift;
	using namespace ::apache::thrift::protocol;
	using namespace ::apache::thrift::transport;

	bool ClientCards::addCard(std::string _sid,
		std::string _utf8number,
		std::string _utf8pin,
		std::string _utf8cvv,
		std::string _utf8holder,
		std::string _utf8paySystem,
		std::string _utf8bankOwner,
		std::string _utf8description,
		int _validMonth,
		int _validYear)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiResult result;
		dgi::DgiBankingClient client(protocol);
		dgi::BankCard card;

		//
		// Here I fill all necessary information which belongs to the card.
		//
		card.number = _utf8number;
		card.cvvCode = _utf8cvv;
		card.bankOwner = _utf8bankOwner;
		card.holder = _utf8holder;
		card.paySystem = _utf8paySystem;
		card.pinCode = _utf8pin;
		card.shortDescription = _utf8description;
		card.vd.vd_month = _validMonth;
		card.vd.vd_year = _validYear;

		try
		{
			transport->open();
			client.AddCard(result, _sid, card);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.status == dgi::DgiStatus::Success);
	}


	bool ClientCards::addCard(std::string _sid, dgi::BankCard& _cardInfo)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiResult result;
		dgi::DgiBankingClient client(protocol);

		try
		{
			transport->open();
			client.AddCard(result, _sid, _cardInfo);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (result.status != dgi::DgiStatus::Success)
		{
			printf("\n%s: error dgi status is %d\n", __FUNCTION__, result.status);
		}

		return (result.status == dgi::DgiStatus::Success);
	}

	bool ClientCards::getCard(std::string _sid, std::string _cardNumber, dgi::BankCard& _cardInfo)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiBankingClient client(protocol);
		dgi::Resp_BankCard response;

		try
		{
			transport->open();
			client.GetCard(response, _sid, _cardNumber);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		if (response.result.status == dgi::DgiStatus::Success)
		{
			_cardInfo = response.cardInfo;
		}

		return (response.result.status == dgi::DgiStatus::Success);
	}

	bool ClientCards::printAllCards(std::string _sid)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiResult result;
		dgi::DgiBankingClient client(protocol);
		dgi::BankCardList cards;

		try
		{
			transport->open();
			client.GetCards(cards, _sid);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		for (auto card : cards)
		{
// 			printf("\ncard details:\t %s (%s) %s %s %s\t : pin - %s, cvv %s, vd %d/%d\n",
// 				card.number.c_str(),
// 				card.shortDescription.c_str(),
// 				card.bankOwner.c_str(),
// 				card.holder.c_str(),
// 				card.paySystem.c_str(),
// 				card.pinCode.c_str(),
// 				card.cvvCode.c_str(),
// 				card.vd.vd_month,
// 				card.vd.vd_year);
		
			printf("\nCard details:\n\tNumber: %s\n\tDescription: %s\n\tBank owner: %s\n\tCard holder: %s\n\tPay system: %s\n\tPin: %s\n\tCVV: %s\n\tValid date: %d/%d\n",
				card.number.c_str(),
				card.shortDescription.c_str(),
				card.bankOwner.c_str(),
				card.holder.c_str(),
				card.paySystem.c_str(),
				card.pinCode.c_str(),
				card.cvvCode.c_str(),
				card.vd.vd_month,
				card.vd.vd_year);
		}

		return true;
	}

	bool ClientCards::printCardByNumber(std::string _sid, std::string _cardnumber)
	{
		dgi::BankCard card;

		if (this->getCard(_sid, _cardnumber, card))
		{
			printf("\ncard details:\t %s %s %s %s\t : pin - %s, cvv %s\n",
				card.number.c_str(), card.bankOwner.c_str(), card.holder.c_str(), card.paySystem.c_str(),
				card.pinCode.c_str(), card.cvvCode.c_str());

			return true;
		}

		return false;
	}

	bool ClientCards::removeCardByNumber(std::string _sid, std::string _cardnumber)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiResult result;
		dgi::DgiBankingClient client(protocol);

		try
		{
			transport->open();
			client.DeleteCardByNumber(result, _sid, _cardnumber);
			transport->close();
		}
		catch (TTransportException& e)
		{
			std::cout << std::endl << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}

		return (result.status == dgi::DgiStatus::Success);
	}

	bool ClientCards::getCards(std::string _sid, dgi::BankCardList& _cards)
	{
		std::shared_ptr<TTransport> socket(new TSocket(m_host, m_port));
		std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

		dgi::DgiBankingClient client(protocol);
		dgi::Resp_BankCard response;

		try
		{
			transport->open();
			client.GetCards(_cards, _sid);
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
