//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"

typedef string CardNumber

struct ValidDate
{
	1: i32 vd_month,
	2: i32 vd_year
}

struct BankCard
{
	// Короткое произвольное название. К примеру – зарплатная, матери, отца, бабушки,
	// студенческая. 
	// "My own card from last job", до 512 символов.
	// 
	1: string shortDescription,
	
	// Банк, который выпустил карту - "Sberbank", "City bank", "Bank of America". 
	//
	2: string bankOwner, 
	
	// Тип платёжной системы - "Visa", "Mastercard", "Maestro", "МИР", "Noname".
	//
	3: string paySystem,
	
	// Номер карты - "3440 3455 0098 1052".
	//
	4: CardNumber number,
	
	// Срок истечения карты.
	//
	5: ValidDate vd,
	
	// Пин код - "5239"
	//
	6: string pinCode,
	
	// CVV код - "123", "379".
	//
	7: string cvvCode,
	
	// Владелец карты - Johny Cash.
	//
	8: string holder,
	
	// Имя карты, до 20 символов - "Salary card".
	//
	9: string name
}

typedef list<BankCard> BankCardList

struct Resp_BankCard
{
	1: dgiCommonTypes.DgiResult result,
	2: BankCard cardInfo
}
