//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"
include "dgiBankingTypes.thrift"

//
// Here is an interface of service for working with bank cards.
//

service DgiBanking
{
	//
	// Add new card to common area.
	//
	dgiCommonTypes.DgiResult AddCard(1: dgiCommonTypes.DgiSid _sid, 2: dgiBankingTypes.BankCard _card)
	
	//
	// Delete card from protected-storage.
	//
	dgiCommonTypes.DgiResult DeleteCard(1: dgiCommonTypes.DgiSid _sid, 2: dgiBankingTypes.BankCard _card)
	
	//
	// Deletes by card number only.
	//
	dgiCommonTypes.DgiResult DeleteCardByNumber(1: dgiCommonTypes.DgiSid _sid, 2: dgiBankingTypes.CardNumber _cn)
	
	//
	// Returns all list of protected cards.
	//
	dgiBankingTypes.BankCardList GetCards(1: dgiCommonTypes.DgiSid _sid)
	
	//
	// Return only one card info.
	//
	dgiBankingTypes.Resp_BankCard GetCard(1: dgiCommonTypes.DgiSid _sid, 2: dgiBankingTypes.CardNumber _cardNumber)
}
