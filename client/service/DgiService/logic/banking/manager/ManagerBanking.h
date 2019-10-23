//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <mutex>


#include "../../../helpers/internal/log.h"
#include "../type-convertors/TDgiBankingConvertor.h"
#include "../storage/CardsStorage.h"
#include "../../common/master-password.h"
#include "../../common/DgiCommonControl.h"


namespace logic
{
	namespace banking
	{
		namespace manager
		{
			using namespace ::logic::common;

			class Wallet : public ::logic::common::DgiCommonControl
			{
			public:
				Wallet(std::wstring _logfilepath = L"wallet.log");

				common::InternalStatus addCard(const ::logic::banking::storage::CardInfo& _card);
				common::InternalStatus getCard(const std::string& _inCardNumber /* 4585 4594 4580 7018 */, ::logic::banking::storage::CardInfo& _outCard);
				common::InternalStatus removeCard(const ::logic::banking::storage::CardInfo& _card);
				common::InternalStatus removeCard(std::string _cardNumber /* 4585 4594 4580 7018 */);
				common::InternalStatus clear();
				common::InternalStatus getCards(std::vector<::logic::banking::storage::CardInfo>& _cards);
				unsigned long length() const;


				//
				//	Inherited interface - ::logic::common::DgiCommonControl.
				//

				virtual bool ctrInit() override;
				virtual bool ctrlLateInit() override;
				virtual bool ctrlIsRunning() override;
				virtual bool ctrlShutdown(bool _canWait) override;
				virtual std::string ctrlGetName() override;
				virtual bool ctrlSetPassword(::logic::common::MasterPassword _password) override;
				virtual bool ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword) override;

			private:
				logfile m_log;

				// Locks all access on any operations with the Wallet.
				// std::mutex m_walletLock;

				storage::CardsKeeper m_cardStorage;
			};
		}
	}
}
