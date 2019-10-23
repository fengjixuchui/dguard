//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "../../../../../../thrift/cpp/DgiBanking.h"

#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCommon.h"

namespace service
{
	namespace thrift_impl
	{
		using namespace ::logic::common;

		using namespace ::apache::thrift;
		using namespace ::apache::thrift::protocol;
		using namespace ::apache::thrift::transport;
		using namespace ::apache::thrift::server;
		using boost::shared_ptr;


		class DgiBankingService : virtual public ::dgi::DgiBankingIf
		{

		public:
			DgiBankingService(std::string _logfile);


			//
			// Apache::thrift interface implementation is here.
			//

			virtual void AddCard(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::BankCard& _card) override;
			virtual void DeleteCard(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::BankCard& _card) override;
			virtual void DeleteCardByNumber(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::CardNumber& _cn) override;
			virtual void GetCards(::dgi::BankCardList& _return, const  ::dgi::DgiSid& _sid) override;
			virtual void GetCard(::dgi::Resp_BankCard& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::CardNumber& _cardNumber) override;

		protected:

		private:

			logfile m_log;
		};
	}
}
