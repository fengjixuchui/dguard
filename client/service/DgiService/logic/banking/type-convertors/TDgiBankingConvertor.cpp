//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//


#include "../../../helpers/internal/helpers.h"
#include "TDgiBankingConvertor.h"


namespace thrift_impl
{
	bool fromThrift(const ::dgi::BankCard& _from, logic::banking::storage::CardInfo& _to)
	{
		logic::banking::storage::CardInfo target;

		target.validDate.vd_year = _from.vd.vd_year;
		target.validDate.vd_month = _from.vd.vd_month;

        fill_chars(target.name, _from.name.c_str());
		fill_chars(target.bankOwner, _from.bankOwner.c_str());
		fill_chars(target.cardHolder, _from.holder.c_str());
		fill_chars(target.cardNumber, _from.number.c_str());
		fill_chars(target.cvv, _from.cvvCode.c_str());
		fill_chars(target.paySystem, _from.paySystem.c_str());
		fill_chars(target.shortDescription, _from.shortDescription.c_str());
		fill_chars(target.pin, _from.pinCode.c_str());

		_to = target;

		return true;
	}

	bool toThrift(const logic::banking::storage::CardInfo& _from, ::dgi::BankCard& _to)
	{
		::dgi::BankCard target;

		target.vd.vd_month = _from.validDate.vd_month;
		target.vd.vd_year = _from.validDate.vd_year;

        target.name = _from.name;
		target.bankOwner = _from.bankOwner;
		target.holder = _from.cardHolder;
		target.number = _from.cardNumber;
		target.cvvCode = _from.cvv;
		target.paySystem = _from.paySystem;
		target.shortDescription = _from.shortDescription;
		target.pinCode = _from.pin;

		_to = target;

		return true;
	}

}


