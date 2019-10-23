//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"


service DgiServiceManager
{
	void run()
	
	void stop()
	
	//
	// В будущем можно генерировать ключевую пару и перед логином, передовать клиенту публичный ключ для шифрования мастерпассворда,
	// а приватный ключ использовать для расшифрования закодированного пароля, это позволит не гонять по сети открытый пароль.
	// Конечно же из-за временных ограничений, сейчас мы не исползуем такую схему защиты мастер пароля.
	//
	
	dgiCommonTypes.AuthResponse login(1:string _masterPassword)
	
	dgiCommonTypes.DgiResult logout(1: dgiCommonTypes.DgiSid _sid)
	
	dgiCommonTypes.DgiResult changePassword(1: dgiCommonTypes.DgiSid _sid, 2: string _currentPassword, 3: string _newPassword)
	
	dgiCommonTypes.DgiResult isRightPassword(1:string _masterPassword)
	
	dgiCommonTypes.DgiResult isValidSession(1: dgiCommonTypes.DgiSid _sid)
	
	//
	// Returns true if the master password was have ever set.
	// It helps to understand - Does we need set master password for first login time.
	//
	dgiCommonTypes.BoolResponse isPasswordSet()
	
	// Sets master password if it was not set already.
	//
	dgiCommonTypes.DgiResult setPassword(1:string _masterPassword)
}
