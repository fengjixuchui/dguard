//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiBankingService.h"
#include "../../../logic/session/DgiSession.h"
#include "../../../logic/common/DgiEngine.h"

namespace service
{
	namespace thrift_impl
	{

		DgiBankingService::DgiBankingService(std::string _logfile):
			m_log(_logfile)
		{

		}

		void DgiBankingService::AddCard(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid, const ::dgi::BankCard& _card)
		{
			m_log.printEx("%s", __FUNCTION__);

			//
			// Verify only presence of the operation.
			//
			if (logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}
			
			//
			// Verify and convert input data.
			//
			logic::banking::storage::CardInfo ci;
			if (!::thrift_impl::fromThrift(_card, ci))
			{
				_return.status = dgi::DgiStatus::InvalidFormat;
				_return.description = "can't convert input data to target internal format.";
				return;
			}

			//
			// Add data to storage.
			//
			auto res = ::logic::common::DgiEngine::getWallet().addCard( ci );
			if (res != InternalStatus::Int_Success)
			{
				m_log.printEx("%s: error - couldn't add a card to Wallet.", __FUNCTION__);
				_return.status = dgi::DgiStatus::UnknownError;
				_return.description = "can't convert input data to target internal format.";
				return;
			}

			_return.status = dgi::DgiStatus::Success;
		}

		void DgiBankingService::DeleteCard(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid, const ::dgi::BankCard& _card)
		{
			m_log.printEx("%s", __FUNCTION__);

			//
			// Verify only presence of the operation.
			//

			if (logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			//
			// Verify and convert input data.
			//

			logic::banking::storage::CardInfo ci;
			if (!::thrift_impl::fromThrift(_card, ci))
			{
				_return.status = dgi::DgiStatus::InvalidFormat;
				_return.description = "can't convert input data to target internal format.";
				return;
			}

			//
			// Remove the card from storage.
			//

			auto res = ::logic::common::DgiEngine::getWallet().removeCard(ci);
			if (res != InternalStatus::Int_Success)
			{
				m_log.printEx("%s: error - couldn't remove card from Wallet.", __FUNCTION__);
				_return.status = dgi::DgiStatus::UnknownError;
				_return.description = "couldn't remove card from Wallet";
				return;
			}

			_return.status = dgi::DgiStatus::Success;
		}

		void DgiBankingService::DeleteCardByNumber(::dgi::DgiResult& _return, const ::dgi::DgiSid& _sid, const ::dgi::CardNumber& _cn)
		{
			m_log.print(std::string(__FUNCTION__) + ": remove - " + _cn);

			//
			// Verify only presence of the operation.
			//

			if (logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			//
			// Remove card from storage through banking::manager (middle component).
			//

			auto res = ::logic::common::DgiEngine::getWallet().removeCard(_cn);
			if (res != InternalStatus::Int_Success)
			{
				m_log.print( std::string(__FUNCTION__) + ": error - couldn't remove card from Wallet.");
				_return.status = dgi::DgiStatus::UnknownError;
				_return.description = "couldn't remove card from Wallet";
				return;
			}

			_return.status = dgi::DgiStatus::Success;
		}

		void DgiBankingService::GetCards(::dgi::BankCardList& _return, const ::dgi::DgiSid& _sid)
		{
			m_log.printEx("%s", __FUNCTION__);

			if (logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				//_return.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			std::vector<::logic::banking::storage::CardInfo> cards;
			auto istatus = DgiEngine::getWallet().getCards(cards);

			if (InternalStatus::Int_Success != istatus)
			{
				m_log.printEx("%s: internal error is %d", __FUNCTION__, istatus);
				//_return.status = dgi::DgiStatus::UnknownError;
				//_return.description = "couldn't get cards from Wallet";
				return;
			}

			for (auto card : cards)
			{
				::dgi::BankCard bankCard;

				if (::thrift_impl::toThrift(card, bankCard))
				{
					//
					// Add card to result list.
					//

					_return.push_back(bankCard);
				}
				else
				{
					m_log.printEx("%s: error - while converting bankcard from internal type to thrift-defined.", __FUNCTION__);
				}
			}
		}

		void DgiBankingService::GetCard(::dgi::Resp_BankCard& _return, const ::dgi::DgiSid& _sid, const ::dgi::CardNumber& _cardNumber)
		{
			m_log.printEx("%s", __FUNCTION__);

			//
			// Verify only presence of the operation.
			//

			if (logic::session::Storage::get().present(_sid) == false)
			{
				m_log.print(std::string(__FUNCTION__) + ": error - invalid session " + _sid);
				_return.result.status = ::dgi::DgiStatus::AccessDenied;
				return;
			}

			//
			//  Look up card in our Wallet! Please =)
			//

			::logic::banking::storage::CardInfo card;
			auto istatus = DgiEngine::getWallet().getCard(_cardNumber, card);
			if (istatus != InternalStatus::Int_Success)
			{
				m_log.printEx("%s: getCard(..) returned internal error is %d", __FUNCTION__, istatus);

				_return.result.status = dgi::DgiStatus::NotFound;
				_return.result.description = "couldn't get card from Wallet.";
				return;
			}

			if (::thrift_impl::toThrift(card, _return.cardInfo))
			{
				_return.result.status = dgi::DgiStatus::Success;
			}
			else
			{
				m_log.printEx("%s: error has occurred while converting from internal to thrift type.", __FUNCTION__);

				_return.result.status = dgi::DgiStatus::InvalidFormat;
			}
		}
	}
}
