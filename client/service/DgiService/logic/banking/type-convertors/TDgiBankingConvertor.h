//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../../../../../../thrift/cpp/DgiBanking.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/banking/storage/CardsStorage.h"


namespace thrift_impl
{
	bool fromThrift(const ::dgi::BankCard& _from, logic::banking::storage::CardInfo& _to);

	bool toThrift(const logic::banking::storage::CardInfo& _from, ::dgi::BankCard& _to);

}

