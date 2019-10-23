//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "ManagerBanking.h"
//#include "../../../logic/common/DgiCrypt.h"
#include "../../../logic/common/DgiEngine.h"

namespace logic
{
	namespace banking
	{
		namespace manager
		{
			Wallet::Wallet(std::wstring _logfilepath) :
				m_log(strings::ws_s(_logfilepath)),
				m_cardStorage(::logic::common::DgiEngine::getCryptor(), m_log) // But! May be it's better to make another log for CardsKeeper?
			{
				// ...
			}

			logic::common::InternalStatus Wallet::addCard(const ::logic::banking::storage::CardInfo& _card)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_cardStorage.add(_card))
				{
					m_log.printEx("%s: error - can't add card to Wallet.", __FUNCTION__);
					return logic::common::InternalStatus::Int_UnknownError;
				}

				return InternalStatus::Int_Success;
			}

			logic::common::InternalStatus Wallet::getCard(const std::string& _inCardNumber /* 4585 4594 4580 7018 */, ::logic::banking::storage::CardInfo& _outCard)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_cardStorage.present(_inCardNumber))
				{
					m_log.print(std::string(__FUNCTION__) + ": error - can't find card " + _inCardNumber + " in Wallet. ");
					return logic::common::InternalStatus::Int_NotFound;
				}

				::logic::banking::storage::CardInfo ci;
				if (!m_cardStorage.get(_inCardNumber, ci))
				{
					m_log.print(std::string(__FUNCTION__) + ": error - can't get info about card " + _inCardNumber + " from Wallet. ");
					return logic::common::InternalStatus::Int_UnknownError;
				}

				_outCard = ci;

				return InternalStatus::Int_Success;
			}

			logic::common::InternalStatus Wallet::removeCard(std::string _cardNumber /* 4585 4594 4580 7018 */)
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_cardStorage.present(_cardNumber))
				{
					m_log.print(std::string(__FUNCTION__) + ": error - can't find card " + _cardNumber + " in Wallet. ");
					return logic::common::InternalStatus::Int_NotFound;
				}

				m_cardStorage.remove(_cardNumber);

				return InternalStatus::Int_Success;
			}

			logic::common::InternalStatus Wallet::removeCard(const ::logic::banking::storage::CardInfo& _card)
			{
				m_log.printEx("%s", __FUNCTION__);

				m_cardStorage.remove(_card);

				return InternalStatus::Int_Success;
			}

			logic::common::InternalStatus Wallet::clear()
			{
				m_log.printEx("%s", __FUNCTION__);

				if (!m_cardStorage.clear())
				{
					m_log.printEx("%s: error - can't clear Wallet.", __FUNCTION__);
					return logic::common::InternalStatus::Int_UnknownError;
				}

				return InternalStatus::Int_Success;
			}

			unsigned long Wallet::length() const
			{
				return m_cardStorage.length();
			}

			logic::common::InternalStatus Wallet::getCards(std::vector<::logic::banking::storage::CardInfo>& _cards)
			{
				m_log.printEx("%s", __FUNCTION__);

				std::vector<::logic::banking::storage::CardInfo> cards;
				if (!m_cardStorage.getAll(cards))
				{
					m_log.printEx("%s: error - can't read cards in Wallet.", __FUNCTION__);
					return logic::common::InternalStatus::Int_UnknownError;
				}

				_cards.swap(cards);

				return InternalStatus::Int_Success;
			}

			bool Wallet::ctrInit()
			{
				return false;
			}

			bool Wallet::ctrlLateInit()
			{
				return false;
			}

			bool Wallet::ctrlIsRunning()
			{
				return false;
			}

			bool Wallet::ctrlShutdown(bool _canWait)
			{
				return false;
			}

			std::string Wallet::ctrlGetName()
			{
				return SYSTEM_WALLET;
			}

			bool Wallet::ctrlSetPassword(::logic::common::MasterPassword _password)
			{
				return m_cardStorage.setPassword(_password);
			}

			bool Wallet::ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword)
			{
				bool success = m_cardStorage.reEncrypt(_password, _currentPassword);

				if (success)
				{
					this->ctrlSetPassword(_password);
				}

				return success;
			}

		}
	}
}
