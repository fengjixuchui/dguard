//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//


#include "../../logic/common/DgiEngine.h"
#include "../../helpers/internal/helpers.h"
#include "../../logic/common/master-password.h"

namespace tests
{
	bool testErase();
	bool testEraseDir();
	bool testEraseFile();
	bool testEraseFileThroughDriver();

	bool test_MasterPasswordStorage();
	bool test_MasterPasswordModifications();
	bool test_Aes();

	bool test_FileEncoder();
	
    bool test_FLockChangeState();
}
