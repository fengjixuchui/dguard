//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "testCardsKeeper.h"
#include "../../logic/common/DgiEngine.h"
#include "../../helpers/internal/helpers.h"

namespace tests
{
	void testCardsKeeper()
	{
		logfile log("dgi_test.log");
		::logic::common::MasterPassword password(L"pass");
		::logic::banking::storage::CardsKeeper keeper(::logic::common::DgiEngine::getCryptor(), log, "test_cards.storage");

		keeper.clear();

		keeper.setPassword( password );

		//::logic::common::DgiEngine::initPassword(::logic::common::MasterPassword(L"pass"));

		::logic::banking::storage::CardInfo ci = {0};
		fill_chars(ci.cardNumber, "5689 5322 1743 01544");
		fill_chars(ci.cardHolder, "stas burlutsky is a holder");

		if (keeper.add(ci))
		{
			log.print(std::string(__FUNCTION__) + " success - just added one card and current Wallet size is " + std::to_string(keeper.length()));

			std::vector<::logic::banking::storage::CardInfo> cards;
			
			if (keeper.getAll(cards))
			{
				log.print(std::string(__FUNCTION__) + "I will enumerate all cards! Right now.");

				for (auto card : cards)
				{
					log.print(std::string(__FUNCTION__) + " card info: " + card.cardNumber + " " + card.cardHolder);
				}
			}
			else
			{
				log.print(std::string(__FUNCTION__) + " error - could not get cards list!");
			}
		}
		else
		{
			log.print(std::string(__FUNCTION__) + " error - card was not added!");
		}
	}
}


