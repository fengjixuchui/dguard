//
// Developer:
//				Burlutsky Stanislav
//				burluckij@gmail.com
//

#include "ServiceWorker.h"
#include "thrift_impl/Main/DgiMainService.h"


namespace service
{
	unsigned long  __stdcall WindowsServiceWorker(void* _pArg)
	{
		UNREFERENCED_PARAMETER(_pArg);

		std::string thisFunction(__FUNCTION__);
		logfile& log = ::logic::common::DgiEngine::getConf().getLog(LOG_COMMON /*LOG_MAIN_SRV*/);

		using TMainService = ::service::thrift_impl::ServiceManager<::service::thrift_impl::DgiMainService, ::dgi::DgiServiceManagerProcessor>;
		TMainService serviceMain(THRIFT_MAIN_PORT, log.getLogFilePath());

		//
		//	Before to start internal thrift-services, we need to start engine with
		//	all subsystems (erasing, encryption, wallet, file system lock).
		//


		//
		//	Start thrift-services.
		//

		if (serviceMain.start())
		{
			log.print(thisFunction + ": Main thrift-service is running.");

			if (serviceMain.getService()->startInternalServices())
			{
				// Apply security settings if they are.
				// ...

				// Start all necessary background threads.
				// ...

				log.print(thisFunction + ": Wait for completion.");

				// Wait till somebody sets shutdown event to start completion process.
				auto procTerm = ::logic::common::DgiEngine::getSync().waitForProcessTermination();

				log.print(thisFunction + ": woke be by process termination event, result code is " + std::to_string(procTerm) );

				log.print(thisFunction + ": The windows service is going to stop all internal services.");

				// Stop all early started threads.
				// ...

				if (!serviceMain.getService()->stopInternalServices())
				{
					log.print(thisFunction + ": Internal service stopped.");
				}
				else
				{
					log.print(thisFunction + ": Error - not all internal service were stopped correctly. Please see own internal services logs for more details.");
				}

				log.print(thisFunction + ": It is time to stop main service.");
				serviceMain.stop();
				log.print(thisFunction + ": Main service stopped.");
			}
			else
			{
				log.print(thisFunction + ": Error - internal services were not started.");
			}
		}
		else
		{
			log.print(thisFunction + ": Error - general thrift-services manager was not started.");
		}

		return 0;
	}
}
