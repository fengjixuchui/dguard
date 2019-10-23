//
// Data Guard Local Service
//
//
// Author:				Burlutsky Stas (burluckij@gmail.com)
// Creation date-time:	06.02.2018 18:38
//

#include "stdafx.h"
#include "helpers/internal/log.h"
#include "services/windows/windowsService.h"
#include "helpers/system/privileges/privileges.h"
#include "tests/banking/testCardsKeeper.h"
#include "helpers/encryption/cryptoHelpers.h"
#include "tests/common/DgiCryptoTest.h"
#include "tests/common/DgiTest.h"
#include "services/thrift_impl/Main/DgiMainService.h"
#include "services/ServiceWorker.h"

void DbgMain();
void DbgTests();
bool ParseCommandLine(int argc, TCHAR** argv);

int wmain(int argc, TCHAR** argv)
{
	logfile& log = ::logic::common::DgiEngine::getConf().getLog(LOG_COMMON);
	log.printEx("%s: Application is run.", __FUNCTION__);

	sys::privileges::getMaxPrivileges();

	ParseCommandLine(argc, argv);

	service::windows::DgiSrv::start((LPTHREAD_START_ROUTINE) ::service::WindowsServiceWorker, nullptr);

	//::service::thrift_impl::DebugServices();
	//::service::WindowsServiceWorker(0);
	return 0;
}

void DbgMain()
{
	// system("Color A0"); Light green color.

	logfile& log = ::logic::common::DgiEngine::getConf().getLog(LOG_COMMON);
	log.printEx("%s: Application run in DEBUG mode.", __FUNCTION__);

	::service::WindowsServiceWorker(0);

	log.printEx("\n\n%s: Press any key to exit..", __FUNCTION__);
	getchar();

	ExitProcess(0);
}

void DbgTests()
{
	logfile& log = ::logic::common::DgiEngine::getConf().getLog(LOG_COMMON);

    tests::test_FLockChangeState();
	tests::testEraseFileThroughDriver();
	tests::test_Aes();
	tests::test_Aes128Encoder();
	tests::test_FileEncoder();
	tests::testCardsKeeper();

	log.printEx("%s: Press any key to exit..\n\n", __FUNCTION__);
	getchar();
}

bool ParseCommandLine(int argc, TCHAR** argv)
{
	typedef void(*pfnHandler)();
	typedef std::map<std::wstring, pfnHandler> handlers;

	handlers actions;
	actions[L"-console_test"] = DbgMain;

	for (int i = 0; i < argc; ++i)
	{
		if (actions.count(argv[i]))
		{
			actions[std::wstring(argv[i])]();
		}
	}

	return true;
}
