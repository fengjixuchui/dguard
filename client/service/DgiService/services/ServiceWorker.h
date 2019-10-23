//
// Developer:
//				Burlutsky Stanislav
//				burluckij@gmail.com
//

#pragma once

#include "../logic/common/DgiEngine.h"
#include "thrift_impl/Main/DgiMainService.h"


namespace service
{
	unsigned long  __stdcall WindowsServiceWorker(void* _pArg);
}
