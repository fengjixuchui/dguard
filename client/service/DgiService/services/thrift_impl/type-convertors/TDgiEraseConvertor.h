//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../../../logic/common/DgiCommon.h"
#include "../../../../../../thrift/cpp/DgiSecureErase.h"
#include "../../../helpers/internal/log.h"

namespace service
{
	namespace thrift_impl
	{
		//
		// dgiSecureEraseTypes.EraseResponse getEraseState(1: dgiCommonTypes.DgiSid _sid, 2 : dgiCommonTypes.AsyncId  _asyncId)
		// dgiCommonTypes.AsyncResponse eraseFiles(1: dgiCommonTypes.DgiSid _sid, 2: dgiSecureEraseTypes.EraseList _toErase)

		bool fromThrift(const dgi::EraseList& _fromList, std::vector<::logic::common::EraseObject>& _toEraseList);

		bool toThrift(const std::vector<::logic::common::EraseObjectResult>& _from, dgi::EraseErrorList& _to);
	}
}
