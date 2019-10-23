//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "..\ServiceClient.h"
#include "..\..\..\thrift\cpp\DgiBanking.h"

namespace thrift_client
{
	class ClientCards
	{
	public:

		ClientCards(std::string _host, int _port) : m_host(_host), m_port(_port) {

		}

		bool addCard(std::string _sid,
			std::string _urf8number,
			std::string _urf8pin,
			std::string _urf8cvv,
			std::string _urf8holder,
			std::string _urf8paySystem,
			std::string _urf8bankOwner,
			std::string _utf8description,
			int _validMonth,
			int _validYear);

		bool addCard(std::string _sid, dgi::BankCard& _cardInfo);

		bool getCard(std::string _sid, std::string _cardNumber, dgi::BankCard& _cardInfo);
		
		bool removeCardByNumber(std::string _sid, std::string _cardnumber);		

		bool getCards(std::string _sid, dgi::BankCardList& _cards);


		bool printAllCards(std::string _sid);
		bool printCardByNumber(std::string _sid, std::string _cardnumber);

		//bool canConnect();

	private:
		std::string m_host;
		int m_port;
	};
}
